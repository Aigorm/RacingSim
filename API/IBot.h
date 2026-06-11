#pragma once
#include "../Shared/Telemetry.h"
#include <string>

class IBot {
public:
    virtual ~IBot() = default;

    virtual void on_tick(const Telemetry& data, ControlOutput& out) = 0;

    virtual std::string getName() const = 0; 
    virtual ColorRGB getColor() const = 0;
};