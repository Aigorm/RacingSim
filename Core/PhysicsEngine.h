#pragma once
#include <vector>
#include "../Shared/Telemetry.h"
#include "../Shared/Vector2D.h"

class PhysicsEngine {
private:
    std::vector<CarState> cars;
    TrackInfo currentTrack;

    float carMass = 800.0f;
    float maxEngineForce = 8000.0f;
    float maxBrakeForce = 12000.0f;
    float dragCoefficient = 1.2f;
    float rollingResistance = 30.0f;
    float maxTurnRate = 5.0f;
    float maxSteeringAngle = 0.6f;
    float wheelbase = 2.5f;

    static constexpr float GRAVITY = 9.81f;
    static constexpr float CAR_RADIUS = 1.5f;

    bool carCarCollisionsEnabled = false;

    float getGripCoefficient(TireCompound compound) const;
    float getDegradationRate(TireCompound compound) const;
    static Vector2D closestPointOnSegment(const Vector2D& p, const Vector2D& a, const Vector2D& b);

    void applyForces(float deltaTime, const std::vector<ControlOutput>& inputs);
    void updatePositions(float deltaTime);
    void resolveCollisions();

public:
    PhysicsEngine() = default;

    void initCars(int numberOfCars);
    void setTrack(const TrackInfo& track) { currentTrack = track; }
    void update(float deltaTime, const std::vector<ControlOutput>& inputs);
    const std::vector<CarState>& getCarStates() const;
    Telemetry getTelemetryForBot(int botId) const;

    void setCarTires(int carId, TireCompound compound);
    void setCarCarCollisions(bool enabled) { carCarCollisionsEnabled = enabled; }
    bool getCarCarCollisions() const { return carCarCollisionsEnabled; }
};
