#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct Zone {
    std::string name;
    int smin = 0;
    int smax = 0;
    int dmin = 0;
    int dmax = 0;
    float x = 0.0f;
    float z = 0.0f;
    float r = 0.0f;  // radius
    float h = 0.0f;  // height (optional)
    
    bool selected = false;
    bool visible = true;
};

struct Territory {
    std::string name;  // Will be extracted from zone names or set to "Territory N"
    uint32_t color = 0xFFFFFFFF;
    std::vector<Zone> zones;
    
    bool visible = true;
    bool expanded = true;
};

struct TerritoryData {
    std::vector<Territory> territories;
    
    void clear() {
        territories.clear();
    }
    
    size_t getTotalZoneCount() const {
        size_t count = 0;
        for (const auto& territory : territories) {
            count += territory.zones.size();
        }
        return count;
    }
};

