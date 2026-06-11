#pragma once
#include <vector>
#include "../Shared/Telemetry.h"
#include "../Shared/Vector2D.h"

class PhysicsEngine {
private:
    std::vector<CarState> cars;
    TrackInfo currentTrack;

    bool carCarCollisionsEnabled = false;

    float totalRaceTime = 0.0f;
    int targetLaps = 3;            
    std::vector<int> raceRanking;  

    float getGripCoefficient(TireCompound compound) const;
    float getDegradationRate(TireCompound compound) const;

    void applyForces(float deltaTime, const std::vector<ControlOutput>& inputs);
    void integrateCarMotion(CarState& car, const ControlOutput& input, float deltaTime);
    float computeMaxLateralForce(const CarState& car, float speed) const;
    Vector2D resolveLateralTireForce(CarState& car, const Vector2D& lateral,
                                     float maxLateralForce, float deltaTime);
    void updatePositions(float deltaTime);

    void resolveCollisions();
    void resolveBoundaryCollision(CarState& car, const std::vector<Vector2D>& boundary);
    void resolveCarCarCollisions();

    void updateRaceProgress(float deltaTime);
    void checkCarCheckpoint(CarState& car, float deltaTime);

    static Vector2D closestPointOnSegment(const Vector2D& p, const Vector2D& a, const Vector2D& b);

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

    void setTargetLaps(int laps) { targetLaps = laps; }
    bool isRaceFinished() const { return raceRanking.size() == cars.size() && !cars.empty(); }
    const std::vector<int>& getRanking() const { return raceRanking; }
    float getTotalTime() const { return totalRaceTime; }
};