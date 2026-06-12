#include <iostream>
#include <cmath>
#include <algorithm>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

namespace {
    constexpr float WAYPOINT_REACH_DISTANCE = 40.0f;
    constexpr float CRUISE_THROTTLE = 0.7f;
    constexpr float STEERING_P_GAIN = 2.0f;
    constexpr float PI = 3.14159265f;
}

class LineFollowerBot : public IBot {
private:
    int currentTargetIndex = 0;

    void updateWaypointProgress(const Vector2D& carPos, const std::vector<Vector2D>& line) {
        Vector2D target = line[currentTargetIndex];
        
        float distSq = (target - carPos).magnitudeSquared();
        float reachDistSq = WAYPOINT_REACH_DISTANCE * WAYPOINT_REACH_DISTANCE;
        
        if (distSq < reachDistSq) {
            currentTargetIndex = (currentTargetIndex + 1) % line.size();
        }
    }

    float normalizeAngle(float angle) const {
        while (angle > PI)  angle -= 2.0f * PI;
        while (angle < -PI) angle += 2.0f * PI;
        return angle;
    }

    float calculateSteering(const CarState& car, const Vector2D& target) const {
        Vector2D toTarget = target - car.position;
        float desiredAngle = std::atan2(toTarget.y, toTarget.x);
        float angleDiff = normalizeAngle(desiredAngle - car.rotationAngle);
        
        return std::clamp(angleDiff * STEERING_P_GAIN, -1.0f, 1.0f);
    }

    void applySpeedControl(ControlOutput& out) const {
        out.throttle = CRUISE_THROTTLE; 
        out.brake = 0.0f;
    }

public:
    std::string getName() const override { return "Line Follower"; }
    ColorRGB getColor() const override { return ColorRGB(0, 255, 0); } // Solid Green

    void on_tick(const Telemetry& data, ControlOutput& out) override {
        const auto& line = data.track.optimalRacingLine;
        if (line.empty()) return;

        updateWaypointProgress(data.myCar.position, line);

        Vector2D target = line[currentTargetIndex];
        out.steeringAngle = calculateSteering(data.myCar, target);
        applySpeedControl(out);
    }
};

REGISTER_BOT(LineFollowerBot);