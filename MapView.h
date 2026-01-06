#pragma once

#include "TerritoryData.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <memory>

struct MapInfo {
    std::string name;
    std::string imagePath;
    float worldSizeX = 15360.0f;  // Chernarus world size
    float worldSizeZ = 15360.0f;
    int imageWidth = 0;
    int imageHeight = 0;
    unsigned int textureId = 0;
};

class MapView {
public:
    MapView();
    ~MapView();
    
    bool LoadMapImage(const std::string& imagePath);
    void SetMapInfo(const MapInfo& info);
    const MapInfo& GetCurrentMap() const { return currentMap_; }
    
    void Update(float deltaTime);
    void Render(const TerritoryData& data, const ImVec2& canvasPos, const ImVec2& canvasSize);
    
    // Coordinate conversion
    ImVec2 WorldToScreen(float worldX, float worldZ, const ImVec2& canvasPos, const ImVec2& canvasSize) const;
    ImVec2 ScreenToWorld(float screenX, float screenY, const ImVec2& canvasPos, const ImVec2& canvasSize) const;
    
    // Navigation
    void Pan(float deltaX, float deltaY);
    void Zoom(float delta, float mouseX, float mouseY, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void ResetView();
    
    // Selection
    void StartMarqueeSelection(float x, float y);
    void UpdateMarqueeSelection(float x, float y);
    void EndMarqueeSelection();
    bool IsMarqueeSelecting() const { return isMarqueeSelecting_; }
    ImVec2 GetMarqueeStart() const { return marqueeStart_; }
    ImVec2 GetMarqueeEnd() const { return marqueeEnd_; }
    
    // Zone selection
    std::vector<Zone*> GetZonesInRect(TerritoryData& data, float x1, float y1, float x2, float y2, const ImVec2& canvasPos, const ImVec2& canvasSize);
    
private:
    MapInfo currentMap_;
    
    // View state
    float panX_ = 0.0f;
    float panY_ = 0.0f;
    float zoom_ = 1.0f;
    
    // Marquee selection
    bool isMarqueeSelecting_ = false;
    ImVec2 marqueeStart_;
    ImVec2 marqueeEnd_;
    
    void DrawZone(const Zone& zone, const Territory& territory, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void DrawMarquee(const ImVec2& canvasPos, const ImVec2& canvasSize);
};

