#pragma once
#include <vector>
#include <cstdint>
#include "Vector2D.h"

// 1. Definicja kolorów
struct ColorRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    ColorRGB(uint8_t red = 255, uint8_t green = 255, uint8_t blue = 255) 
        : r(red), g(green), b(blue) {}
};

// 2. Wejście z bota do silnika (Tylko jedna definicja!)
struct ControlOutput {
    float throttle = 0.0f; // od -1.0 (wsteczny) do 1.0 (gaz)
    float brake = 0.0f;    // od 0.0 do 1.0 (hamulec)
    float steeringAngle = 0.0f; 
};

// 3. Enumy
enum class TireCompound { Soft, Medium, Hard };

// 4. Stan pojedynczego auta
struct CarState {
    int id;
    Vector2D position;
    Vector2D velocity;
    float rotationAngle;
    
    ColorRGB color; 
    
    TireCompound currentTires;
    float tireDegradation;
};

// 5. Informacje o torze
struct TrackInfo {
    std::vector<Vector2D> innerBoundaries;
    std::vector<Vector2D> outerBoundaries;
    std::vector<Vector2D> optimalRacingLine; 
};

// 6. Pełna telemetria dla botów
struct Telemetry {
    CarState myCar;
    std::vector<CarState> opponents;
    TrackInfo track;
    float currentLapTime;
    int lapsRemaining;
};