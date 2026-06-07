#pragma once
#include <vector>
#include "../Shared/Telemetry.h"
#include "../Shared/Vector2D.h"

class PhysicsEngine {
private:
    //Wewnętrzny stan wszystkich pojazdów i toru
    std::vector<CarState> cars;
    TrackInfo currentTrack;

    // TODO: Zdefiniować stałe fizyczne (masa aut, współczynniki tarcia opon, opór powietrza)
    // float carMass = 1000.0f;
    // float dragCoefficient = 0.05f;

    // PRYWATNE FUNKCJE POMOCNICZE (Fazy obliczeń w jednej klatce)

    // Faza 1: Obliczanie wektorów sił (np. trakcja z opon) na podstawie ControlOutput
    void applyForces(float deltaTime, const std::vector<ControlOutput>& inputs);

    // Faza 2: Całkowanie numeryczne (np. metoda Eulera) - zmiana pozycji i prędkości
    void updatePositions(float deltaTime);

    // Faza 3: Wykrywanie i rozwiązywanie kolizji ( najpierw tylko kolizje z torami, potem między autami ale zróbmy je toglablle na podstawie stałej)
    void resolveCollisions();

public:
    PhysicsEngine() = default;

    // TODO: Napisać logikę ustawiającą auta na polach startowych (np. grid alignment)
    void initCars(int numberOfCars);

    // Przekazanie przetworzonych granic toru z TrackLoadera
    void setTrack(const TrackInfo& track) { currentTrack = track; }

    // Główna funkcja wywoływana z main.cpp co klatkę (np. deltaTime = 0.016f)
    void update(float deltaTime, const std::vector<ControlOutput>& inputs);

    // Przepakowanie danych dla konkretnego bota (patrz w Telemetry.h)
    Telemetry getTelemetryForBot(int botId) const;
};