#include <iostream>
#include <vector>

#include "Core/Renderer.h"
#include "Core/TrackLoader.h"
#include "Shared/Telemetry.h"

// W środowiskach z SDL2, main czasami musi przyjmować standardowe argumenty
int main(int argc, char* argv[]) {
    std::cout << "--- TEST RENDEROWANIA TORU ---" << std::endl;

    // 1. Powołanie głównych instancji
    Renderer renderer;
    TrackLoader trackLoader;

    // 2. Inicjalizacja okna SDL2 (Rozdzielczość zgodna z naszym generatorem: 1280x720, VSync=true)
    if (!renderer.init("Racing Sim 2D - Test Toru", 1280, 720, true)) {
        std::cerr << "BLAD: Nie udalo sie zainicjowac biblioteki SDL2!" << std::endl;
        return -1;
    }

    // 3. Wczytanie danych toru
    std::cout << "Wczytywanie toru z pliku assets/test_track.txt..." << std::endl;
    TrackInfo trackData = trackLoader.loadFromTxt("assets/test_track.txt");

    if (trackData.optimalRacingLine.empty()) {
        std::cerr << "BLAD: Brak danych toru. Czy na pewno wygenerowales test_track.txt?" << std::endl;
        return -1;
    }

    std::cout << "Tor wczytany pomyslnie! Liczba punktow kontrolnych: " 
              << trackData.optimalRacingLine.size() << std::endl;

    // Pusty wektor - w tym teście nie interesują nas samochody
    std::vector<CarState> emptyCars;

    // 4. Główna pętla programu
    bool isRunning = true;
    while (isRunning) {
        
        // Sprawdzamy, czy użytkownik kliknął X w prawym górnym rogu
        if (renderer.pollEvents() == false) {
            isRunning = false;
        }

        // Zlecenie wyrenderowania klatki
        // Funkcja wewnątrz wyczyści ekran na zielono, narysuje tor i poczeka na VSync
        renderer.drawFrame(emptyCars, trackData);
    }

    std::cout << "Zamykanie testu grafiki..." << std::endl;
    return 0;
}