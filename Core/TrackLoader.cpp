#include "TrackLoader.h"
#include <fstream>
#include <cmath>

TrackInfo TrackLoader::loadFromTxt(const std::string& filename) {
    TrackInfo track;
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Możesz tu dodać std::cerr << "Nie udalo sie otworzyc mapy!" << std::endl;
        return track;
    }

    float outerWidth, innerWidth;
    // Wczytanie pierwszej linijki (np. 25 25)
    if (!(file >> outerWidth >> innerWidth)) return track;

    std::vector<Vector2D> centerline;
    float x, y;
    // Wczytywanie kolejnych punktów X Y
    while (file >> x >> y) {
        centerline.push_back({x, y});
    }
    track.optimalRacingLine = centerline;

    int n = centerline.size();
    if (n < 2) return track;

    // --- OBLICZANIE GRANIC TORU (Matematyka wektorów normalnych) ---
    for (int i = 0; i < n; ++i) {
        // Zabezpieczenie dla zamkniętej pętli toru (zawijanie indeksów)
        int prev_idx = (i - 1 + n) % n;
        int next_idx = (i + 1) % n;

        Vector2D p_prev = centerline[prev_idx];
        Vector2D p_next = centerline[next_idx];

        // 1. Wektor styczny (Tangent) - kierunek, w którym "patrzy" tor
        float tx = p_next.x - p_prev.x;
        float ty = p_next.y - p_prev.y;

        // 2. Normalizacja wektora stycznego
        float length = std::sqrt(tx * tx + ty * ty);
        if (length > 0.0f) {
            tx /= length;
            ty /= length;
        }

        // 3. Wektor normalny (obrócony o 90 stopni)
        // Jeśli tor jest generowany tak jak Twój (zgodnie z ruchem wskazówek zegara),
        // wektor (-ty, tx) wskazuje na ZEWNĄTRZ toru.
        float nx = -ty;
        float ny = tx;

        // 4. Wyliczenie krawędzi
        Vector2D outerPoint(centerline[i].x + nx * outerWidth, centerline[i].y + ny * outerWidth);
        Vector2D innerPoint(centerline[i].x - nx * innerWidth, centerline[i].y - ny * innerWidth);

        track.outerBoundaries.push_back(outerPoint);
        track.innerBoundaries.push_back(innerPoint);
    }

    return track;
}