#include <iostream>
#include <fstream>
#include <cmath>

int main() {
    // Otwarcie pliku do zapisu
    std::ofstream file("test_track.txt");
    if (!file.is_open()) {
        std::cerr << "Blad: Nie mozna utworzyc pliku!" << std::endl;
        return 1;
    }

    // Parametry okna i toru
    const int WIDTH = 1280;
    const int HEIGHT = 720;
    const int PADDING = 50;
    const int NUM_POINTS = 100; // Zwiększ, jeśli tor ma być bardziej gładki

    // Obliczenia geometrii
    const float cx = WIDTH / 2.0f;
    const float cy = HEIGHT / 2.0f;
    const float radius = (HEIGHT - 2.0f * PADDING) / 2.0f;

    const float PI = 3.14159265359f;

    std::cout << "Generowanie toru..." << std::endl;
    std::cout << "Srodek: (" << cx << ", " << cy << "), Promien: " << radius << std::endl;

    // Generowanie punktów
    for (int i = 0; i < NUM_POINTS; ++i) {
        // t przyjmuje wartości od 0.0 do prawie 1.0
        float t = static_cast<float>(i) / NUM_POINTS;
        
        // Zaczynamy od PI (lewa strona) i odejmujemy kąt, 
        // aby w systemie ekranowym (Y rosnące w dół) iść w stronę dolnej krawędzi
        float angle = PI - (t * 2.0f * PI);

        float x = cx + radius * std::cos(angle);
        float y = cy + radius * std::sin(angle);

        // Zapis do pliku w formacie: X Y
        file << x << " " << y << "\n";
    }

    file.close();
    std::cout << "Zapisano " << NUM_POINTS << " punktow do pliku test_track.txt" << std::endl;
    
    return 0;
}