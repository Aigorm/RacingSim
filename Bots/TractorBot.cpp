#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class TractorBot : public IBot {
public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        std::cout << "[TractorBot] Pyr pyr pyr... jade powoli do przodu." << std::endl;
        out.throttle = 0.3f;
    }

    std::string getName() const override {
        return "TractorBot (Test)";
    }

    ColorRGB getColor() const override {
        return ColorRGB(220, 220, 260); 
    }
};

REGISTER_BOT(TractorBot)