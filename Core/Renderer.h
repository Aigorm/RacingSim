#pragma once
#include <vector>
#include <string>
#include "../Shared/Telemetry.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Color;
struct SDL_FPoint;

class Renderer {
private:
    SDL_Window* window;
    SDL_Renderer* sdlRenderer;

    void drawTrack(const TrackInfo& track);
    void drawAsphalt(const TrackInfo& track, int n);
    void drawThickBoundary(const std::vector<Vector2D>& boundary, int n);
    void drawCenterLine(const std::vector<Vector2D>& line, int n);
    void drawStartFinishLine(const TrackInfo& track);

    void drawCar(const CarState& car);
    
    SDL_FPoint transformLocalToWorld(float localX, float localY, float cx, float cy, float cosA, float sinA) const;
    void drawQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, SDL_Color color, float cx, float cy, float cosA, float sinA);
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color, float cx, float cy, float cosA, float sinA);

public:
    Renderer();
    ~Renderer();

    bool init(const std::string& title, int width, int height, bool vsync);
    bool pollEvents();
    void drawFrame(const std::vector<CarState>& cars, const TrackInfo& track);
};