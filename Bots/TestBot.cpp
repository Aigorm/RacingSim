#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class TestBot : public IBot {
public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        // Tutaj logika wyliczająca optymalny tor jazdy i prędkość
        out.throttle = 1.0f; // Gaz do dechy
        out.steeringAngle = 0.0f;
    }

    std::string getName() const override {
        return "TestBot";
    }

    ColorRGB getColor() const override {
        return ColorRGB(20, 220, 60); 
    }
};

// Ta jedna linijka załatwia całą magię!
REGISTER_BOT(TestBot)