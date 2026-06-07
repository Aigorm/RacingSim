#include <iostream>
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class BlagojaBot : public IBot {
public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        std::cout << "[BlagojaBot] Analizuje telemetrie. Trzymam sie optymalnej linii." << std::endl;
        out.throttle = 0.8f;
    }

    std::string getName() const override {
        return "BlagojaBot";
    }
};

REGISTER_BOT(BlagojaBot)