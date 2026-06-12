#include <cmath>
#include <algorithm>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

namespace {
    constexpr float REACH_DIST = 50.0f;
    constexpr float STEER_GAIN = 2.5f;
    constexpr float PI = 3.14159265f;
}

class BlagojaBot : public IBot {
private:
    int targetIndex = 0;

    float normalizeAngle(float a) const {
        while (a > PI)  a -= 2.0f * PI;
        while (a < -PI) a += 2.0f * PI;
        return a;
    }

public:
    std::string getName() const override { return "BlagojaBot"; }
    ColorRGB getColor() const override { return ColorRGB(20, 20, 220); }

    void on_tick(const Telemetry& data, ControlOutput& out) override {
        const auto& line = data.track.optimalRacingLine;
        if (line.empty()) return;

        Vector2D target = line[targetIndex];
        if ((target - data.myCar.position).magnitudeSquared() < REACH_DIST * REACH_DIST)
            targetIndex = (targetIndex + 1) % line.size();

        Vector2D toTarget = target - data.myCar.position;
        float desired = std::atan2(toTarget.y, toTarget.x);
        float diff = normalizeAngle(desired - data.myCar.rotationAngle);

        out.steeringAngle = std::clamp(diff * STEER_GAIN, -1.0f, 1.0f);
        out.throttle = std::abs(diff) > 0.5f ? 0.5f : 0.85f;
        out.brake = 0.0f;
    }
};

REGISTER_BOT(BlagojaBot);
