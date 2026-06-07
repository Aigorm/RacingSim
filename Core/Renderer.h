#pragma once
#include <vector>
#include <string>
#include "../Shared/Telemetry.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class Renderer {
private:
    // Główne uchwyty (wskaźniki) do okna i mechanizmu rysującego SDL2
    SDL_Window* window;
    SDL_Renderer* sdlRenderer;

    // TODO: Zadeklarować zmienne na tekstury
    // SDL_Texture* carTexture;
    // SDL_Texture* trackBackgroundTexture;

    // PRYWATNE FUNKCJE POMOCNICZE
    
    // TODO: Funkcja rysująca kształt toru (np. wielokąty na podstawie TrackInfo)
    void drawTrack(const TrackInfo& track);
    
    // TODO: Funkcja rysująca pojedyncze auto z uwzględnieniem jego rotacji i pozycji
    void drawCar(const CarState& car);
    
    // TODO: (Opcjonalnie) Funkcja rysująca interfejs z czasami okrążeń i statystykami
    void drawUI();

public:
    Renderer();
    
    // Destruktor : SDL_DestroyRenderer i SDL_Quit
    ~Renderer();

    // Tworzy okno, inicjalizuje SDL2 i ustawia VSync
    bool init(const std::string& title, int width, int height, bool vsync);

    // Sprawdza kolejkę zdarzeń systemu operacyjnego (np. czy wciśnięto X)
    // Zwraca false, jeśli użytkownik chce zamknąć program.
    bool pollEvents();

    // Główna funkcja wywoływana co klatkę z main.cpp
    // Czyści ekran, wywołuje prywatne funkcje drawTrack i drawCar, a na końcu robi SDL_RenderPresent
    void drawFrame(const std::vector<CarState>& cars, const TrackInfo& track);
};