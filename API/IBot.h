#pragma once
#include "../Shared/Telemetry.h"
#include <string>

class IBot {
public:
    // Wirtualny destruktor
    virtual ~IBot() = default;

    // Metoda, która musi zostać napisana przez gracza
    virtual void on_tick(const Telemetry& data, ControlOutput& out) = 0;

    // funkcja zwracająca nazwę bota do statystyk i GUI
    virtual std::string getName() const = 0; 
    virtual ColorRGB getColor() const = 0;
};