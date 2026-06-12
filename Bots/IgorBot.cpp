#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace {
    constexpr float WAYPOINT_REACHED_DIST = 20.0f; 
    constexpr float STEERING_P_GAIN = 3.0f;        
    constexpr float STEERING_SMOOTHING = 0.15f;    
    constexpr float STEERING_DEADZONE = 0.03f;     

    constexpr int   SPEED_LOOKAHEAD_POINTS = 8;    
    constexpr float MAX_STRAIGHT_SPEED = 220.0f;   
    constexpr float MIN_CORNER_SPEED = 70.0f;      
    constexpr float HARD_CORNER_ANGLE = 1.2f;      

    constexpr float THROTTLE_P_GAIN = 0.05f;       
    constexpr float BRAKE_P_GAIN = 0.1f;           

    constexpr float DP_CAR_WIDTH = 12.0f;       
    constexpr int   DP_NUM_NODES = 11;            
    constexpr float DP_TURN_PENALTY = 200.0f;
    constexpr float PI = 3.14159265f;
}

class IgorBot : public IBot {
private:
    bool lineCalculated = false;
    std::vector<Vector2D> optimalLine;
    int currentWaypointIndex = 0;
    float currentSteeringOutput = 0.0f;

    float normalizeAngle(float angle) const {
        while (angle > PI)  angle -= 2.0f * PI;
        while (angle < -PI) angle += 2.0f * PI;
        return angle;
    }

public:
    std::string getName() const override { return "IgorBot"; }
    ColorRGB getColor() const override { return ColorRGB(220, 20, 60); } 

    void on_tick(const Telemetry& data, ControlOutput& out) override {
        if (!lineCalculated) {
            initializeRoute(data);
        }

        if (optimalLine.empty()) return;

        updateWaypointProgress(data.myCar.position);
        
        out.steeringAngle = calculateSteering(data.myCar);
        applySpeedControl(data.myCar, out);
    }

private:
    void initializeRoute(const Telemetry& data) {
        optimalLine = calculateOptimalRacingLine(data.track);
        lineCalculated = true;

        float minDistSq = 1e9f;
        for (size_t i = 0; i < optimalLine.size(); ++i) {
            float distSq = (optimalLine[i] - data.myCar.position).magnitudeSquared();
            if (distSq < minDistSq) {
                minDistSq = distSq;
                currentWaypointIndex = i;
            }
        }
    }

    void updateWaypointProgress(const Vector2D& carPosition) {
        Vector2D toTarget = optimalLine[currentWaypointIndex] - carPosition;
        if (toTarget.magnitude() < WAYPOINT_REACHED_DIST) {
            currentWaypointIndex = (currentWaypointIndex + 1) % optimalLine.size();
        }
    }

    float calculateSteering(const CarState& car) {
        Vector2D toTarget = optimalLine[currentWaypointIndex] - car.position;
        float desiredAngle = std::atan2(toTarget.y, toTarget.x);
        float steeringError = normalizeAngle(desiredAngle - car.rotationAngle);

        if (std::abs(steeringError) < STEERING_DEADZONE) {
            steeringError = 0.0f;
        }

        float targetSteering = std::clamp(steeringError * STEERING_P_GAIN, -1.0f, 1.0f);
        
        currentSteeringOutput += (targetSteering - currentSteeringOutput) * STEERING_SMOOTHING;
        return currentSteeringOutput;
    }

    void applySpeedControl(const CarState& car, ControlOutput& out) const {
        int futureIndex = (currentWaypointIndex + SPEED_LOOKAHEAD_POINTS) % optimalLine.size();
        Vector2D toFuture = optimalLine[futureIndex] - car.position;
        
        float futureDesiredAngle = std::atan2(toFuture.y, toFuture.x);
        float futureAngleDiff = std::abs(normalizeAngle(futureDesiredAngle - car.rotationAngle));

        float cornerSharpness = std::min(futureAngleDiff / HARD_CORNER_ANGLE, 1.0f);
        float targetSpeed = MAX_STRAIGHT_SPEED - (cornerSharpness * (MAX_STRAIGHT_SPEED - MIN_CORNER_SPEED));
        
        float speedError = targetSpeed - car.velocity.magnitude();

        if (speedError > 0.0f) {
            out.throttle = std::clamp(speedError * THROTTLE_P_GAIN, 0.0f, 1.0f);
            out.brake = 0.0f;
        } else {
            out.throttle = 0.0f;
            out.brake = std::clamp(std::abs(speedError) * BRAKE_P_GAIN, 0.0f, 1.0f);
        }
    }

    std::vector<Vector2D> calculateOptimalRacingLine(const TrackInfo& track) const {
        int realNodeCount = track.innerBoundaries.size();
        if (realNodeCount < 3) return {}; 

        int virtualNodeCount = realNodeCount * 2; 

        auto grid = generateSearchGrid(track, virtualNodeCount, realNodeCount);
        
        std::vector<std::vector<float>> cost(virtualNodeCount, std::vector<float>(DP_NUM_NODES, 0.0f));
        std::vector<std::vector<int>> best_next(virtualNodeCount, std::vector<int>(DP_NUM_NODES, DP_NUM_NODES / 2));
        
        computeDPCosts(grid, virtualNodeCount, cost, best_next);

        return reconstructOptimalPath(grid, cost, best_next, realNodeCount);
    }

    std::vector<std::vector<Vector2D>> generateSearchGrid(const TrackInfo& track, int virtualNodes, int realNodes) const {
        std::vector<std::vector<Vector2D>> grid(virtualNodes, std::vector<Vector2D>(DP_NUM_NODES));

        for (int i = 0; i < virtualNodes; ++i) {
            int real_i = i % realNodes; 
            
            Vector2D inner = track.innerBoundaries[real_i];
            Vector2D outer = track.outerBoundaries[real_i];
            
            Vector2D dirNorm = (outer - inner).normalized();
            Vector2D safeInner = inner + (dirNorm * DP_CAR_WIDTH);
            Vector2D safeOuter = outer - (dirNorm * DP_CAR_WIDTH);

            for (int j = 0; j < DP_NUM_NODES; ++j) {
                float t = static_cast<float>(j) / (DP_NUM_NODES - 1);
                grid[i][j] = safeInner + ((safeOuter - safeInner) * t);
            }
        }
        return grid;
    }

    void computeDPCosts(const std::vector<std::vector<Vector2D>>& grid, int virtualNodes, 
                        std::vector<std::vector<float>>& cost, std::vector<std::vector<int>>& best_next) const {
        for (int stage = virtualNodes - 3; stage >= 0; --stage) {
            for (int curr_node = 0; curr_node < DP_NUM_NODES; ++curr_node) {
                float min_cost = 1e9f; 
                int best_node_idx = 0;

                for (int next_node = 0; next_node < DP_NUM_NODES; ++next_node) {
                    Vector2D A = grid[stage][curr_node];
                    Vector2D B = grid[stage + 1][next_node];
                    Vector2D C = grid[stage + 2][best_next[stage + 1][next_node]];

                    Vector2D dir1 = B - A;
                    Vector2D dir2 = C - B;

                    float dist = dir1.magnitude();
                    float dotProduct = std::clamp(dir1.normalized().dot(dir2.normalized()), -1.0f, 1.0f); 
                    float angleChange = std::acos(dotProduct); 

                    float step_cost = dist + (angleChange * angleChange * DP_TURN_PENALTY);
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
    }

    std::vector<Vector2D> reconstructOptimalPath(const std::vector<std::vector<Vector2D>>& grid, 
                                                 const std::vector<std::vector<float>>& cost, 
                                                 const std::vector<std::vector<int>>& best_next, 
                                                 int realNodes) const {
        std::vector<Vector2D> optimalLineOutput;
        int current_best_node = 0;
        float best_start_cost = 1e9f;
        
        for (int j = 0; j < DP_NUM_NODES; ++j) {
            if (cost[0][j] < best_start_cost) {
                best_start_cost = cost[0][j];
                current_best_node = j;
            }
        }

        for (int stage = 0; stage < realNodes; ++stage) {
            optimalLineOutput.push_back(grid[stage][current_best_node]);
            current_best_node = best_next[stage][current_best_node]; 
        }

        return optimalLineOutput;
    }
};

REGISTER_BOT(IgorBot);