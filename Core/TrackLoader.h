#pragma once
#include <string>
#include "../Shared/Telemetry.h"

class TrackLoader {
public:
    TrackLoader() = default;
    ~TrackLoader() = default;

    // Funkcja wczytująca punkty z naszego generatora tekstowego
    TrackInfo loadFromTxt(const std::string& filename);

    // TODO: Zaimplementować na koniec projektu, aby wczytywać płynne krzywe
    // TrackInfo loadFromSvg(const std::string& filename);
};