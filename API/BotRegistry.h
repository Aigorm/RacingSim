#pragma once
#include <vector>
#include <memory>
#include "IBot.h" // interfejs z czysto wirtualną funkcją on_tick

class BotRegistry {
public:
    // Zwraca referencję do statycznej listy botów
    static std::vector<IBot*>& getBots() {
        static std::vector<IBot*> bots;
        return bots;
    }

    // Funkcja dodająca bota do listy
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