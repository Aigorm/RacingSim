#pragma once

struct Vector2D {
    float x;
    float y;

    // Przydatne konstruktory
    Vector2D() : x(0.0f), y(0.0f) {}
    Vector2D(float x, float y) : x(x), y(y) {}

    // TODO: przeciążania operatorów (+, -, *, /)
    // oraz metody matematyczne np. długość wektora (magnitude), normalizację itp.
};