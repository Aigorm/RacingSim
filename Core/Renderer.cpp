#include "Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>

// --- KONSTRUKTOR I DESTRUKTOR ---

Renderer::Renderer() : window(nullptr), sdlRenderer(nullptr) {
    // Inicjalizacja wskaźników
}

Renderer::~Renderer() {
    // Zwalnianie pamięci w odwrotnej kolejności do jej alokacji
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

// --- INICJALIZACJA SDL2 ---

bool Renderer::init(const std::string& title, int width, int height, bool vsync) {
    // Inicjalizacja podsystemu wideo SDL2
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

    // Flagi dla renderera - sprzętowa akceleracja i opcjonalny VSync
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

// --- OBSŁUGA ZDARZEŃ ---

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

// --- GŁÓWNA PĘTLA RYSOWANIA ---

void Renderer::drawFrame(const std::vector<CarState>& cars, const TrackInfo& track) {
    // 1. Tło na ciemnozielono
    SDL_SetRenderDrawColor(sdlRenderer, 34, 139, 34, 255);
    SDL_RenderClear(sdlRenderer);

    // 2. Rysowanie toru
    drawTrack(track);

    // TODO: W przyszłości dodamy tu pętlę po autach:
    // for (const auto& car : cars) { drawCar(car); }

    // 3. Wyrzucenie klatki na monitor (tu program zablokuje się dzięki VSync)
    SDL_RenderPresent(sdlRenderer);
}

// --- LOGIKA RYSOWANIA TORU ---

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

    // C: Zewnętrzna banda wewnętrzna (Czarna, pogrubiona) - Dodano, by tor miał dwie bandy!
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

    // ==========================================
    // ETAP E: Linia startu/mety (Pomarańczowy prostokąt)
    // ==========================================
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
        // Bierzemy punkty z krawędzi i przesuwamy je lekko wzdłuż toru
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