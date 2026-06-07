#include <iostream>
#include <vector>
#include "API/BotRegistry.h"
#include "Shared/Telemetry.h"

int main() {
    std::cout << "--- TESTOWANIE REJESTRACJI BOTOW ---" << std::endl;

    // 1. Pobranie listy zarejestrowanych botów
    auto& bots = BotRegistry::getBots();

    std::cout << "Znaleziono botow: " << bots.size() << std::endl;
    std::cout << "------------------------------------" << std::endl;

    if (bots.empty()) {
        std::cerr << "BLAD: Nie znaleziono zadnych botow!" << std::endl;
        return 1;
    }

    // Puste dane testowe
    Telemetry dummyTelemetry;

    // 2. Symulacja kilku klatek gry
    for (int frame = 1; frame <= 3; ++frame) {
        std::cout << "\nKLATKA SYMULACJI: #" << frame << std::endl;
        
        for (auto* bot : bots) {
            ControlOutput output;
            
            // Wywołanie logiki bota
            bot->on_tick(dummyTelemetry, output);
            
            // Sprawdzenie, czy bot zapisał dane do referencji
            std::cout << " -> " << bot->getName() 
                      << " zwrocil przepustnice (throttle): " << output.throttle 
                      << std::endl;
        }
    }

    std::cout << "\n------------------------------------" << std::endl;
    std::cout << "Zwalnianie pamieci..." << std::endl;
    
    // Ręczne sprzątanie zaalokowanych botów (ponieważ użyliśmy 'new' w makrze)
    for (auto* bot : bots) {
        delete bot;
    }
    bots.clear();

    std::cout << "Test zakonczony sukcesem!" << std::endl;
    return 0;
}