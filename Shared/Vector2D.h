#pragma once
#include <cmath>

struct Vector2D {
    float x;
    float y;

    Vector2D() : x(0.0f), y(0.0f) {}
    Vector2D(float x, float y) : x(x), y(y) {}

    Vector2D operator+(const Vector2D& o) const { return {x + o.x, y + o.y}; }
    Vector2D operator-(const Vector2D& o) const { return {x - o.x, y - o.y}; }
    Vector2D operator*(float s) const { return {x * s, y * s}; }
    Vector2D operator/(float s) const { return {x / s, y / s}; }

    Vector2D& operator+=(const Vector2D& o) { x += o.x; y += o.y; return *this; }
    Vector2D& operator-=(const Vector2D& o) { x -= o.x; y -= o.y; return *this; }
    Vector2D& operator*=(float s) { x *= s; y *= s; return *this; }

    friend Vector2D operator*(float s, const Vector2D& v) { return {s * v.x, s * v.y}; }

    float magnitude() const { return std::sqrt(x * x + y * y); }
    float magnitudeSquared() const { return x * x + y * y; }

    Vector2D normalized() const {
        float m = magnitude();
        if (m < 1e-6f) return {0.0f, 0.0f};
        return {x / m, y / m};
    }

    float dot(const Vector2D& o) const { return x * o.x + y * o.y; }
    float cross(const Vector2D& o) const { return x * o.y - y * o.x; }
};