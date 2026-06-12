#pragma once
#include <vector>
#include <memory>
#include "IBot.h"

class BotRegistry {
public:
    static std::vector<IBot*>& getBots() {
        static std::vector<IBot*> bots;
        return bots;
    }

    static void addBot(IBot* bot) {
        getBots().push_back(bot);
    }
};

class BotRegistrar {
public:
    BotRegistrar(IBot* bot) {
        BotRegistry::addBot(bot);
    }
};

#define REGISTER_BOT(BotClassName) \
    static BotRegistrar global_##BotClassName##_registrar(new BotClassName());