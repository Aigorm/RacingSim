#include <iostream>
#include <cmath>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class LineFollowerBot : public IBot {
private:
    // Przechowujemy indeks punktu, do którego aktualnie zmierzamy
    int currentTargetIndex = 0; 
    
    // Ustawiamy bezpieczną prędkość początkową (nie używamy pełnego gazu, bo wylecimy z zakrętu)
    const float CRUISE_THROTTLE = 0.7f;

public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        const auto& line = data.track.optimalRacingLine;
        
        // Zabezpieczenie przed brakiem toru
        if (line.empty()) return;

        Vector2D carPos = data.myCar.position;
        Vector2D target = line[currentTargetIndex];
        
        // 1. Obliczamy wektor do celu i dystans
        float dx = target.x - carPos.x;
        float dy = target.y - carPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        // 2. Jeśli jesteśmy blisko (np. mniej niż 40 pikseli), bierzemy następny punkt
        if (dist < 40.0f) {
            currentTargetIndex = (currentTargetIndex + 1) % line.size();
            
            // Aktualizujemy wektory dla nowego celu
            target = line[currentTargetIndex];
            dx = target.x - carPos.x;
            dy = target.y - carPos.y;
        }
        
        // 3. Obliczamy pożądany kąt za pomocą atan2 (kąt między osią X a wektorem do celu)
        float desiredAngle = std::atan2(dy, dx);
        float currentAngle = data.myCar.rotationAngle;
        
        // 4. Różnica kątów
        float angleDiff = desiredAngle - currentAngle;
        
        const float PI = 3.14159265f;
        while (angleDiff > PI)  angleDiff -= 2.0f * PI;
        while (angleDiff < -PI) angleDiff += 2.0f * PI;
        
        // 5. Sterowanie proporcjonalne (im większy błąd, tym mocniej kręcimy kierownicą)
        // Mnożnik ustala czułość reakcji
        out.steeringAngle = angleDiff * 2.0f; 
        
        // Ucinamy wartości, żeby nie przekroczyły mechanicznych możliwości auta [-1.0, 1.0]
        if (out.steeringAngle > 1.0f) out.steeringAngle = 1.0f;
        if (out.steeringAngle < -1.0f) out.steeringAngle = -1.0f;
        
        // 6. Przepustnica i hamulec
        out.throttle = CRUISE_THROTTLE; 
        out.brake = 0.0f;
    }

    std::string getName() const override {
        return "Line Follower";
    }

    ColorRGB getColor() const override {
        return ColorRGB(0, 255, 0); 
    }
};

REGISTER_BOT(LineFollowerBot)