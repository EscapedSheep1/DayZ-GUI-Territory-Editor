#pragma once

#include "TerritoryData.h"
#include <string>

class TerritoryParser {
public:
    static bool LoadFromFile(const std::string& filepath, TerritoryData& data);
    static bool SaveToFile(const std::string& filepath, const TerritoryData& data);
    
private:
    static std::string ExtractTerritoryName(const Territory& territory);
};

