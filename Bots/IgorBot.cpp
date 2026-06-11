#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"
#include <vector>
#include <cmath>
#include <algorithm>

class IgorBot : public IBot {
private:

    // 1. KIEROWNICA I NAWIGACJA
    const float WAYPOINT_REACHED_DIST = 20.0f; // W jakiej odległości (w pikselach) bot "odhacza" punkt i bierze następny
    const float STEERING_P_GAIN = 3.0f;        // Jak agresywnie bot kręci kierownicą (im więcej, tym ostrzej reaguje na błąd kątowy)
    const float STEERING_SMOOTHING = 0.15f;    // Wygładzanie (0.01 - wolno, 1.0 - natychmiast)
    const float STEERING_DEADZONE = 0.03f;     // Błąd w radianach (~1.7 stopnia), poniżej którego jedziemy prosto
    float currentSteeringOutput = 0.0f;        // Pamięta fizyczne wychylenie kół w poprzedniej klatce

    // 2. PRĘDKOŚĆ I HAMOWANIE (LOOKAHEAD)
    const int   SPEED_LOOKAHEAD_POINTS = 8;    // Ile punktów w przód po optymalnej linii bot patrzy, by ocenić zakręt
    const float MAX_STRAIGHT_SPEED = 220.0f;   // Prędkość docelowa na idealnie prostej drodze (piksele/s)
    const float MIN_CORNER_SPEED = 70.0f;      // Prędkość docelowa w najostrzejszym możliwym zakręcie
    const float HARD_CORNER_ANGLE = 1.2f;      // Kąt w radianach (~68 stopni), który bot uważa za "najostrzejszy zakręt" i zwalnia do MIN_CORNER_SPEED

    // 3. PEDAŁY (AGRESYWNOŚĆ)
    const float THROTTLE_P_GAIN = 0.05f;       // Jak szybko wciska gaz, gdy brakuje mu prędkości
    const float BRAKE_P_GAIN = 0.1f;           // Jak mocno depcze hamulec, gdy ma nadmiar prędkości

    bool lineCalculated = false;
    std::vector<Vector2D> optimalLine;
    int currentWaypointIndex = 0;

    float normalizeAngle(float angle) {
        const float PI = 3.14159265f;
        while (angle > PI)  angle -= 2.0f * PI;
        while (angle < -PI) angle += 2.0f * PI;
        return angle;
    }

public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        
        // 1. OBLICZANIE LINII (Tylko raz na początku wyścigu)
        if (!lineCalculated) {
            optimalLine = calculateOptimalRacingLine(data.track);
            lineCalculated = true;

            // Znajdujemy najbliższy punkt z nowej linii jako nasz cel startowy
            float minDistSq = 1e9f;
            for (size_t i = 0; i < optimalLine.size(); ++i) {
                Vector2D diff = optimalLine[i] - data.myCar.position;
                float distSq = diff.magnitudeSquared();
                if (distSq < minDistSq) {
                    minDistSq = distSq;
                    currentWaypointIndex = i;
                }
            }
        }

        if (optimalLine.empty()) return; // Zabezpieczenie

        // 2. LOGIKA ŚLEDZENIA ŚCIEŻKI (KIEROWNICA)
        Vector2D targetPos = optimalLine[currentWaypointIndex];
        Vector2D toTarget = targetPos - data.myCar.position;
        float distToTarget = toTarget.magnitude();

        // Czy dojechaliśmy do punktu? Jeśli tak, bierzemy następny
        if (distToTarget < WAYPOINT_REACHED_DIST) {
            currentWaypointIndex = (currentWaypointIndex + 1) % optimalLine.size();
            targetPos = optimalLine[currentWaypointIndex];
            toTarget = targetPos - data.myCar.position;
        }

        // Błąd kąta do bezpośredniego celu
        float desiredAngle = std::atan2(toTarget.y, toTarget.x);
        float currentAngle = data.myCar.rotationAngle;
        float steeringError = normalizeAngle(desiredAngle - currentAngle);

        // Skręt (P-Controller kierownicy)
        // A. Martwa strefa (Deadzone)
        // Jeśli błąd jest mniejszy niż deadzone, ignorujemy go (zapobiega to mikrokorektom na prostych)
        if (std::abs(steeringError) < STEERING_DEADZONE) {
            steeringError = 0.0f;
        }

        // B. Wyliczamy, jak mocno bot CHCE skręcić w tej klatce
        float targetSteering = std::clamp(steeringError * STEERING_P_GAIN, -1.0f, 1.0f);

        // C. Wygładzanie (Linear Interpolation - Lerp)
        // Zamiast szarpać kierownicą, płynnie przesuwamy obecny stan w stronę celu.
        currentSteeringOutput = currentSteeringOutput + (targetSteering - currentSteeringOutput) * STEERING_SMOOTHING;

        // D. Wysłanie wygładzonego sygnału do silnika fizycznego
        out.steeringAngle = currentSteeringOutput;

        // 3. LOGIKA PRĘDKOŚCI (LOOKAHEAD BRAKING)
        // Patrzymy kilka punktów w przód, by przewidzieć zakręt
        int futureIndex = (currentWaypointIndex + SPEED_LOOKAHEAD_POINTS) % optimalLine.size();
        Vector2D futureTarget = optimalLine[futureIndex];
        
        Vector2D toFuture = futureTarget - data.myCar.position;
        float futureDesiredAngle = std::atan2(toFuture.y, toFuture.x);
        float futureAngleDiff = std::abs(normalizeAngle(futureDesiredAngle - currentAngle));

        // Oceniamy ostrość zakrętu (0.0 to prosta, 1.0 to ostry zakręt)
        float cornerSharpness = std::min(futureAngleDiff / HARD_CORNER_ANGLE, 1.0f);

        // Interpolacja pożądanej prędkości
        // Jeśli cornerSharpness == 0, targetSpeed = MAX_STRAIGHT_SPEED
        // Jeśli cornerSharpness == 1, targetSpeed = MIN_CORNER_SPEED
        float targetSpeed = MAX_STRAIGHT_SPEED - (cornerSharpness * (MAX_STRAIGHT_SPEED - MIN_CORNER_SPEED));
        
        // 4. OBSŁUGA PEDAŁÓW (P-Controller Prędkości)
        float currentSpeed = data.myCar.velocity.magnitude();
        float speedError = targetSpeed - currentSpeed;

        if (speedError > 0.0f) {
            out.throttle = std::clamp(speedError * THROTTLE_P_GAIN, 0.0f, 1.0f);
            out.brake = 0.0f;
        } else {
            out.throttle = 0.0f;
            out.brake = std::clamp(std::abs(speedError) * BRAKE_P_GAIN, 0.0f, 1.0f);
        }
    }

    std::string getName() const override {
        return "IgorBot";
    }

    ColorRGB getColor() const override {
        return ColorRGB(220, 20, 60); 
    }

private:
    std::vector<Vector2D> calculateOptimalRacingLine(const TrackInfo& track) 
    {
        const float CAR_WIDTH = 12.0f;       
        const int NUM_NODES = 11;            
        const float TURN_PENALTY = 200.0f;   
        
        int N = track.innerBoundaries.size();
        if (N < 3) return {}; 

        int virtualN = N * 2; 

        std::vector<std::vector<Vector2D>> grid(virtualN, std::vector<Vector2D>(NUM_NODES));

        for (int i = 0; i < virtualN; ++i) {
            int real_i = i % N; 
            
            Vector2D inner = track.innerBoundaries[real_i];
            Vector2D outer = track.outerBoundaries[real_i];
            
            Vector2D dir = outer - inner;
            float width = dir.magnitude();
            Vector2D dirNorm = dir / width;

            Vector2D safeInner = inner + (dirNorm * CAR_WIDTH);
            Vector2D safeOuter = outer - (dirNorm * CAR_WIDTH);

            for (int j = 0; j < NUM_NODES; ++j) {
                float t = static_cast<float>(j) / (NUM_NODES - 1);
                grid[i][j] = safeInner + ((safeOuter - safeInner) * t);
            }
        }

        std::vector<std::vector<float>> cost(virtualN, std::vector<float>(NUM_NODES, 0.0f));
        std::vector<std::vector<int>> best_next(virtualN, std::vector<int>(NUM_NODES, NUM_NODES / 2));

        for (int stage = virtualN - 3; stage >= 0; --stage) {
            for (int curr_node = 0; curr_node < NUM_NODES; ++curr_node) {
                float min_cost = 1e9f; 
                int best_node_idx = 0;

                for (int next_node = 0; next_node < NUM_NODES; ++next_node) {
                    
                    Vector2D A = grid[stage][curr_node];
                    Vector2D B = grid[stage + 1][next_node];
                    int next_next_node = best_next[stage + 1][next_node];
                    Vector2D C = grid[stage + 2][next_next_node];

                    Vector2D dir1 = B - A;
                    Vector2D dir2 = C - B;

                    float dist = dir1.magnitude();

                    Vector2D n1 = dir1.normalized();
                    Vector2D n2 = dir2.normalized();
                    
                    float dotProduct = std::max(-1.0f, std::min(1.0f, n1.dot(n2))); 
                    float angleChange = std::acos(dotProduct); 

                    float step_cost = dist + (angleChange * angleChange * TURN_PENALTY);
                    float total_cost = step_cost + cost[stage + 1][next_node];

                    if (total_cost < min_cost) {
                        min_cost = total_cost;
                        best_node_idx = next_node;
                    }
                }
                cost[stage][curr_node] = min_cost;
                best_next[stage][curr_node] = best_node_idx;
            }
        }

        std::vector<Vector2D> optimalLineOutput;
        int current_best_node = 0;
        float best_start_cost = 1e9f;
        for (int j = 0; j < NUM_NODES; ++j) {
            if (cost[0][j] < best_start_cost) {
                best_start_cost = cost[0][j];
                current_best_node = j;
            }
        }

        for (int stage = 0; stage < N; ++stage) {
            optimalLineOutput.push_back(grid[stage][current_best_node]);
            current_best_node = best_next[stage][current_best_node]; 
        }

        return optimalLineOutput;
    }
};

REGISTER_BOT(IgorBot)