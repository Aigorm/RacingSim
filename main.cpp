#include <iostream>
#include <chrono>
#include <vector>

#include "Core/PhysicsEngine.h"
#include "Core/Renderer.h"
#include "Core/TrackLoader.h"
#include "API/BotRegistry.h"
#include "Shared/Telemetry.h"

int main() {
    std::cout << "Uruchamianie 2D Auto Racing Simulator..." << std::endl;

    float TimeSpeedupMultiplier = 4.0;

    // 1. Inicjalizacja głównych komponentów
    Renderer renderer;
    TrackLoader trackLoader;
    PhysicsEngine physics;
    
    // Konfiguracja Renderera (SDL2) - włączenie VSync
    if (!renderer.init("Racing Sim 2D", 1280, 720, true)) {
        std::cerr << "Błąd inicjalizacji grafiki!" << std::endl;
        return -1;
    }

    // 2. Wczytanie mapy i przekazanie jej do fizyki
    std::cout << "Wczytywanie pliku toru..." << std::endl;
    TrackInfo trackData = trackLoader.loadFromTxt("assets/Track_1.txt");
    physics.setTrack(trackData);
    
    // 3. Rejestracja botów i przygotowanie aut
    auto& bots = BotRegistry::getBots(); 
    std::cout << "Zarejestrowano botów: " << bots.size() << std::endl;
    
    if (bots.empty()) {
        std::cerr << "Brak zarejestrowanych botów. Zakończenie programu." << std::endl;
        return 0;
    }

    physics.initCars(bots.size());
    physics.setTargetLaps(3);

    // 4. Przygotowanie stopera do mierzenia deltaTime
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    // 5. Główna pętla symulacji
    bool isRunning = true;
    while (isRunning && !physics.isRaceFinished()) {
        // A. Odmierzanie deltaTime (stoper)
        auto currentTime = clock::now();
        std::chrono::duration<float> elapsedTime = currentTime - lastTime;
        lastTime = currentTime;
        float deltaTime = (elapsedTime.count()*TimeSpeedupMultiplier);

        // B. Sprawdzenie, czy użytkownik nie zamknął okna (obsługa zdarzeń)
        if (renderer.pollEvents() == false) {
            isRunning = false;
            break;
        }

        if (renderer.pollEvents() == false) {
            isRunning = false;
        }

        // C. Zbieranie decyzji od botów
        std::vector<ControlOutput> botInputs;
        botInputs.reserve(bots.size());

        for (size_t i = 0; i < bots.size(); ++i) {
            // Fizyka generuje telemetrię z perspektywy konkretnego auta (id = i)
            Telemetry currentTelemetry = physics.getTelemetryForBot(i);
            
            ControlOutput output;
            // Odpytujemy bota gracza o decyzję w tej klatce
            bots[i]->on_tick(currentTelemetry, output); 
            botInputs.push_back(output);
        }
        
        // D. Aktualizacja świata (Fizyka)
        // Silnik fizyczny oblicza nowe pozycje na podstawie wejścia i upływu czasu
        physics.update(deltaTime, botInputs);
        
        // E. Renderowanie klatki
        renderer.drawFrame(physics.getCarStates(), trackData);
    }

    // --- CEREMONIA ZAKOŃCZENIA (Podium) ---
    std::cout << "\n===================================" << std::endl;
    std::cout << "         WYSCIG ZAKONCZONY!        " << std::endl;
    std::cout << "===================================" << std::endl;
    
    auto ranking = physics.getRanking();

    for (size_t pos = 0; pos < ranking.size(); ++pos) {
        int carId = ranking[pos];
        // Ponieważ przechowujemy ID jako pozycję w wektorze, mamy łatwy dostęp do bota:
        std::string botName = (carId < bots.size()) ? bots[carId]->getName() : "Nieznany Bot";
        
        std::cout << "Miejsce " << (pos + 1) << ": " << botName 
                  << " (Auto #" << carId << ")" << std::endl;
    }
    std::cout << "===================================\n" << std::endl;

    std::cout << "Zamykanie symulatora..." << std::endl;
    return 0;
}