#pragma once
#include <vector>
#include <cstdint>
#include "Vector2D.h"

struct ColorRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    ColorRGB(uint8_t red = 255, uint8_t green = 255, uint8_t blue = 255) 
        : r(red), g(green), b(blue) {}
};

struct ControlOutput {
    float throttle = 0.0f; 
    float brake = 0.0f;    
    float steeringAngle = 0.0f; 
};

enum class TireCompound { Soft, Medium, Hard };

struct CarState {
    int id;
    Vector2D position;
    Vector2D velocity;
    float rotationAngle;
    
    ColorRGB color; 
    
    TireCompound currentTires;
    float tireDegradation;

    int lapsCompleted = 0;
    int nextSectorToClear = 1; 
    bool isFinished = false;
    float finishTime = 0.0f;
};

struct TrackInfo {
    std::vector<Vector2D> innerBoundaries;
    std::vector<Vector2D> outerBoundaries;
    std::vector<Vector2D> optimalRacingLine; 
};

struct Telemetry {
    CarState myCar;
    std::vector<CarState> opponents;
    TrackInfo track;
    float currentLapTime;
    int lapsRemaining;
};