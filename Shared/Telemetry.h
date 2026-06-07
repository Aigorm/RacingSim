#pragma once
#include <vector>
#include "Vector2D.h"

// Co bot chce zrobić w danej klatce? (Wysyłane DO silnika fizycznego)
struct ControlOutput {
    // Od -1.0 (pełen wsteczny) do 1.0 (gaz w podłogę)
    float throttle = 0.0f; 
    
    // Od 0.0 (brak hamowania) do 1.0 (maksymalny wcisk hamulca)
    float brake = 0.0f;

    // np. -1.0 (maksymalnie w lewo), 1.0 (maksymalnie w prawo)
    float steeringAngle = 0.0f;
    
    ControlOutput() : throttle(0.0f), steeringAngle(0.0f) {}
};

enum class TireCompound { Soft, Medium, Hard };

// Stan fizyczny jednego auta (Odbierane OD silnika fizycznego)
struct CarState {
    int id;
    Vector2D position;
    Vector2D velocity;
    float rotationAngle; // Zwrót auta w radianach lub stopniach

    ColorRGB color;
    
    TireCompound currentTires;
    float tireDegradation; // 0.0 - 1.0
};

// Informacje o torze
struct TrackInfo {
    std::vector<Vector2D> innerBoundaries;
    std::vector<Vector2D> outerBoundaries;
    std::vector<Vector2D> optimalRacingLine; 
};

// Pełen "spersonalizowany" pakiet wiedzy dla bota w danej klatce
struct Telemetry {
    CarState myCar;
    std::vector<CarState> opponents;
    TrackInfo track;
    float currentLapTime;
    int lapsRemaining;
};

struct ColorRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    ColorRGB(uint8_t red = 255, uint8_t green = 255, uint8_t blue = 255) 
        : r(red), g(green), b(blue) {}
};

struct ControlOutput {
    // Od -1.0 (pełen wsteczny) do 1.0 (gaz w podłogę)
    float throttle = 0.0f; 
    
    // Od 0.0 (brak hamowania) do 1.0 (maksymalny wcisk hamulca)
    float brake = 0.0f;    
    
    // Od -1.0 (maksymalnie w lewo) do 1.0 (maksymalnie w prawo)
    float steeringAngle = 0.0f; 
};