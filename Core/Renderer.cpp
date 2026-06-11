#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

namespace {
    constexpr SDL_Color COLOR_GRASS       = {34, 139, 34, 255};
    constexpr SDL_Color COLOR_ASPHALT     = {128, 128, 128, 255};
    constexpr SDL_Color COLOR_BOUNDARY    = {0, 0, 0, 255};
    constexpr SDL_Color COLOR_CENTERLINE  = {255, 255, 0, 255};
    constexpr SDL_Color COLOR_STARTLINE   = {255, 165, 0, 255};
    constexpr SDL_Color COLOR_WHEEL_BLACK = {0, 0, 0, 255};

    constexpr int   BOUNDARY_THICKNESS_OFFSET = 1; 
    constexpr float START_LINE_THICKNESS      = 6.0f;

    constexpr float CAR_HALF_LENGTH = 12.0f; 
    constexpr float CAR_HALF_WIDTH  = 6.0f;  
    constexpr float NOSE_LENGTH     = 6.0f;  
    
    constexpr float WHEEL_INSET = 4.0f;
    constexpr float WHEEL_EXTENT = 2.0f;
    constexpr float WHEEL_PROTRUSION_OUT = 2.0f;
    constexpr float WHEEL_PROTRUSION_IN = 1.0f;
}

Renderer::Renderer() : window(nullptr), sdlRenderer(nullptr) {}

Renderer::~Renderer() {
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (window)      SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init(const std::string& title, int width, int height, bool vsync) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Blad SDL_Init: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Blad SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return false;
    }

    Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
    if (vsync) renderFlags |= SDL_RENDERER_PRESENTVSYNC;

    sdlRenderer = SDL_CreateRenderer(window, -1, renderFlags);
    if (!sdlRenderer) {
        std::cerr << "Blad SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool Renderer::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}

void Renderer::drawFrame(const std::vector<CarState>& cars, const TrackInfo& track) {
    SDL_SetRenderDrawColor(sdlRenderer, COLOR_GRASS.r, COLOR_GRASS.g, COLOR_GRASS.b, COLOR_GRASS.a);
    SDL_RenderClear(sdlRenderer);

    drawTrack(track);
    for (const auto& car : cars) { 
        drawCar(car); 
    }

    SDL_RenderPresent(sdlRenderer);
}

void Renderer::drawTrack(const TrackInfo& track) {
    if (track.optimalRacingLine.empty() || track.innerBoundaries.empty()) return;
    
    int n = track.optimalRacingLine.size();

    drawAsphalt(track, n);
    drawThickBoundary(track.outerBoundaries, n);
    drawThickBoundary(track.innerBoundaries, n); 
    drawCenterLine(track.optimalRacingLine, n);
    drawStartFinishLine(track);
}

void Renderer::drawAsphalt(const TrackInfo& track, int n) {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;

        int baseIdx = vertices.size();
        vertices.push_back({{track.innerBoundaries[i].x, track.innerBoundaries[i].y}, COLOR_ASPHALT, {0,0}});
        vertices.push_back({{track.outerBoundaries[i].x, track.outerBoundaries[i].y}, COLOR_ASPHALT, {0,0}});
        vertices.push_back({{track.outerBoundaries[next_i].x, track.outerBoundaries[next_i].y}, COLOR_ASPHALT, {0,0}});
        vertices.push_back({{track.innerBoundaries[next_i].x, track.innerBoundaries[next_i].y}, COLOR_ASPHALT, {0,0}});

        indices.insert(indices.end(), {baseIdx, baseIdx + 1, baseIdx + 2, baseIdx, baseIdx + 2, baseIdx + 3});
    }
    
    SDL_RenderGeometry(sdlRenderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}

void Renderer::drawThickBoundary(const std::vector<Vector2D>& boundary, int n) {
    SDL_SetRenderDrawColor(sdlRenderer, COLOR_BOUNDARY.r, COLOR_BOUNDARY.g, COLOR_BOUNDARY.b, COLOR_BOUNDARY.a);
    
    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;
        
        for(int offsetX = -BOUNDARY_THICKNESS_OFFSET; offsetX <= BOUNDARY_THICKNESS_OFFSET; ++offsetX) {
            for(int offsetY = -BOUNDARY_THICKNESS_OFFSET; offsetY <= BOUNDARY_THICKNESS_OFFSET; ++offsetY) {
                 SDL_RenderDrawLine(sdlRenderer, 
                    boundary[i].x + offsetX, boundary[i].y + offsetY,
                    boundary[next_i].x + offsetX, boundary[next_i].y + offsetY);
            }
        }
    }
}

void Renderer::drawCenterLine(const std::vector<Vector2D>& line, int n) {
    SDL_SetRenderDrawColor(sdlRenderer, COLOR_CENTERLINE.r, COLOR_CENTERLINE.g, COLOR_CENTERLINE.b, COLOR_CENTERLINE.a);
    for (int i = 0; i < n; ++i) {
        SDL_RenderDrawLine(sdlRenderer, 
            line[i].x, line[i].y,
            line[(i + 1) % n].x, line[(i + 1) % n].y);
    }
}

void Renderer::drawStartFinishLine(const TrackInfo& track) {
    if (track.optimalRacingLine.size() < 2) return;

    Vector2D p0 = track.optimalRacingLine[0];
    Vector2D p1 = track.optimalRacingLine[1];
    
    Vector2D dir = p1 - p0;
    float length = dir.magnitude();
    if (length > 0.0f) dir = dir / length;

    std::vector<SDL_Vertex> startLineVertices = {
        {{track.innerBoundaries[0].x, track.innerBoundaries[0].y}, COLOR_STARTLINE, {0,0}},
        {{track.outerBoundaries[0].x, track.outerBoundaries[0].y}, COLOR_STARTLINE, {0,0}},
        {{track.outerBoundaries[0].x + dir.x * START_LINE_THICKNESS, track.outerBoundaries[0].y + dir.y * START_LINE_THICKNESS}, COLOR_STARTLINE, {0,0}},
        {{track.innerBoundaries[0].x + dir.x * START_LINE_THICKNESS, track.innerBoundaries[0].y + dir.y * START_LINE_THICKNESS}, COLOR_STARTLINE, {0,0}}
    };
    
    std::vector<int> startLineIndices = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(sdlRenderer, nullptr, startLineVertices.data(), startLineVertices.size(), startLineIndices.data(), startLineIndices.size());
}

void Renderer::drawCar(const CarState& car) {
    const float cosA = std::cos(car.rotationAngle);
    const float sinA = std::sin(car.rotationAngle);
    const float cx = car.position.x;
    const float cy = car.position.y;

    SDL_Color bodyColor = {car.color.r, car.color.g, car.color.b, 255};
    SDL_Color noseColor = {
        static_cast<Uint8>(car.color.r * 0.7f), 
        static_cast<Uint8>(car.color.g * 0.7f), 
        static_cast<Uint8>(car.color.b * 0.7f), 255
    };

    drawQuad(CAR_HALF_LENGTH - WHEEL_INSET, -CAR_HALF_WIDTH - WHEEL_PROTRUSION_OUT, CAR_HALF_LENGTH + WHEEL_EXTENT, -CAR_HALF_WIDTH - WHEEL_PROTRUSION_OUT, CAR_HALF_LENGTH + WHEEL_EXTENT, -CAR_HALF_WIDTH + WHEEL_PROTRUSION_IN, CAR_HALF_LENGTH - WHEEL_INSET, -CAR_HALF_WIDTH + WHEEL_PROTRUSION_IN, COLOR_WHEEL_BLACK, cx, cy, cosA, sinA);
    drawQuad(CAR_HALF_LENGTH - WHEEL_INSET, CAR_HALF_WIDTH - WHEEL_PROTRUSION_IN, CAR_HALF_LENGTH + WHEEL_EXTENT, CAR_HALF_WIDTH - WHEEL_PROTRUSION_IN, CAR_HALF_LENGTH + WHEEL_EXTENT, CAR_HALF_WIDTH + WHEEL_PROTRUSION_OUT, CAR_HALF_LENGTH - WHEEL_INSET, CAR_HALF_WIDTH + WHEEL_PROTRUSION_OUT, COLOR_WHEEL_BLACK, cx, cy, cosA, sinA);
    drawQuad(-CAR_HALF_LENGTH, -CAR_HALF_WIDTH - WHEEL_PROTRUSION_OUT, -CAR_HALF_LENGTH + NOSE_LENGTH, -CAR_HALF_WIDTH - WHEEL_PROTRUSION_OUT, -CAR_HALF_LENGTH + NOSE_LENGTH, -CAR_HALF_WIDTH + WHEEL_PROTRUSION_IN, -CAR_HALF_LENGTH, -CAR_HALF_WIDTH + WHEEL_PROTRUSION_IN, COLOR_WHEEL_BLACK, cx, cy, cosA, sinA);
    drawQuad(-CAR_HALF_LENGTH, CAR_HALF_WIDTH - WHEEL_PROTRUSION_IN, -CAR_HALF_LENGTH + NOSE_LENGTH, CAR_HALF_WIDTH - WHEEL_PROTRUSION_IN, -CAR_HALF_LENGTH + NOSE_LENGTH, CAR_HALF_WIDTH + WHEEL_PROTRUSION_OUT, -CAR_HALF_LENGTH, CAR_HALF_WIDTH + WHEEL_PROTRUSION_OUT, COLOR_WHEEL_BLACK, cx, cy, cosA, sinA);

    drawQuad(CAR_HALF_LENGTH, CAR_HALF_WIDTH, CAR_HALF_LENGTH, -CAR_HALF_WIDTH, -CAR_HALF_LENGTH, -CAR_HALF_WIDTH, -CAR_HALF_LENGTH, CAR_HALF_WIDTH, bodyColor, cx, cy, cosA, sinA);

    drawTriangle(CAR_HALF_LENGTH, -CAR_HALF_WIDTH, CAR_HALF_LENGTH, CAR_HALF_WIDTH, CAR_HALF_LENGTH + NOSE_LENGTH, 0.0f, noseColor, cx, cy, cosA, sinA);
}

SDL_FPoint Renderer::transformLocalToWorld(float localX, float localY, float cx, float cy, float cosA, float sinA) const {
    return {
        cx + (localX * cosA - localY * sinA),
        cy + (localX * sinA + localY * cosA)
    };
}

void Renderer::drawQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, SDL_Color color, float cx, float cy, float cosA, float sinA) {
    SDL_Vertex verts[4] = {
        {transformLocalToWorld(x1, y1, cx, cy, cosA, sinA), color, {0,0}},
        {transformLocalToWorld(x2, y2, cx, cy, cosA, sinA), color, {0,0}},
        {transformLocalToWorld(x3, y3, cx, cy, cosA, sinA), color, {0,0}},
        {transformLocalToWorld(x4, y4, cx, cy, cosA, sinA), color, {0,0}}
    };
    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(sdlRenderer, nullptr, verts, 4, indices, 6);
}

void Renderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color, float cx, float cy, float cosA, float sinA) {
    SDL_Vertex verts[3] = {
        {transformLocalToWorld(x1, y1, cx, cy, cosA, sinA), color, {0,0}},
        {transformLocalToWorld(x2, y2, cx, cy, cosA, sinA), color, {0,0}},
        {transformLocalToWorld(x3, y3, cx, cy, cosA, sinA), color, {0,0}}
    };
    int indices[3] = {0, 1, 2};
    SDL_RenderGeometry(sdlRenderer, nullptr, verts, 3, indices, 3);
}