#include "PhysicsEngine.h"
#include "../API/BotRegistry.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace {
namespace cfg {
    constexpr float CAR_MASS                   = 700.0f;
    constexpr float MAX_ENGINE_FORCE           = 15000.0f;
    constexpr float MAX_BRAKE_FORCE            = 12000.0f;
    constexpr float DRAG_COEFFICIENT           = 0.4f;
    constexpr float ROLLING_RESISTANCE         = 30.0f;
    constexpr float MAX_TURN_RATE              = 5.0f;
    constexpr float MAX_STEERING_ANGLE         = 0.6f;
    constexpr float WHEELBASE                  = 2.5f;

    constexpr float AERO_DOWNFORCE_MULTIPLIER  = 0.05f;
    constexpr float MECHANICAL_GRIP_MULTIPLIER = 2.8f;
    constexpr float GRAVITY                    = 9.81f;
    constexpr float MIN_GRIP_FACTOR            = 0.3f;  

    constexpr float MIN_EFFECTIVE_SPEED        = 0.5f;  
    constexpr float MOVEMENT_THRESHOLD         = 0.1f;  
    constexpr float BRAKE_STOP_SPEED           = 0.5f;  

    constexpr float SLIDE_DEGRADATION_FACTOR   = 3.0f;  
    constexpr float BASE_LOAD                  = 0.1f;  

    constexpr float CAR_RADIUS                 = 1.5f;
    constexpr float CAR_HALF_LENGTH            = 12.0f;
    constexpr float CAR_HALF_WIDTH             = 6.0f;

    constexpr float GRID_SPACING_BACKWARD      = 30.0f; 
    constexpr float GRID_SPACING_LATERAL       = 10.0f; 

    constexpr float WALL_RESTITUTION           = 0.3f;  
    constexpr float WALL_FRICTION              = 0.6f; 
    constexpr float CAR_COLLISION_ELASTICITY   = 0.8f;

    constexpr int   NUM_SECTORS                = 20;
    constexpr float GATE_EXTENSION             = 5.0f;  
}

Vector2D headingVector(float angle) { return { std::cos(angle),  std::sin(angle) }; }
Vector2D lateralVector(float angle) { return { -std::sin(angle), std::cos(angle) }; }

bool ccw(const Vector2D& a, const Vector2D& b, const Vector2D& c) {
    return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
}
bool segmentsIntersect(const Vector2D& a, const Vector2D& b,
                       const Vector2D& c, const Vector2D& d) {
    return (ccw(a, c, d) != ccw(b, c, d)) && (ccw(a, b, c) != ccw(a, b, d));
}

} 

float PhysicsEngine::getGripCoefficient(TireCompound compound) const {
    switch (compound) {
        case TireCompound::Soft:   return 2.5f; // Zwiększone z 1.3
        case TireCompound::Medium: return 2.0f; // Zwiększone z 1.0
        case TireCompound::Hard:   return 1.6f; // Zwiększone z 0.8
    }
    return 2.0f;
}

float PhysicsEngine::getDegradationRate(TireCompound compound) const {
    switch (compound) {
        case TireCompound::Soft:   return 0.001f;
        case TireCompound::Medium: return 0.0006f;
        case TireCompound::Hard:   return 0.0003f;
    }
    return 0.002f;
}

Vector2D PhysicsEngine::closestPointOnSegment(const Vector2D& p, const Vector2D& a, const Vector2D& b) {
    Vector2D ab = b - a;
    float lenSq = ab.magnitudeSquared();
    if (lenSq < 1e-10f) return a;
    float t = std::clamp((p - a).dot(ab) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}


void PhysicsEngine::initCars(int numberOfCars) {
    cars.clear();
    auto& bots = BotRegistry::getBots();

    if (currentTrack.optimalRacingLine.size() < 2) {
        std::cerr << "Blad: Silnik fizyczny probuje ustawic auta, ale tor nie zostal wczytany!" << std::endl;
        return;
    }

    // wyznaczamy kierunek jazdy.
    const Vector2D p0 = currentTrack.optimalRacingLine[0];
    const Vector2D p1 = currentTrack.optimalRacingLine[1];

    Vector2D tangent = (p1 - p0);
    if (tangent.magnitude() > 0.0f) {
        tangent = tangent.normalized();
    } else {
        tangent = {1.0f, 0.0f}; // w prawo w razie błędu
    }

    const float startRotation = std::atan2(tangent.y, tangent.x);
    const Vector2D normal(-tangent.y, tangent.x); // przesuwa auta w bok

    for (int i = 0; i < numberOfCars; ++i) {
        CarState car;
        car.id = i;

        const float lateralMultiplier = (i % 2 == 0) ? -1.0f : 1.0f;

        car.position = p0
                     - tangent * (static_cast<float>(i) * cfg::GRID_SPACING_BACKWARD)
                     + normal  * (lateralMultiplier * cfg::GRID_SPACING_LATERAL);

        car.velocity = {0.0f, 0.0f};
        car.rotationAngle = startRotation;

        car.color = (i < static_cast<int>(bots.size()))
                  ? bots[i]->getColor()
                  : ColorRGB(255, 255, 255);

        car.currentTires = TireCompound::Medium;
        car.tireDegradation = 0.0f;

        car.lapsCompleted = 0;
        car.nextSectorToClear = 1;
        car.isFinished = false;
        car.finishTime = 0.0f;

        cars.push_back(car);
    }
}

void PhysicsEngine::setCarTires(int carId, TireCompound compound) {
    if (carId >= 0 && carId < static_cast<int>(cars.size())) {
        cars[carId].currentTires = compound;
        cars[carId].tireDegradation = 0.0f;
    }
}

void PhysicsEngine::applyForces(float deltaTime, const std::vector<ControlOutput>& inputs) {
    for (size_t i = 0; i < cars.size() && i < inputs.size(); ++i) {
        integrateCarMotion(cars[i], inputs[i], deltaTime);
    }
}

float PhysicsEngine::computeMaxLateralForce(const CarState& car, float speed) const {
    float gripBase = getGripCoefficient(car.currentTires);
    float degradationPenalty = std::sqrt(car.tireDegradation);
    float grip = gripBase * std::max(cfg::MIN_GRIP_FACTOR, 1.0f - degradationPenalty);

    float downforce = (cfg::CAR_MASS * cfg::GRAVITY * cfg::MECHANICAL_GRIP_MULTIPLIER)
                    + (cfg::AERO_DOWNFORCE_MULTIPLIER * speed * speed);
    return grip * downforce;
}

Vector2D PhysicsEngine::resolveLateralTireForce(CarState& car, const Vector2D& lateral,
                                                float maxLateralForce, float deltaTime) {
    float lateralSpeed = car.velocity.dot(lateral);
    float desiredLateralForce = -lateralSpeed * cfg::CAR_MASS / deltaTime;
    float degradationRate = getDegradationRate(car.currentTires);
    float actualLateralForce;

    if (std::abs(desiredLateralForce) > maxLateralForce) {
        float sign = (desiredLateralForce > 0.0f) ? 1.0f : -1.0f;
        actualLateralForce = sign * maxLateralForce;
        car.tireDegradation += degradationRate * cfg::SLIDE_DEGRADATION_FACTOR * deltaTime;
    } else {
        actualLateralForce = desiredLateralForce;
        float loadRatio = (maxLateralForce > 0.0f)
            ? std::abs(desiredLateralForce) / maxLateralForce : 0.0f;
        car.tireDegradation += degradationRate * (cfg::BASE_LOAD + loadRatio) * deltaTime;
    }
    car.tireDegradation = std::clamp(car.tireDegradation, 0.0f, 1.0f);

    return lateral * actualLateralForce;
}

void PhysicsEngine::integrateCarMotion(CarState& car, const ControlOutput& input, float deltaTime) {
    float throttle = std::clamp(input.throttle, -1.0f, 1.0f);
    float brake    = std::clamp(input.brake, 0.0f, 1.0f);
    float steering = std::clamp(input.steeringAngle, -1.0f, 1.0f);

    float speed = car.velocity.magnitude();

    float steerAngle = steering * cfg::MAX_STEERING_ANGLE;
    float effectiveSpeed = std::max(speed, cfg::MIN_EFFECTIVE_SPEED);
    float turnRate = effectiveSpeed * std::tan(steerAngle) / cfg::WHEELBASE;
    turnRate = std::clamp(turnRate, -cfg::MAX_TURN_RATE, cfg::MAX_TURN_RATE);
    car.rotationAngle += turnRate * deltaTime;

    Vector2D forward = headingVector(car.rotationAngle);
    Vector2D lateral = lateralVector(car.rotationAngle);

    float maxLateralForce = computeMaxLateralForce(car, speed);
    Vector2D lateralTireForce = resolveLateralTireForce(car, lateral, maxLateralForce, deltaTime);

    Vector2D engineForce = forward * (throttle * cfg::MAX_ENGINE_FORCE);

    Vector2D brakeForce(0.0f, 0.0f);
    if (speed > cfg::MOVEMENT_THRESHOLD) {
        brakeForce = car.velocity.normalized() * (-brake * cfg::MAX_BRAKE_FORCE);
    }

    Vector2D dragForce = car.velocity * (-cfg::DRAG_COEFFICIENT * speed);

    Vector2D rollingForce(0.0f, 0.0f);
    if (speed > cfg::MOVEMENT_THRESHOLD) {
        rollingForce = car.velocity.normalized() * (-cfg::ROLLING_RESISTANCE);
    }

    Vector2D totalForce = engineForce + brakeForce + dragForce + rollingForce + lateralTireForce;
    car.velocity += (totalForce / cfg::CAR_MASS) * deltaTime;

    if (brake > 0.5f && car.velocity.magnitude() < cfg::BRAKE_STOP_SPEED) {
        car.velocity = Vector2D(0.0f, 0.0f);
    }
}

void PhysicsEngine::updatePositions(float deltaTime) {
    for (auto& car : cars) {
        car.position += car.velocity * deltaTime;
    }
}


void PhysicsEngine::resolveBoundaryCollision(CarState& car, const std::vector<Vector2D>& boundary) {
    if (boundary.size() < 2) return;

    // najbliższy punkt obwodu toru względem auta.
    float minDist = 1e10f;
    Vector2D closestPt;
    auto consider = [&](const Vector2D& a, const Vector2D& b) {
        Vector2D cp = closestPointOnSegment(car.position, a, b);
        float d = (car.position - cp).magnitude();
        if (d < minDist) { minDist = d; closestPt = cp; }
    };

    for (size_t j = 0; j + 1 < boundary.size(); ++j) {
        consider(boundary[j], boundary[j + 1]);
    }
    if (boundary.size() >= 3) {
        consider(boundary.back(), boundary.front());
    }

    if (minDist <= 1e-6f) return;

    Vector2D normal = (car.position - closestPt).normalized();

    Vector2D forward = headingVector(car.rotationAngle);
    Vector2D lateral = lateralVector(car.rotationAngle);

    float effectiveRadius = cfg::CAR_HALF_LENGTH * std::abs(normal.dot(forward))
                          + cfg::CAR_HALF_WIDTH  * std::abs(normal.dot(lateral));

    if (minDist >= effectiveRadius) return;

    car.position = closestPt + normal * effectiveRadius;

    float velNormal = car.velocity.dot(normal);
    if (velNormal < 0.0f) {
        Vector2D vNormal  = normal * velNormal;     
        Vector2D vTangent = car.velocity - vNormal; 

        // auto ślizga się wzdłuż ściany
        car.velocity = vTangent * cfg::WALL_FRICTION - vNormal * cfg::WALL_RESTITUTION;
    }
}

void PhysicsEngine::resolveCarCarCollisions() {
    const float minSep = cfg::CAR_RADIUS * 2.0f;

    for (size_t i = 0; i < cars.size(); ++i) {
        for (size_t j = i + 1; j < cars.size(); ++j) {
            Vector2D diff = cars[j].position - cars[i].position;
            float dist = diff.magnitude();

            if (dist < minSep && dist > 1e-6f) {
                Vector2D normal = diff / dist;
                float overlap = minSep - dist;

                cars[i].position -= normal * (overlap * 0.5f);
                cars[j].position += normal * (overlap * 0.5f);

                float vi = cars[i].velocity.dot(normal);
                float vj = cars[j].velocity.dot(normal);

                cars[i].velocity += normal * ((vj - vi) * cfg::CAR_COLLISION_ELASTICITY);
                cars[j].velocity += normal * ((vi - vj) * cfg::CAR_COLLISION_ELASTICITY);
            }
        }
    }
}

void PhysicsEngine::resolveCollisions() {
    for (auto& car : cars) {
        resolveBoundaryCollision(car, currentTrack.outerBoundaries);
        resolveBoundaryCollision(car, currentTrack.innerBoundaries);
    }

    if (carCarCollisionsEnabled) {
        resolveCarCarCollisions();
    }
}

void PhysicsEngine::update(float deltaTime, const std::vector<ControlOutput>& inputs) {
    if (deltaTime <= 0.0f) return;

    applyForces(deltaTime, inputs);
    updatePositions(deltaTime);
    resolveCollisions();

    // sędzia
    totalRaceTime += deltaTime;
    updateRaceProgress(deltaTime);
}

void PhysicsEngine::updateRaceProgress(float deltaTime) {
    if (currentTrack.optimalRacingLine.empty()) return; 

    for (auto& car : cars) {
        if (car.isFinished) continue;
        checkCarCheckpoint(car, deltaTime);
    }
}

void PhysicsEngine::checkCarCheckpoint(CarState& car, float deltaTime) {
    const int totalPoints = static_cast<int>(currentTrack.optimalRacingLine.size());
    const int targetPointIndex = (car.nextSectorToClear * totalPoints) / cfg::NUM_SECTORS % totalPoints;

    Vector2D checkpointInner = currentTrack.innerBoundaries[targetPointIndex];
    Vector2D checkpointOuter = currentTrack.outerBoundaries[targetPointIndex];

    Vector2D gateDir = (checkpointOuter - checkpointInner).normalized();
    Vector2D cpStart = checkpointInner - gateDir * cfg::GATE_EXTENSION;
    Vector2D cpEnd   = checkpointOuter + gateDir * cfg::GATE_EXTENSION;

    Vector2D oldPos = car.position - (car.velocity * deltaTime);
    Vector2D newPos = car.position;

    if (!segmentsIntersect(oldPos, newPos, cpStart, cpEnd)) return;

    int nextTrackPointIndex = (targetPointIndex + 1) % totalPoints;
    Vector2D trackForward = (currentTrack.optimalRacingLine[nextTrackPointIndex]
                           - currentTrack.optimalRacingLine[targetPointIndex]).normalized();

    if (car.velocity.dot(trackForward) <= 0.0f) return;

    car.nextSectorToClear++;
    if (car.nextSectorToClear <= cfg::NUM_SECTORS) return;

    car.nextSectorToClear = 1;
    car.lapsCompleted++;

    std::cout << "[SEDZIA] Auto ID " << car.id << " zakonczylo okrazenie "
              << car.lapsCompleted << "/" << targetLaps << std::endl;

    if (car.lapsCompleted == targetLaps) {
        car.isFinished = true;
        car.finishTime = totalRaceTime;
        raceRanking.push_back(car.id);
        std::cout << "[SEDZIA] Auto ID " << car.id << " WPADA NA METE! Czas: "
                  << totalRaceTime << "s" << std::endl;
    }
}


const std::vector<CarState>& PhysicsEngine::getCarStates() const {
    return cars;
}

Telemetry PhysicsEngine::getTelemetryForBot(int botId) const {
    Telemetry t;
    if (botId >= 0 && botId < static_cast<int>(cars.size())) {
        t.myCar = cars[botId];
        for (size_t i = 0; i < cars.size(); ++i) {
            if (static_cast<int>(i) != botId) {
                t.opponents.push_back(cars[i]);
            }
        }
    }
    t.track = currentTrack;

    t.currentLapTime = totalRaceTime;
    t.lapsRemaining = targetLaps - t.myCar.lapsCompleted;

    return t;
}