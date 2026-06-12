#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>

Renderer::Renderer() : window(nullptr), sdlRenderer(nullptr) {
}

Renderer::~Renderer() {
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool Renderer::init(const std::string& title, int width, int height, bool vsync) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Blad SDL_Init: " << SDL_GetError() << std::endl;
        return false;
    }

    // Tworzenie okna
    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Blad SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return false;
    }

    Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
    if (vsync) {
        renderFlags |= SDL_RENDERER_PRESENTVSYNC;
    }

    // Tworzenie renderera
    sdlRenderer = SDL_CreateRenderer(window, -1, renderFlags);
    if (!sdlRenderer) {
        std::cerr << "Blad SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool Renderer::pollEvents() {
    SDL_Event e;
    // Ściąganie wszystkich zdarzeń z kolejki systemu operacyjnego
    while (SDL_PollEvent(&e)) {
        // Jeśli ktoś kliknął 'X' na oknie
        if (e.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

void Renderer::drawFrame(const std::vector<CarState>& cars, const TrackInfo& track) {
    // 1. Tło na ciemnozielono
    SDL_SetRenderDrawColor(sdlRenderer, 34, 139, 34, 255);
    SDL_RenderClear(sdlRenderer);

    // 2. Rysowanie toru
    drawTrack(track);

    for (const auto& car : cars) { drawCar(car); }

    // 3. Wyrzucenie klatki na monitor (tu program zablokuje się dzięki VSync)
    SDL_RenderPresent(sdlRenderer);
}

void Renderer::drawTrack(const TrackInfo& track) {
    if (track.optimalRacingLine.empty() || track.innerBoundaries.empty()) return;
    
    int n = track.optimalRacingLine.size();

    // A: Asfalt (Szary wielokąt)
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    SDL_Color gray = {128, 128, 128, 255};

    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;

        SDL_Vertex v1 = {{track.innerBoundaries[i].x, track.innerBoundaries[i].y}, gray, {0,0}};
        SDL_Vertex v2 = {{track.outerBoundaries[i].x, track.outerBoundaries[i].y}, gray, {0,0}};
        SDL_Vertex v3 = {{track.outerBoundaries[next_i].x, track.outerBoundaries[next_i].y}, gray, {0,0}};
        SDL_Vertex v4 = {{track.innerBoundaries[next_i].x, track.innerBoundaries[next_i].y}, gray, {0,0}};

        int baseIdx = vertices.size();
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);

        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 3);
    }
    
    SDL_RenderGeometry(sdlRenderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());

    // B: Zewnętrzna banda (Czarna, pogrubiona)
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;
        for(int offsetX = -1; offsetX <= 1; ++offsetX) {
            for(int offsetY = -1; offsetY <= 1; ++offsetY) {
                 SDL_RenderDrawLine(sdlRenderer, 
                    track.outerBoundaries[i].x + offsetX, track.outerBoundaries[i].y + offsetY,
                    track.outerBoundaries[next_i].x + offsetX, track.outerBoundaries[next_i].y + offsetY);
            }
        }
    }

    // C: Zewnętrzna banda wewnętrzna (Czarna, pogrubiona)
    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;
        for(int offsetX = -1; offsetX <= 1; ++offsetX) {
            for(int offsetY = -1; offsetY <= 1; ++offsetY) {
                 SDL_RenderDrawLine(sdlRenderer, 
                    track.innerBoundaries[i].x + offsetX, track.innerBoundaries[i].y + offsetY,
                    track.innerBoundaries[next_i].x + offsetX, track.innerBoundaries[next_i].y + offsetY);
            }
        }
    }

    // D: Linia środkowa (Żółta)
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, 255);
    for (int i = 0; i < n; ++i) {
        int next_i = (i + 1) % n;
        SDL_RenderDrawLine(sdlRenderer, 
            track.optimalRacingLine[i].x, track.optimalRacingLine[i].y,
            track.optimalRacingLine[next_i].x, track.optimalRacingLine[next_i].y);
    }

    if (n > 1) {
        // 1. Obliczamy wektor kierunku (styczny) na samym starcie toru
        Vector2D p0 = track.optimalRacingLine[0];
        Vector2D p1 = track.optimalRacingLine[1];
        
        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float length = std::sqrt(dx * dx + dy * dy);
        
        // Normalizacja wektora
        if (length > 0.0f) {
            dx /= length;
            dy /= length;
        }

        // 2. Parametry prostokąta startowego
        float thickness = 6.0f; // Grubość linii w pikselach
        SDL_Color orange = {255, 165, 0, 255}; // Pomarańczowy RGB

        // 3. Budujemy 4 narożniki prostokąta
        std::vector<SDL_Vertex> startLineVertices;
        
        SDL_Vertex v1 = {{track.innerBoundaries[0].x, track.innerBoundaries[0].y}, orange, {0,0}};
        SDL_Vertex v2 = {{track.outerBoundaries[0].x, track.outerBoundaries[0].y}, orange, {0,0}};
        SDL_Vertex v3 = {{track.outerBoundaries[0].x + dx * thickness, track.outerBoundaries[0].y + dy * thickness}, orange, {0,0}};
        SDL_Vertex v4 = {{track.innerBoundaries[0].x + dx * thickness, track.innerBoundaries[0].y + dy * thickness}, orange, {0,0}};

        startLineVertices.push_back(v1);
        startLineVertices.push_back(v2);
        startLineVertices.push_back(v3);
        startLineVertices.push_back(v4);

        // 4. Definiujemy dwa trójkąty tworzące ten prostokąt
        std::vector<int> startLineIndices = {0, 1, 2, 0, 2, 3};

        // 5. Rysujemy na ekranie
        SDL_RenderGeometry(sdlRenderer, nullptr, startLineVertices.data(), startLineVertices.size(), startLineIndices.data(), startLineIndices.size());
    }
}

void Renderer::drawCar(const CarState& car) {
    // Zakładamy, że wektor kierunku auta to w matematyce oś X.
    // Zatem kąt 0 radianów oznacza auto skierowane w prawo.
    float cosA = std::cos(car.rotationAngle);
    float sinA = std::sin(car.rotationAngle);
    float cx = car.position.x;
    float cy = car.position.y;

    // POMOCNICZA LAMBDA: Obraca lokalne punkty auta (względem jego środka) na współrzędne świata
    auto transform = [&](float localX, float localY) -> SDL_FPoint {
        return {
            cx + (localX * cosA - localY * sinA),
            cy + (localX * sinA + localY * cosA)
        };
    };

    // POMOCNICZA LAMBDA: Rysuje prostokąt
    auto drawQuad = [&](float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, SDL_Color color) {
        SDL_Vertex verts[4];
        SDL_FPoint p1 = transform(x1, y1); verts[0] = { {p1.x, p1.y}, color, {0,0} };
        SDL_FPoint p2 = transform(x2, y2); verts[1] = { {p2.x, p2.y}, color, {0,0} };
        SDL_FPoint p3 = transform(x3, y3); verts[2] = { {p3.x, p3.y}, color, {0,0} };
        SDL_FPoint p4 = transform(x4, y4); verts[3] = { {p4.x, p4.y}, color, {0,0} };

        int indices[6] = {0, 1, 2, 0, 2, 3};
        SDL_RenderGeometry(sdlRenderer, nullptr, verts, 4, indices, 6);
    };

    // POMOCNICZA LAMBDA: Rysuje trójkąt kierunkowy
    auto drawTriangle = [&](float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color) {
        SDL_Vertex verts[3];
        SDL_FPoint p1 = transform(x1, y1); verts[0] = { {p1.x, p1.y}, color, {0,0} };
        SDL_FPoint p2 = transform(x2, y2); verts[1] = { {p2.x, p2.y}, color, {0,0} };
        SDL_FPoint p3 = transform(x3, y3); verts[2] = { {p3.x, p3.y}, color, {0,0} };

        int indices[3] = {0, 1, 2};
        SDL_RenderGeometry(sdlRenderer, nullptr, verts, 3, indices, 3);
    };

    SDL_Color bodyColor = {car.color.r, car.color.g, car.color.b, 255};
    SDL_Color black = {0, 0, 0, 255};
    // Delikatnie przyciemniony kolor dla trójkąta z przodu, żeby odcinał się od maski
    SDL_Color darkBodyColor = {
        static_cast<Uint8>(car.color.r * 0.7), 
        static_cast<Uint8>(car.color.g * 0.7), 
        static_cast<Uint8>(car.color.b * 0.7), 
        255
    };

    // Oś X to przód/tył, Oś Y to lewo/prawo. Środek auta to (0,0).
    const float L = 12.0f; // Połowa długości (X)
    const float W = 6.0f;  // Połowa szerokości (Y)
    
    // 1. KOŁA (Czarne prostokąty delikatnie wystające poza obrys karoserii)
    // Lewe-Przód
    drawQuad(L-4, -W-2, L+2, -W-2, L+2, -W+1, L-4, -W+1, black);
    // Prawe-Przód
    drawQuad(L-4, W-1,  L+2, W-1,  L+2, W+2,  L-4, W+2,  black);
    // Lewe-Tył
    drawQuad(-L, -W-2, -L+6, -W-2, -L+6, -W+1, -L, -W+1, black);
    // Prawe-Tył
    drawQuad(-L, W-1,  -L+6, W-1,  -L+6, W+2,  -L, W+2,  black);

    // 2. KAROSERIA (Główny prostokąt)
    // Kolejność: Prawy-Przód, Lewy-Przód, Lewy-Tył, Prawy-Tył
    drawQuad(L, W, L, -W, -L, -W, -L, W, bodyColor);

    // 3. TRÓJKĄT KIERUNKOWY (Maska/Nos auta)
    // Rysowany z przodu karoserii (wyciągnięty o 6 pikseli w stronę osi X)
    drawTriangle(L, -W, L, W, L+6, 0.0f, darkBodyColor);
}