#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class IgorBot : public IBot {
public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        std::cout << "[IgorBot] Gaz do dechy." << std::endl;
        out.throttle = 1.0f;
    }

    std::string getName() const override {
        return "IgorBot";
    }
};

REGISTER_BOT(IgorBot)