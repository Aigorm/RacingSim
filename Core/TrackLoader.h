#pragma once
#include <string>
#include "../Shared/Telemetry.h"

class TrackLoader {
private:    
    bool parseFile(const std::string& filename, TrackInfo& track, float& outWidth, float& inWidth) const;
    
    void generateBoundaries(TrackInfo& track, float outWidth, float inWidth) const;

public:
    TrackLoader() = default;
    ~TrackLoader() = default;

    TrackInfo loadFromTxt(const std::string& filename) const;
};