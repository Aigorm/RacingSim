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
    std::cout << "Wczytywanie toru z pliku SVG..." << std::endl;
    // TrackInfo trackData = trackLoader.loadFromSvg("assets/track1.svg"); // TODO SWAP WITH THE LINE BELOW
    TrackInfo trackData = trackLoader.loadFromTxt("assets/test_track.txt");
    physics.setTrack(trackData);
    
    // 3. Rejestracja botów i przygotowanie aut
    auto& bots = BotRegistry::getBots(); 
    std::cout << "Zarejestrowano botów: " << bots.size() << std::endl;
    
    if (bots.empty()) {
        std::cerr << "Brak zarejestrowanych botów. Zakończenie programu." << std::endl;
        return 0;
    }

    // Fizyka musi stworzyć odpowiednią liczbę pojazdów
    physics.initCars(bots.size());

    // 4. Przygotowanie stopera do mierzenia deltaTime
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    // 5. Główna pętla symulacji
    bool isRunning = true;
    while (isRunning) {
        // A. Odmierzanie deltaTime (stoper)
        auto currentTime = clock::now();
        std::chrono::duration<float> elapsedTime = currentTime - lastTime;
        lastTime = currentTime;
        float deltaTime = elapsedTime.count();

        // B. Sprawdzenie, czy użytkownik nie zamknął okna (obsługa zdarzeń)
        if (renderer.pollEvents() == false) {
            isRunning = false;
            break;
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

    std::cout << "Zamykanie symulatora..." << std::endl;
    return 0;
}