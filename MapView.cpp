#include "MapView.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <algorithm>
#include <cmath>

// OpenGL function declarations (avoiding Windows GL/gl.h conflicts)
// These functions are provided by opengl32.dll on Windows
#ifdef _WIN32
extern "C" {
    typedef unsigned int GLuint;
    typedef unsigned int GLenum;
    typedef int GLint;
    typedef int GLsizei;
    
    void __stdcall glGenTextures(GLsizei n, GLuint* textures);
    void __stdcall glDeleteTextures(GLsizei n, const GLuint* textures);
    void __stdcall glBindTexture(GLenum target, GLuint texture);
    void __stdcall glTexParameteri(GLenum target, GLenum pname, GLint param);
    void __stdcall glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
    
    #define GL_TEXTURE_2D         0x0DE1
    #define GL_TEXTURE_MIN_FILTER 0x2801
    #define GL_TEXTURE_MAG_FILTER 0x2800
    #define GL_TEXTURE_WRAP_S     0x2802
    #define GL_TEXTURE_WRAP_T     0x2803
    #define GL_LINEAR             0x2601
    #define GL_CLAMP_TO_EDGE      0x812F
    #define GL_RGBA               0x1908
    #define GL_UNSIGNED_BYTE      0x1401
}
#else
#include <GL/gl.h>
#endif

MapView::MapView() {
    currentMap_.name = "Chernarus";
    currentMap_.worldSizeX = 15360.0f;
    currentMap_.worldSizeZ = 15360.0f;
}

MapView::~MapView() {
    if (currentMap_.textureId != 0) {
        glDeleteTextures(1, &currentMap_.textureId);
    }
}

bool MapView::LoadMapImage(const std::string& imagePath) {
    int width, height, channels;
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        return false;
    }
    
    // Delete old texture if exists
    if (currentMap_.textureId != 0) {
        glDeleteTextures(1, &currentMap_.textureId);
    }
    
    // Create new texture
    glGenTextures(1, &currentMap_.textureId);
    glBindTexture(GL_TEXTURE_2D, currentMap_.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    
    currentMap_.imageWidth = width;
    currentMap_.imageHeight = height;
    currentMap_.imagePath = imagePath;
    
    return true;
}

void MapView::SetMapInfo(const MapInfo& info) {
    currentMap_ = info;
    // Load the map image if path is provided
    if (!info.imagePath.empty()) {
        LoadMapImage(info.imagePath);
    }
}

void MapView::Update(float deltaTime) {
    // Update logic if needed
}

void MapView::Render(const TerritoryData& data, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Draw map background - transform with zoom/pan like zones
    if (currentMap_.textureId != 0) {
        // Transform the four corners of the world map to screen coordinates
        // World coordinates: (0,0) = bottom-left, (worldSizeX, worldSizeZ) = top-right
        // Image UV: (0,0) = top-left, (1,1) = bottom-right
        ImVec2 bottomLeft = WorldToScreen(0.0f, 0.0f, canvasPos, canvasSize);
        ImVec2 bottomRight = WorldToScreen(currentMap_.worldSizeX, 0.0f, canvasPos, canvasSize);
        ImVec2 topLeft = WorldToScreen(0.0f, currentMap_.worldSizeZ, canvasPos, canvasSize);
        ImVec2 topRight = WorldToScreen(currentMap_.worldSizeX, currentMap_.worldSizeZ, canvasPos, canvasSize);
        
        // Use AddImageQuad to draw the transformed quad
        // UV coordinates: (0,0) top-left, (1,0) top-right, (1,1) bottom-right, (0,1) bottom-left
        ImVec2 uv0(0.0f, 0.0f); // top-left
        ImVec2 uv1(1.0f, 0.0f); // top-right
        ImVec2 uv2(1.0f, 1.0f); // bottom-right
        ImVec2 uv3(0.0f, 1.0f); // bottom-left
        
        drawList->AddImageQuad(
            reinterpret_cast<void*>(static_cast<intptr_t>(currentMap_.textureId)),
            topLeft, topRight, bottomRight, bottomLeft,
            uv0, uv1, uv2, uv3
        );
    } else {
        // Draw placeholder background
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(40, 40, 40, 255));
    }
    
    // Draw zones
    for (const auto& territory : data.territories) {
        if (!territory.visible) continue;
        
        for (const auto& zone : territory.zones) {
            if (!zone.visible) continue;
            DrawZone(zone, territory, canvasPos, canvasSize);
        }
    }
    
    // Draw marquee selection
    if (isMarqueeSelecting_) {
        DrawMarquee(canvasPos, canvasSize);
    }
}

ImVec2 MapView::WorldToScreen(float worldX, float worldZ, const ImVec2& canvasPos, const ImVec2& canvasSize) const {
    float scaleX = canvasSize.x / currentMap_.worldSizeX;
    float scaleZ = canvasSize.y / currentMap_.worldSizeZ;
    
    // X increases left to right (correct)
    float screenX = (worldX * scaleX * zoom_) + panX_ + canvasPos.x;
    // Z increases bottom to top in world, but Y increases top to bottom on screen (need to flip)
    float screenY = ((currentMap_.worldSizeZ - worldZ) * scaleZ * zoom_) + panY_ + canvasPos.y;
    
    return ImVec2(screenX, screenY);
}

ImVec2 MapView::ScreenToWorld(float screenX, float screenY, const ImVec2& canvasPos, const ImVec2& canvasSize) const {
    float scaleX = currentMap_.worldSizeX / canvasSize.x;
    float scaleZ = currentMap_.worldSizeZ / canvasSize.y;
    
    // X increases left to right (correct)
    float worldX = ((screenX - canvasPos.x - panX_) / zoom_) * scaleX;
    // Z increases bottom to top in world, but Y increases top to bottom on screen (need to flip)
    float worldZ = currentMap_.worldSizeZ - (((screenY - canvasPos.y - panY_) / zoom_) * scaleZ);
    
    return ImVec2(worldX, worldZ);
}

void MapView::Pan(float deltaX, float deltaY) {
    panX_ += deltaX;
    panY_ += deltaY;
}

void MapView::Zoom(float delta, float mouseX, float mouseY, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    float oldZoom = zoom_;
    
    // Get world position before zoom
    ImVec2 mouseWorld = ScreenToWorld(mouseX + canvasPos.x, mouseY + canvasPos.y, canvasPos, canvasSize);
    
    // Apply zoom
    zoom_ = std::max(0.1f, std::min(10.0f, zoom_ + delta * 0.1f));
    
    // Adjust pan to keep mouse position fixed
    float scaleX = canvasSize.x / currentMap_.worldSizeX;
    float scaleZ = canvasSize.y / currentMap_.worldSizeZ;
    
    ImVec2 newScreenPos = WorldToScreen(mouseWorld.x, mouseWorld.y, canvasPos, canvasSize);
    panX_ += (mouseX + canvasPos.x) - newScreenPos.x;
    panY_ += (mouseY + canvasPos.y) - newScreenPos.y;
}

void MapView::ResetView() {
    panX_ = 0.0f;
    panY_ = 0.0f;
    zoom_ = 1.0f;
}

void MapView::StartMarqueeSelection(float x, float y) {
    isMarqueeSelecting_ = true;
    marqueeStart_ = ImVec2(x, y);
    marqueeEnd_ = marqueeStart_;
}

void MapView::UpdateMarqueeSelection(float x, float y) {
    if (isMarqueeSelecting_) {
        marqueeEnd_ = ImVec2(x, y);
    }
}

void MapView::EndMarqueeSelection() {
    isMarqueeSelecting_ = false;
}

std::vector<Zone*> MapView::GetZonesInRect(TerritoryData& data, float x1, float y1, float x2, float y2, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    std::vector<Zone*> result;
    
    float minX = std::min(x1, x2);
    float maxX = std::max(x1, x2);
    float minY = std::min(y1, y2);
    float maxY = std::max(y1, y2);
    
    for (auto& territory : data.territories) {
        for (auto& zone : territory.zones) {
            ImVec2 screenPos = WorldToScreen(zone.x, zone.z, canvasPos, canvasSize);
            if (screenPos.x >= minX && screenPos.x <= maxX &&
                screenPos.y >= minY && screenPos.y <= maxY) {
                result.push_back(&zone);
            }
        }
    }
    
    return result;
}

void MapView::DrawZone(const Zone& zone, const Territory& territory, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    ImVec2 center = WorldToScreen(zone.x, zone.z, canvasPos, canvasSize);
    
    // Calculate radius in screen space
    float scaleX = canvasSize.x / currentMap_.worldSizeX;
    float scaleZ = canvasSize.y / currentMap_.worldSizeZ;
    float radius = zone.r * std::min(scaleX, scaleZ) * zoom_;
    
    // Convert territory color from ARGB to RGBA
    uint32_t color = territory.color;
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    // Make zones much more visible - use full alpha for selected, high alpha for unselected
    uint32_t imguiColor = IM_COL32(r, g, b, zone.selected ? 255 : 200);
    
    // Draw circle outline
    drawList->AddCircle(center, radius, imguiColor, 32, zone.selected ? 3.0f : 1.0f);
    
    // Draw center point
    drawList->AddCircleFilled(center, 3.0f, imguiColor);
}

void MapView::DrawMarquee(const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Convert relative coordinates to absolute
    ImVec2 start = ImVec2(marqueeStart_.x + canvasPos.x, marqueeStart_.y + canvasPos.y);
    ImVec2 end = ImVec2(marqueeEnd_.x + canvasPos.x, marqueeEnd_.y + canvasPos.y);
    
    ImVec2 min = ImVec2(std::min(start.x, end.x), std::min(start.y, end.y));
    ImVec2 max = ImVec2(std::max(start.x, end.x), std::max(start.y, end.y));
    
    drawList->AddRect(min, max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
    drawList->AddRectFilledMultiColor(
        min, max,
        IM_COL32(255, 255, 0, 30),
        IM_COL32(255, 255, 0, 30),
        IM_COL32(255, 255, 0, 30),
        IM_COL32(255, 255, 0, 30)
    );
}

