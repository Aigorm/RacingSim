#include "PhysicsEngine.h"
#include <cmath>
#include <algorithm>

float PhysicsEngine::getGripCoefficient(TireCompound compound) const {
    switch (compound) {
        case TireCompound::Soft:   return 1.3f;
        case TireCompound::Medium: return 1.0f;
        case TireCompound::Hard:   return 0.8f;
    }
    return 1.0f;
}

float PhysicsEngine::getDegradationRate(TireCompound compound) const {
    switch (compound) {
        case TireCompound::Soft:   return 0.008f;
        case TireCompound::Medium: return 0.004f;
        case TireCompound::Hard:   return 0.002f;
    }
    return 0.004f;
}

Vector2D PhysicsEngine::closestPointOnSegment(const Vector2D& p, const Vector2D& a, const Vector2D& b) {
    Vector2D ab = b - a;
    float lenSq = ab.magnitudeSquared();
    if (lenSq < 1e-10f) return a;
    float t = std::clamp((p - a).dot(ab) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}

// void PhysicsEngine::initCars(int numberOfCars) {
//     cars.clear();
//     cars.resize(numberOfCars);
//     for (int i = 0; i < numberOfCars; ++i) {
//         cars[i].id = i;
//         cars[i].position = Vector2D(10.0f + (i % 2) * 4.0f,
//                                      15.0f + (i / 2) * 6.0f);
//         cars[i].velocity = Vector2D(0.0f, 0.0f);
//         cars[i].rotationAngle = 0.0f;
//         cars[i].currentTires = TireCompound::Medium;
//         cars[i].tireDegradation = 0.0f;
//     }
// }
void PhysicsEngine::initCars(int numberOfCars) {
    cars.clear();
    cars.resize(numberOfCars);
    for (int i = 0; i < numberOfCars; ++i) {
        cars[i].id = i;
        // Spawning at Y=10 (center of the track), spaced out along the X axis
        cars[i].position = Vector2D(15.0f + (i * 10.0f), 10.0f); 
        cars[i].velocity = Vector2D(0.0f, 0.0f);
        cars[i].rotationAngle = 0.0f; // Facing directly right (+X)
        
        // Give them distinct colors!
        if (i == 0) cars[i].color = {255, 50, 50}; // Red car
        else cars[i].color = {50, 150, 255};       // Blue car

        cars[i].currentTires = TireCompound::Medium;
        cars[i].tireDegradation = 0.0f;
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
        CarState& car = cars[i];
        const ControlOutput& input = inputs[i];

        float throttle = std::clamp(input.throttle, -1.0f, 1.0f);
        float brake = std::clamp(input.brake, 0.0f, 1.0f);
        float steering = std::clamp(input.steeringAngle, -1.0f, 1.0f);

        float speed = car.velocity.magnitude();

        // Steering — bicycle model: turn rate depends on speed and wheel angle
        float steerAngle = steering * maxSteeringAngle;
        float effectiveSpeed = std::max(speed, 0.5f);
        float turnRate = effectiveSpeed * std::tan(steerAngle) / wheelbase;
        turnRate = std::clamp(turnRate, -maxTurnRate, maxTurnRate);
        car.rotationAngle += turnRate * deltaTime;

        Vector2D forward(std::cos(car.rotationAngle), std::sin(car.rotationAngle));
        Vector2D lateral(-std::sin(car.rotationAngle), std::cos(car.rotationAngle));

        // Tire grip (decreases with degradation)
        float gripBase = getGripCoefficient(car.currentTires);
        float degradationPenalty = car.tireDegradation * 0.7f;
        float grip = gripBase * std::max(0.3f, 1.0f - degradationPenalty);
        float maxLateralForce = grip * carMass * GRAVITY;

        // Lateral dynamics — eliminate sideways velocity up to grip limit
        float lateralSpeed = car.velocity.dot(lateral);
        float desiredLateralForce = -lateralSpeed * carMass / deltaTime;
        float actualLateralForce;

        if (std::abs(desiredLateralForce) > maxLateralForce) {
            // Tires exceeded grip — car is sliding
            float sign = (desiredLateralForce > 0.0f) ? 1.0f : -1.0f;
            actualLateralForce = sign * maxLateralForce;
            car.tireDegradation += getDegradationRate(car.currentTires) * 3.0f * deltaTime;
        } else {
            actualLateralForce = desiredLateralForce;
            float loadRatio = (maxLateralForce > 0.0f)
                ? std::abs(desiredLateralForce) / maxLateralForce : 0.0f;
            car.tireDegradation += getDegradationRate(car.currentTires) * (0.1f + loadRatio) * deltaTime;
        }
        car.tireDegradation = std::clamp(car.tireDegradation, 0.0f, 1.0f);

        // Engine force along car heading
        Vector2D engineForce = forward * (throttle * maxEngineForce);

        // Brake force opposing current velocity
        Vector2D brakeForce(0.0f, 0.0f);
        if (speed > 0.1f) {
            brakeForce = car.velocity.normalized() * (-brake * maxBrakeForce);
        }

        // Aerodynamic drag (quadratic)
        Vector2D dragForce = car.velocity * (-dragCoefficient * speed);

        // Rolling resistance
        Vector2D rollingForce(0.0f, 0.0f);
        if (speed > 0.1f) {
            rollingForce = car.velocity.normalized() * (-rollingResistance);
        }

        // Lateral tire correction force
        Vector2D lateralTireForce = lateral * actualLateralForce;

        // Integrate forces
        Vector2D totalForce = engineForce + brakeForce + dragForce + rollingForce + lateralTireForce;
        car.velocity += (totalForce / carMass) * deltaTime;

        // Snap to zero when braking at very low speed
        if (brake > 0.5f && car.velocity.magnitude() < 0.5f) {
            car.velocity = Vector2D(0.0f, 0.0f);
        }
    }
}

void PhysicsEngine::updatePositions(float deltaTime) {
    for (auto& car : cars) {
        car.position += car.velocity * deltaTime;
    }
}

void PhysicsEngine::resolveCollisions() {
    // --- Track boundary collisions ---
    auto pushFromBoundary = [this](CarState& car, const std::vector<Vector2D>& boundary) {
        if (boundary.size() < 2) return;

        float minDist = 1e10f;
        Vector2D closestPt;

        for (size_t j = 0; j + 1 < boundary.size(); ++j) {
            Vector2D cp = closestPointOnSegment(car.position, boundary[j], boundary[j + 1]);
            float d = (car.position - cp).magnitude();
            if (d < minDist) { minDist = d; closestPt = cp; }
        }
        if (boundary.size() >= 3) {
            Vector2D cp = closestPointOnSegment(car.position, boundary.back(), boundary.front());
            float d = (car.position - cp).magnitude();
            if (d < minDist) { minDist = d; closestPt = cp; }
        }

        if (minDist < CAR_RADIUS && minDist > 1e-6f) {
            Vector2D pushDir = (car.position - closestPt).normalized();
            car.position = closestPt + pushDir * CAR_RADIUS;

            float velIntoWall = car.velocity.dot(pushDir);
            if (velIntoWall < 0.0f) {
                car.velocity -= pushDir * (1.5f * velIntoWall);
                car.velocity *= 0.85f;
            }
        }
    };

    for (auto& car : cars) {
        pushFromBoundary(car, currentTrack.outerBoundaries);
        pushFromBoundary(car, currentTrack.innerBoundaries);
    }

    // --- Car-car collisions (toggleable) ---
    if (!carCarCollisionsEnabled) return;

    for (size_t i = 0; i < cars.size(); ++i) {
        for (size_t j = i + 1; j < cars.size(); ++j) {
            Vector2D diff = cars[j].position - cars[i].position;
            float dist = diff.magnitude();
            float minSep = CAR_RADIUS * 2.0f;

            if (dist < minSep && dist > 1e-6f) {
                Vector2D normal = diff / dist;
                float overlap = minSep - dist;

                cars[i].position -= normal * (overlap * 0.5f);
                cars[j].position += normal * (overlap * 0.5f);

                float vi = cars[i].velocity.dot(normal);
                float vj = cars[j].velocity.dot(normal);

                cars[i].velocity += normal * ((vj - vi) * 0.8f);
                cars[j].velocity += normal * ((vi - vj) * 0.8f);
            }
        }
    }
}

void PhysicsEngine::update(float deltaTime, const std::vector<ControlOutput>& inputs) {
    if (deltaTime <= 0.0f) return;
    applyForces(deltaTime, inputs);
    updatePositions(deltaTime);
    resolveCollisions();
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
    t.currentLapTime = 0.0f;
    t.lapsRemaining = 0;
    return t;
}
