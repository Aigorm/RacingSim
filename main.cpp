#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include "Core/PhysicsEngine.h"
#include "Core/Renderer.h"
#include "Shared/Telemetry.h"

// Mock track generator
TrackInfo createTestTrack() {
    TrackInfo track;
    // Outer wall: 100x80 rectangle
    track.outerBoundaries = { {0,0}, {100,0}, {100,80}, {0,80} };
    // Inner wall: 60x40 rectangle in the middle
    track.innerBoundaries = { {20,20}, {80,20}, {80,60}, {20,60} };
    return track;
}

int main(int argc, char* argv[]) {
    std::cout << "--- Starting 2D Auto Racing Simulator ---\n";

    Renderer renderer;
    if (!renderer.init("Racing Sim 2D", 1280, 720, true)) { // true = VSync enabled
        return -1;
    }

    PhysicsEngine engine;
    TrackInfo track = createTestTrack();
    engine.setTrack(track);
    engine.initCars(2); // Let's spawn 2 cars to see them both!

    // Give the cars some colors
    auto states = engine.getCarStates();
    // Note: To modify the cars, you might need a setter in PhysicsEngine. 
    // For now, we assume they default to white if not set.

    float deltaTime = 1.0f / 60.0f; // 60 FPS physics step
    bool isRunning = true;

    while (isRunning) {
        isRunning = renderer.pollEvents();

        // Get the current state of the cars so our "Bots" can make decisions
        auto states = engine.getCarStates();
        std::vector<ControlOutput> botInputs;

        for (size_t i = 0; i < states.size(); ++i) {
            ControlOutput input;
            input.throttle = 0.4f; // Drive at a safe, moderate speed
            input.brake = 0.0f;
            input.steeringAngle = 0.0f; // Default: go straight

            // --- VERY SIMPLE ALGORITHMIC DRIVER ---
            const auto& pos = states[i].position;
            const auto& vel = states[i].velocity;

            // 1. Approaching the right corner? Turn Left!
            if (pos.x > 75.0f && vel.x > 0.0f) {
                input.steeringAngle = 0.7f; 
            }
            // 2. Approaching the top corner? Turn Left!
            else if (pos.y > 55.0f && vel.y > 0.0f) {
                input.steeringAngle = 0.7f;
            }
            // 3. Approaching the left corner? Turn Left!
            else if (pos.x < 25.0f && vel.x < 0.0f) {
                input.steeringAngle = 0.7f;
            }
            // 4. Approaching the bottom corner? Turn Left!
            else if (pos.y < 25.0f && vel.y < 0.0f) {
                input.steeringAngle = 0.7f;
            }

            // (Optional) Make Car 1 drive slightly faster but turn wider
            if (i == 1) {
                input.throttle = 0.5f;
                input.steeringAngle *= 0.8f;
            }

            botInputs.push_back(input);
        }

        // Update Physics & Render
        engine.update(deltaTime, botInputs);
        renderer.drawFrame(engine.getCarStates(), track);

        SDL_Delay(16); 
    }
    std::cout << "Shutting down cleanly...\n";
    return 0;
}

