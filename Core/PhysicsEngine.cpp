#include "PhysicsEngine.h"
#include "../API/BotRegistry.h"
#include <cmath>
#include <algorithm>
#include <iostream>

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

    // 1. Zabezpieczenie: czy tor na pewno został wczytany?
    if (currentTrack.optimalRacingLine.size() < 2) {
        std::cerr << "Blad: Silnik fizyczny probuje ustawic auta, ale tor nie zostal wczytany!" << std::endl;
        return;
    }

    // 2. Analiza linii startu (od punktu 0 do 1)
    Vector2D p0 = currentTrack.optimalRacingLine[0];
    Vector2D p1 = currentTrack.optimalRacingLine[1];

    // Wektor kierunkowy (styczny) - gdzie jedziemy?
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float length = std::sqrt(dx * dx + dy * dy);

    if (length > 0.0f) {
        dx /= length;
        dy /= length;
    } else {
        dx = 1.0f; dy = 0.0f; // Domyślnie w prawo w razie błędu
    }

    // Kąt w radianach (atan2 zwraca kąt między osią X a wektorem)
    float startRotation = std::atan2(dy, dx);

    // Wektor prostopadły (normalny) - służy do przesuwania aut w bok
    float nx = -dy;
    float ny = dx;

    // 3. Konfiguracja "Gridu" wyścigowego
    float gridSpacingBackward = 30.0f; // O ile pikseli cofamy każde kolejne pole startowe
    float gridSpacingLateral = 10.0f;  // O ile pikseli odsuwamy auto od osi toru (w lewo/prawo)

    for (int i = 0; i < numberOfCars; ++i) {
        CarState car;
        car.id = i;

        // Logika szachownicy: na przemian lewa i prawa strona toru
        // i = 0 (Pole position) -> lewo (-1), i = 1 -> prawo (1), i = 2 -> lewo (-1)
        float lateralMultiplier = (i % 2 == 0) ? -1.0f : 1.0f;

        // Obliczamy finalną pozycję:
        // Startujemy z p0, cofamy się o 'i * gridSpacingBackward', dodajemy przesunięcie boczne
        car.position.x = p0.x - (dx * i * gridSpacingBackward) + (nx * lateralMultiplier * gridSpacingLateral);
        car.position.y = p0.y - (dy * i * gridSpacingBackward) + (ny * lateralMultiplier * gridSpacingLateral);

        car.velocity = {0.0f, 0.0f};
        car.rotationAngle = startRotation;

        // Wyciągamy kolor bezpośrednio z definicji bota gracza
        if (i < bots.size()) {
            car.color = bots[i]->getColor();
        } else {
            car.color = ColorRGB(255, 255, 255); // Domyślnie biały
        }

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
        float degradationPenalty = sqrt(car.tireDegradation);
        float grip = gripBase * std::max(0.3f, 1.0f - degradationPenalty);
        
        float downforce = (carMass * GRAVITY * mechanicalGripMultiplier) + (aeroDownforceMultiplier * speed * speed);
        float maxLateralForce = grip * downforce;

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

        if (minDist <= 1e-6f) return;

        // Kierunek "od ściany" (normalna ściany skierowana w stronę środka auta).
        Vector2D normal = (car.position - closestPt).normalized();

        // Osie auta: przód (X lokalne) i bok (Y lokalne) - jak w Renderer.cpp.
        Vector2D forward(std::cos(car.rotationAngle), std::sin(car.rotationAngle));
        Vector2D lateral(-std::sin(car.rotationAngle), std::cos(car.rotationAngle));

        // Rzut połowy prostokąta auta na kierunek normalnej ściany.
        // To zastępuje stały promień - auto jest traktowane jako obrócony prostokąt.
        float effectiveRadius = CAR_HALF_LENGTH * std::abs(normal.dot(forward))
                              + CAR_HALF_WIDTH  * std::abs(normal.dot(lateral));

        if (minDist < effectiveRadius) {
            // Wypchnij auto tak, by jego krawędź ledwo dotykała ściany.
            car.position = closestPt + normal * effectiveRadius;

            // Rozbij prędkość na składową prostopadłą (w ścianę) i styczną (wzdłuż ściany).
            float velNormal = car.velocity.dot(normal); // < 0 gdy auto wjeżdża w ścianę
            if (velNormal < 0.0f) {
                Vector2D vNormal  = normal * velNormal;       // składowa wbijająca się w ścianę
                Vector2D vTangent = car.velocity - vNormal;   // składowa wzdłuż ściany

                const float restitution = 0.3f; // odbicie (deflekcja) od ściany
                const float friction    = 0.60f; // tarcie przy ślizganiu wzdłuż ściany

                // Auto ślizga się wzdłuż ściany i delikatnie się od niej odbija.
                car.velocity = vTangent * friction - vNormal * restitution;
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
    
    // 1. FIZYKA JAZDY
    applyForces(deltaTime, inputs);
    updatePositions(deltaTime);
    resolveCollisions();

    // 2. LOGIKA WYŚCIGOWA (Sędzia)
    totalRaceTime += deltaTime;

    int totalPoints = currentTrack.optimalRacingLine.size();
    if (totalPoints == 0) return; // Zabezpieczenie przed brakiem toru

    int numSectors = 20; 

    // Helper lambdas to calculate exact line segment intersection (CCW mathematically determines if two lines cross)
    auto ccw = [](const Vector2D& a, const Vector2D& b, const Vector2D& c) {
        return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
    };
    auto segmentsIntersect = [&](const Vector2D& a, const Vector2D& b, const Vector2D& c, const Vector2D& d) {
        return (ccw(a, c, d) != ccw(b, c, d)) && (ccw(a, b, c) != ccw(a, b, d));
    };

    for (auto& car : cars) {
        if (car.isFinished) continue;

        int targetPointIndex = (car.nextSectorToClear * totalPoints) / numSectors % totalPoints;

        // Build the checkpoint "Gate" connecting the inner and outer boundaries
        Vector2D checkpointInner = currentTrack.innerBoundaries[targetPointIndex];
        Vector2D checkpointOuter = currentTrack.outerBoundaries[targetPointIndex];
        
        // We slightly extend the gate (e.g., by 5 pixels) into the grass to ensure 
        // cars heavily hugging or grinding the wall don't accidentally slip past the line.
        Vector2D gateDir = (checkpointOuter - checkpointInner).normalized();
        Vector2D cpStart = checkpointInner - gateDir * 5.0f;
        Vector2D cpEnd = checkpointOuter + gateDir * 5.0f;

        // Calculate where the car was in the last frame vs where it is now
        Vector2D oldPos = car.position - (car.velocity * deltaTime);
        Vector2D newPos = car.position;

        // Check for physical intersection between the car's path and the checkpoint gate
        if (segmentsIntersect(oldPos, newPos, cpStart, cpEnd)) {
            
            // Security check: Make sure the car crossed it driving FORWARD, not rolling backwards
            int nextTrackPointIndex = (targetPointIndex + 1) % totalPoints;
            Vector2D trackForward = (currentTrack.optimalRacingLine[nextTrackPointIndex] - currentTrack.optimalRacingLine[targetPointIndex]).normalized();
            
            if (car.velocity.dot(trackForward) > 0.0f) {
                car.nextSectorToClear++; 
                
                if (car.nextSectorToClear > numSectors) {
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
            }
        }
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
