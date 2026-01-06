#pragma once

#include "TerritoryData.h"
#include "MapView.h"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

class Application {
public:
    Application();
    ~Application();
    
    bool Initialize();
    void Shutdown();
    void Run();
    
private:
    void RenderUI();
    void RenderTerritoryHierarchy();
    void RenderMapView();
    void RenderInspector();
    
    void HandleInput();
    void ClearSelection();
    void SelectZone(Zone* zone, bool addToSelection = false);
    void DeleteSelectedZones();
    void ShowAddZoneDialog(float worldX, float worldZ);
    void SaveUndoState();
    void Undo();
    void OpenFileDialog();
    
    TerritoryData territoryData_;
    MapView mapView_;
    
    std::string currentFilePath_;
    bool fileLoaded_ = false;
    
    // Selection
    std::vector<Zone*> selectedZones_;
    Territory* selectedTerritory_ = nullptr;
    
    // UI State
    char searchFilter_[256] = "";
    int selectedMapIndex_ = 0;
    std::vector<MapInfo> availableMaps_;
    bool dockingInitialized_ = false;
    
    // Batch edit
    char batchEditValue_[64] = "";
    int batchEditField_ = 0; // 0=smin, 1=smax, 2=smin+smax, 3=dmin, 4=dmax, 5=dmin+dmax, 6=r
    bool batchEditPercentage_ = false;
    
    // Undo system
    std::vector<TerritoryData> undoStack_;
    static const size_t MAX_UNDO_STACK_SIZE = 50;
    
    // Add zone dialog
    bool showAddZoneDialog_ = false;
    float newZoneX_ = 0.0f;
    float newZoneZ_ = 0.0f;
    int selectedTerritoryTypeIndex_ = 0;
    std::vector<std::string> availableTerritoryTypes_;
    
    // Zone dragging
    Zone* draggingZone_ = nullptr;
    float dragClickWorldX_ = 0.0f;
    float dragClickWorldZ_ = 0.0f;
    bool isDraggingZone_ = false;
    std::map<Zone*, std::pair<float, float>> dragOriginalPositions_;
    
    GLFWwindow* window_ = nullptr;
};

