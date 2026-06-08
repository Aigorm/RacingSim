#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

// Constants for coordinate mapping (Zoom and Camera position)
const float SCALE = 8.0f;       // 1 meter = 8 pixels
const float OFFSET_X = 100.0f;  // Push everything right
const float OFFSET_Y = 100.0f;  // Push everything down
int SCREEN_HEIGHT = 720;

// Helper to convert Physics (World) coordinates to Screen coordinates
SDL_Point worldToScreen(const Vector2D& worldPos) {
    return {
        static_cast<int>(worldPos.x * SCALE + OFFSET_X),
        static_cast<int>(SCREEN_HEIGHT - (worldPos.y * SCALE + OFFSET_Y)) // Invert Y-axis for SDL
    };
}

Renderer::Renderer() : window(nullptr), sdlRenderer(nullptr) {}

Renderer::~Renderer() {
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init(const std::string& title, int width, int height, bool vsync) {
    SCREEN_HEIGHT = height;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
    if (vsync) renderFlags |= SDL_RENDERER_PRESENTVSYNC;

    sdlRenderer = SDL_CreateRenderer(window, -1, renderFlags);
    if (!sdlRenderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool Renderer::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            return false; // User clicked the 'X' to close the window
        }
    }
    return true;
}

void Renderer::drawTrack(const TrackInfo& track) {
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255); // White for track lines

    auto drawBoundary = [&](const std::vector<Vector2D>& boundary) {
        if (boundary.empty()) return;
        for (size_t i = 0; i < boundary.size() - 1; ++i) {
            SDL_Point p1 = worldToScreen(boundary[i]);
            SDL_Point p2 = worldToScreen(boundary[i + 1]);
            SDL_RenderDrawLine(sdlRenderer, p1.x, p1.y, p2.x, p2.y);
        }
        // Connect the loop (last point to first point)
        SDL_Point pFirst = worldToScreen(boundary.front());
        SDL_Point pLast = worldToScreen(boundary.back());
        SDL_RenderDrawLine(sdlRenderer, pLast.x, pLast.y, pFirst.x, pFirst.y);
    };

    drawBoundary(track.outerBoundaries);
    
    SDL_SetRenderDrawColor(sdlRenderer, 150, 150, 150, 255); // Gray for inner boundaries
    drawBoundary(track.innerBoundaries);
}

void Renderer::drawCar(const CarState& car) {
    // Set car color from telemetry
    SDL_SetRenderDrawColor(sdlRenderer, car.color.r, car.color.g, car.color.b, 255);

    // Car dimensions in meters
    float length = 4.0f;
    float width = 2.0f;

    // Define the 4 corners of the car relative to its center (0,0)
    Vector2D corners[4] = {
        { length / 2,  width / 2},  // Front Left
        { length / 2, -width / 2},  // Front Right
        {-length / 2, -width / 2},  // Rear Right
        {-length / 2,  width / 2}   // Rear Left
    };

    // Rotate and translate corners
    SDL_Point screenCorners[4];
    for (int i = 0; i < 4; ++i) {
        // 2D Rotation Matrix
        float rotX = corners[i].x * std::cos(car.rotationAngle) - corners[i].y * std::sin(car.rotationAngle);
        float rotY = corners[i].x * std::sin(car.rotationAngle) + corners[i].y * std::cos(car.rotationAngle);
        
        Vector2D worldPos = car.position + Vector2D(rotX, rotY);
        screenCorners[i] = worldToScreen(worldPos);
    }

    // Draw the 4 lines connecting the corners
    for (int i = 0; i < 4; ++i) {
        SDL_RenderDrawLine(sdlRenderer, 
                           screenCorners[i].x, screenCorners[i].y, 
                           screenCorners[(i + 1) % 4].x, screenCorners[(i + 1) % 4].y);
    }

    // Draw a small line to indicate the front/heading of the car
    SDL_Point center = worldToScreen(car.position);
    SDL_Point frontCenter = { (screenCorners[0].x + screenCorners[1].x) / 2, 
                              (screenCorners[0].y + screenCorners[1].y) / 2 };
    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 255); // Red line for heading
    SDL_RenderDrawLine(sdlRenderer, center.x, center.y, frontCenter.x, frontCenter.y);
}

void Renderer::drawFrame(const std::vector<CarState>& cars, const TrackInfo& track) {
    // 1. Clear the screen with a dark background (Asphalt-ish)
    SDL_SetRenderDrawColor(sdlRenderer, 30, 30, 30, 255);
    SDL_RenderClear(sdlRenderer);

    // 2. Draw the track
    drawTrack(track);

    // 3. Draw all cars
    for (const auto& car : cars) {
        drawCar(car);
    }

    // 4. Swap buffers to show what we drew
    SDL_RenderPresent(sdlRenderer);
}