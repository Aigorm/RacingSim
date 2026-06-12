#include "TrackLoader.h"
#include <fstream>
#include <iostream>
#include <cmath>

namespace {

    Vector2D calculateOutwardNormal(const Vector2D& p_prev, const Vector2D& p_next) {
        float tx = p_next.x - p_prev.x;
        float ty = p_next.y - p_prev.y;

        float length = std::sqrt(tx * tx + ty * ty);
        if (length > 0.0f) {
            tx /= length;
            ty /= length;
        }

        return {-ty, tx};
    }
}

TrackInfo TrackLoader::loadFromTxt(const std::string& filename) const {
    TrackInfo track;
    float outerWidth = 0.0f;
    float innerWidth = 0.0f;

    if (!parseFile(filename, track, outerWidth, innerWidth)) {
        std::cerr << "[TrackLoader] Nie udalo sie wczytac mapy lub plik jest pusty: " << filename << std::endl;
        return track; 
    }

    generateBoundaries(track, outerWidth, innerWidth);

    return track;
}

bool TrackLoader::parseFile(const std::string& filename, TrackInfo& track, float& outWidth, float& inWidth) const {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    if (!(file >> outWidth >> inWidth)) return false;

    float x, y;
    while (file >> x >> y) {
        track.optimalRacingLine.push_back({x, y});
    }

    return track.optimalRacingLine.size() >= 2;
}

void TrackLoader::generateBoundaries(TrackInfo& track, float outWidth, float inWidth) const {
    const auto& centerline = track.optimalRacingLine;
    const int n = centerline.size();

    for (int i = 0; i < n; ++i) {
        const int prev_idx = (i - 1 + n) % n;
        const int next_idx = (i + 1) % n;

        const Vector2D normal = calculateOutwardNormal(centerline[prev_idx], centerline[next_idx]);

        Vector2D outerPoint(centerline[i].x + (normal.x * outWidth), centerline[i].y + (normal.y * outWidth));
        Vector2D innerPoint(centerline[i].x - (normal.x * inWidth),  centerline[i].y - (normal.y * inWidth));

        track.outerBoundaries.push_back(outerPoint);
        track.innerBoundaries.push_back(innerPoint);
    }
}