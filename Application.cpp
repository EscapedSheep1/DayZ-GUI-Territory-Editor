#include "Application.h"
#include "TerritoryParser.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <set>
#include <map>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

Application::Application() {
    // Initialize available maps
    MapInfo chernarus;
    chernarus.name = "Chernarus";
    chernarus.worldSizeX = 15360.0f;
    chernarus.worldSizeZ = 15360.0f;
    chernarus.imagePath = "Maps/chernarusplus_Map.png";
    availableMaps_.push_back(chernarus);
    
    MapInfo livonia;
    livonia.name = "Livonia";
    livonia.worldSizeX = 12800.0f;
    livonia.worldSizeZ = 12800.0f;
    availableMaps_.push_back(livonia);
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window_ = glfwCreateWindow(1920, 1080, "DayZ Territory Editor", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // VSync
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Dark theme
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Load the default map image
    if (!availableMaps_.empty()) {
        mapView_.SetMapInfo(availableMaps_[0]);
        if (!availableMaps_[0].imagePath.empty()) {
            mapView_.LoadMapImage(availableMaps_[0].imagePath);
        }
    }
    
    return true;
}

void Application::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

void Application::Run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Enable docking
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();
        
        // Set up initial docking layout on first run
        if (!dockingInitialized_) {
            dockingInitialized_ = true;
            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any existing layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
            
            // Split the dockspace
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
            ImGuiID dock_id_center = dockspace_id; // The remaining center area
            
            // Dock the windows
            ImGui::DockBuilderDockWindow("Territory Hierarchy", dock_id_left);
            ImGui::DockBuilderDockWindow("Map View", dock_id_center);
            ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
            
            ImGui::DockBuilderFinish(dockspace_id);
        }
        
        // Render menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    OpenFileDialog();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    if (!currentFilePath_.empty()) {
                        TerritoryParser::SaveToFile(currentFilePath_, territoryData_);
                    }
                }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                    // TODO: File dialog
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window_, true);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        HandleInput();
        RenderUI();
        
        // Render
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(window_, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        
        glfwSwapBuffers(window_);
    }
}

void Application::RenderUI() {
    // Left panel - Territory Hierarchy
    ImGui::Begin("Territory Hierarchy");
    RenderTerritoryHierarchy();
    ImGui::End();
    
    // Center panel - Map View
    ImGui::Begin("Map View");
    RenderMapView();
    ImGui::End();
    
    // Right panel - Inspector
    ImGui::Begin("Inspector");
    RenderInspector();
    ImGui::End();
    
    // Add Zone Dialog
    if (showAddZoneDialog_) {
        ImGui::OpenPopup("Add New Zone");
        showAddZoneDialog_ = false;
    }
    
    if (ImGui::BeginPopupModal("Add New Zone", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Position: (%.2f, %.2f)", newZoneX_, newZoneZ_);
        ImGui::Separator();
        
        if (!availableTerritoryTypes_.empty()) {
            // Create array of const char* for Combo
            std::vector<const char*> typeNames;
            for (const auto& type : availableTerritoryTypes_) {
                typeNames.push_back(type.c_str());
            }
            
            ImGui::Text("Territory Type:");
            ImGui::Combo("##type", &selectedTerritoryTypeIndex_, typeNames.data(), static_cast<int>(typeNames.size()));
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Create")) {
            if (!availableTerritoryTypes_.empty() && selectedTerritoryTypeIndex_ < availableTerritoryTypes_.size()) {
                // Find or create territory with this name
                Territory* targetTerritory = nullptr;
                for (auto& territory : territoryData_.territories) {
                    if (territory.name == availableTerritoryTypes_[selectedTerritoryTypeIndex_]) {
                        targetTerritory = &territory;
                        break;
                    }
                }
                
                // Create new territory if it doesn't exist
                if (!targetTerritory) {
                    Territory newTerritory;
                    newTerritory.name = availableTerritoryTypes_[selectedTerritoryTypeIndex_];
                    newTerritory.color = 0xFFFFFFFF; // Default white color
                    territoryData_.territories.push_back(newTerritory);
                    targetTerritory = &territoryData_.territories.back();
                }
                
                // Create new zone
                SaveUndoState();
                
                Zone newZone;
                newZone.name = targetTerritory->name;
                newZone.x = newZoneX_;
                newZone.z = newZoneZ_;
                newZone.r = 50.0f; // Default radius
                newZone.smin = 0;
                newZone.smax = 0;
                newZone.dmin = 1;
                newZone.dmax = 3;
                
                targetTerritory->zones.push_back(newZone);
            }
            
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void Application::RenderTerritoryHierarchy() {
    // Search filter
    ImGui::InputText("Search", searchFilter_, sizeof(searchFilter_));
    ImGui::Separator();
    
    // Map selector
    if (ImGui::Combo("Map", &selectedMapIndex_, [](void* data, int idx, const char** out_text) {
        auto* maps = static_cast<std::vector<MapInfo>*>(data);
        *out_text = maps->at(idx).name.c_str();
        return true;
    }, &availableMaps_, availableMaps_.size())) {
        if (selectedMapIndex_ < availableMaps_.size()) {
            mapView_.SetMapInfo(availableMaps_[selectedMapIndex_]);
            // SetMapInfo will automatically load the image if path is set
        }
    }
    
    ImGui::Separator();
    
    // Territory list
    ImGui::BeginChild("TerritoryList", ImVec2(0, 0), false);
    
    for (size_t i = 0; i < territoryData_.territories.size(); ++i) {
        auto& territory = territoryData_.territories[i];
        
        // Filter check
        bool show = true;
        if (searchFilter_[0] != '\0') {
            show = territory.name.find(searchFilter_) != std::string::npos;
            if (!show) {
                for (const auto& zone : territory.zones) {
                    if (zone.name.find(searchFilter_) != std::string::npos) {
                        show = true;
                        break;
                    }
                }
            }
        }
        
        if (!show) continue;
        
        // Push unique ID for this territory to avoid conflicts when names are the same
        ImGui::PushID(static_cast<int>(i));
        
        // Territory visibility checkbox
        ImGui::Checkbox("##vis", &territory.visible);
        ImGui::SameLine();
        
        // Territory name (clickable)
        bool isSelected = (selectedTerritory_ == &territory);
        if (ImGui::Selectable(territory.name.c_str(), isSelected, 0, ImVec2(0, 0))) {
            ClearSelection();
            selectedTerritory_ = &territory;
            // Select all zones in this territory
            for (auto& zone : territory.zones) {
                SelectZone(&zone, true);
            }
        }
        
        // Zone count
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", territory.zones.size());
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

void Application::RenderMapView() {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;
    
    // Helper function to find zone at world position
    auto findZoneAt = [&](float worldX, float worldZ) -> Zone* {
        Zone* closestZone = nullptr;
        float closestDist = 1000.0f; // Max selection distance in world units
        
        for (auto& territory : territoryData_.territories) {
            if (!territory.visible) continue;
            for (auto& zone : territory.zones) {
                if (!zone.visible) continue;
                float dx = zone.x - worldX;
                float dz = zone.z - worldZ;
                float dist = std::sqrt(dx * dx + dz * dz);
                
                if (dist < zone.r && dist < closestDist) {
                    closestDist = dist;
                    closestZone = &zone;
                }
            }
        }
        return closestZone;
    };
    
    // Draw canvas first (needed for ImGui to capture mouse input)
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    bool isHovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();
    
    // Mouse wheel for zooming
    if (isHovered && io.MouseWheel != 0.0f) {
        ImVec2 mousePos = io.MousePos;
        ImVec2 relativeMouse = ImVec2(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        mapView_.Zoom(io.MouseWheel, relativeMouse.x, relativeMouse.y, canvasPos, canvasSize);
    }
    
    // Middle mouse drag for panning
    if (isHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        mapView_.Pan(delta.x, delta.y);
    }
    
    // Double-click to add new zone
    if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !io.KeyShift && !isDraggingZone_ && !mapView_.IsMarqueeSelecting()) {
        ImVec2 mousePos = io.MousePos;
        ImVec2 worldPos = mapView_.ScreenToWorld(mousePos.x, mousePos.y, canvasPos, canvasSize);
        ShowAddZoneDialog(worldPos.x, worldPos.y);  // y contains the Z coordinate
    }
    
    // Right-click drag for marquee selection
    if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImVec2 mousePos = io.MousePos;
            mapView_.StartMarqueeSelection(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        } else if (mapView_.IsMarqueeSelecting()) {
            ImVec2 mousePos = io.MousePos;
            mapView_.UpdateMarqueeSelection(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        }
    } else if (mapView_.IsMarqueeSelecting() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImVec2 start = mapView_.GetMarqueeStart();
        ImVec2 end = mapView_.GetMarqueeEnd();
        float x1 = start.x + canvasPos.x;
        float y1 = start.y + canvasPos.y;
        float x2 = end.x + canvasPos.x;
        float y2 = end.y + canvasPos.y;
        auto zones = mapView_.GetZonesInRect(territoryData_, x1, y1, x2, y2, canvasPos, canvasSize);
        ClearSelection();
        for (auto* zone : zones) {
            SelectZone(zone, true);
        }
        mapView_.EndMarqueeSelection();
    }
    
    // Left-click handling - zone selection/dragging or marquee selection
    if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = io.MousePos;
        ImVec2 worldPos = mapView_.ScreenToWorld(mousePos.x, mousePos.y, canvasPos, canvasSize);
        
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            Zone* zoneAtPos = findZoneAt(worldPos.x, worldPos.y);
            
            if (zoneAtPos && zoneAtPos->selected) {
                // Starting to drag a selected zone - save undo state
                SaveUndoState();
                isDraggingZone_ = true;
                draggingZone_ = zoneAtPos;
                dragClickWorldX_ = worldPos.x;
                dragClickWorldZ_ = worldPos.y;
                dragOriginalPositions_.clear();
                for (auto* zone : selectedZones_) {
                    dragOriginalPositions_[zone] = {zone->x, zone->z};
                }
            } else if (io.KeyShift || !zoneAtPos) {
                // Starting marquee selection (shift+click or click on empty space)
                mapView_.StartMarqueeSelection(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
            } else if (zoneAtPos) {
                // Clicking on unselected zone - select it
                if (!io.KeyCtrl) {
                    ClearSelection();
                }
                SelectZone(zoneAtPos, true);
            }
        } else if (isDraggingZone_ && draggingZone_) {
            // Dragging a zone
            ImVec2 currentWorldPos = mapView_.ScreenToWorld(mousePos.x, mousePos.y, canvasPos, canvasSize);
            float moveDeltaX = currentWorldPos.x - dragClickWorldX_;
            float moveDeltaZ = currentWorldPos.y - dragClickWorldZ_;
            
            for (auto* zone : selectedZones_) {
                auto it = dragOriginalPositions_.find(zone);
                if (it != dragOriginalPositions_.end()) {
                    zone->x = it->second.first + moveDeltaX;
                    zone->z = it->second.second + moveDeltaZ;
                }
            }
        } else if (mapView_.IsMarqueeSelecting()) {
            // Update marquee selection
            ImVec2 mousePos = io.MousePos;
            mapView_.UpdateMarqueeSelection(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        }
    } else if (isDraggingZone_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDraggingZone_ = false;
        draggingZone_ = nullptr;
        dragOriginalPositions_.clear();
    } else if (mapView_.IsMarqueeSelecting() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        ImVec2 start = mapView_.GetMarqueeStart();
        ImVec2 end = mapView_.GetMarqueeEnd();
        float x1 = start.x + canvasPos.x;
        float y1 = start.y + canvasPos.y;
        float x2 = end.x + canvasPos.x;
        float y2 = end.y + canvasPos.y;
        auto zones = mapView_.GetZonesInRect(territoryData_, x1, y1, x2, y2, canvasPos, canvasSize);
        if (!io.KeyCtrl) {
            ClearSelection();
        }
        for (auto* zone : zones) {
            SelectZone(zone, true);
        }
        mapView_.EndMarqueeSelection();
    }
    
    // Render map and zones
    mapView_.Render(territoryData_, canvasPos, canvasSize);
    
    // Reset view button
    if (ImGui::Button("Reset View")) {
        mapView_.ResetView();
    }
}

void Application::RenderInspector() {
    // Undo button at the top
    if (ImGui::Button("Undo")) {
        Undo();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu)", undoStack_.size());
    
    ImGui::Separator();
    
    if (selectedZones_.empty() && selectedTerritory_ == nullptr) {
        ImGui::Text("No selection");
        return;
    }
    
    // Show selected count
    if (!selectedZones_.empty()) {
        ImGui::Text("Selected: %zu zone(s)", selectedZones_.size());
        ImGui::Separator();
        
        // Batch editing
        ImGui::Text("Batch Edit:");
        const char* fields[] = { "smin", "smax", "smin+smax", "dmin", "dmax", "dmin+dmax", "radius" };
        ImGui::Combo("Field", &batchEditField_, fields, IM_ARRAYSIZE(fields));
        ImGui::Checkbox("Percentage", &batchEditPercentage_);
        ImGui::InputText("Value", batchEditValue_, sizeof(batchEditValue_));
        
        if (ImGui::Button("Apply")) {
            SaveUndoState();
            float value = std::stof(batchEditValue_);
            
            for (auto* zone : selectedZones_) {
                if (batchEditPercentage_) {
                    float multiplier = 1.0f + (value / 100.0f);
                    switch (batchEditField_) {
                        case 0: zone->smin = static_cast<int>(zone->smin * multiplier); break;
                        case 1: zone->smax = static_cast<int>(zone->smax * multiplier); break;
                        case 2: 
                            zone->smin = static_cast<int>(zone->smin * multiplier);
                            zone->smax = static_cast<int>(zone->smax * multiplier);
                            break;
                        case 3: zone->dmin = static_cast<int>(zone->dmin * multiplier); break;
                        case 4: zone->dmax = static_cast<int>(zone->dmax * multiplier); break;
                        case 5:
                            zone->dmin = static_cast<int>(zone->dmin * multiplier);
                            zone->dmax = static_cast<int>(zone->dmax * multiplier);
                            break;
                        case 6: zone->r = zone->r * multiplier; break;
                    }
                } else {
                    switch (batchEditField_) {
                        case 0: zone->smin = static_cast<int>(value); break;
                        case 1: zone->smax = static_cast<int>(value); break;
                        case 2:
                            zone->smin = static_cast<int>(value);
                            zone->smax = static_cast<int>(value);
                            break;
                        case 3: zone->dmin = static_cast<int>(value); break;
                        case 4: zone->dmax = static_cast<int>(value); break;
                        case 5:
                            zone->dmin = static_cast<int>(value);
                            zone->dmax = static_cast<int>(value);
                            break;
                        case 6: zone->r = value; break;
                    }
                }
            }
        }
        
        ImGui::Separator();
        
        // Show properties of first selected zone (or average if multiple)
        if (selectedZones_.size() == 1) {
            Zone* zone = selectedZones_[0];
            ImGui::Text("Zone: %s", zone->name.c_str());
            ImGui::InputInt("smin", &zone->smin);
            ImGui::InputInt("smax", &zone->smax);
            ImGui::InputInt("dmin", &zone->dmin);
            ImGui::InputInt("dmax", &zone->dmax);
            ImGui::InputFloat("X", &zone->x);
            ImGui::InputFloat("Z", &zone->z);
            
            // Radius with +/- buttons
            ImGui::PushID("Radius");
            ImGui::InputFloat("Radius", &zone->r);
            ImGui::SameLine();
            ImGuiIO& io = ImGui::GetIO();
            float step = io.KeyShift ? 20.0f : 5.0f;
            if (ImGui::Button("-")) {
                SaveUndoState();
                zone->r = zone->r - step;
                if (zone->r < 0.0f) zone->r = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("+")) {
                SaveUndoState();
                zone->r += step;
            }
            ImGui::PopID();
            
            ImGui::InputFloat("Height", &zone->h);
        } else {
            // Show averages
            float avgX = 0, avgZ = 0, avgR = 0;
            for (auto* zone : selectedZones_) {
                avgX += zone->x;
                avgZ += zone->z;
                avgR += zone->r;
            }
            avgX /= selectedZones_.size();
            avgZ /= selectedZones_.size();
            avgR /= selectedZones_.size();
            
            ImGui::Text("Average Position: (%.2f, %.2f)", avgX, avgZ);
            ImGui::Text("Average Radius: %.2f", avgR);
        }
    }
    
    if (selectedTerritory_) {
        ImGui::Separator();
        ImGui::Text("Territory: %s", selectedTerritory_->name.c_str());
        ImGui::Text("Zones: %zu", selectedTerritory_->zones.size());
    }
}

void Application::HandleInput() {
    // Handle keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            OpenFileDialog();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (io.KeyShift) {
                // TODO: Save As
            } else {
                if (!currentFilePath_.empty()) {
                    TerritoryParser::SaveToFile(currentFilePath_, territoryData_);
                }
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z) && !io.WantTextInput) {
            Undo();
        }
    }
    
    // Delete key to delete selected zones (only if not typing in a text field)
    if (!io.WantTextInput && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))) {
        DeleteSelectedZones();
    }
}

void Application::ClearSelection() {
    for (auto* zone : selectedZones_) {
        zone->selected = false;
    }
    selectedZones_.clear();
    selectedTerritory_ = nullptr;
}

void Application::SelectZone(Zone* zone, bool addToSelection) {
    if (!addToSelection) {
        ClearSelection();
    }
    
    if (zone && !zone->selected) {
        zone->selected = true;
        selectedZones_.push_back(zone);
    }
}

void Application::DeleteSelectedZones() {
    if (selectedZones_.empty()) {
        return;
    }
    
    SaveUndoState();
    
    // Remove selected zones from their territories
    for (auto it = territoryData_.territories.begin(); it != territoryData_.territories.end();) {
        auto& territory = *it;
        auto zoneIt = territory.zones.begin();
        
        while (zoneIt != territory.zones.end()) {
            if (zoneIt->selected) {
                zoneIt = territory.zones.erase(zoneIt);
            } else {
                ++zoneIt;
            }
        }
        
        // Remove territory if it has no zones left
        if (territory.zones.empty()) {
            if (selectedTerritory_ == &territory) {
                selectedTerritory_ = nullptr;
            }
            it = territoryData_.territories.erase(it);
        } else {
            ++it;
        }
    }
    
    ClearSelection();
}

void Application::SaveUndoState() {
    // Store current state in undo stack
    undoStack_.push_back(territoryData_);
    
    // Limit stack size
    if (undoStack_.size() > MAX_UNDO_STACK_SIZE) {
        undoStack_.erase(undoStack_.begin());
    }
}

void Application::Undo() {
    if (undoStack_.empty()) {
        return;
    }
    
    // Restore previous state
    territoryData_ = undoStack_.back();
    undoStack_.pop_back();
    
    // Clear selection since zone pointers are now invalid
    ClearSelection();
}

void Application::OpenFileDialog() {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(window_);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "XML Files\0*.xml\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        std::string filePath = ofn.lpstrFile;
        if (TerritoryParser::LoadFromFile(filePath, territoryData_)) {
            currentFilePath_ = filePath;
            fileLoaded_ = true;
            ClearSelection();
            std::cout << "Loaded " << territoryData_.getTotalZoneCount() << " zones from " << filePath << std::endl;
        } else {
            std::cerr << "Failed to load file: " << filePath << std::endl;
        }
    }
#else
    // For non-Windows platforms, fall back to default file
    std::string filePath = "Example Territory Files/zombie_territories.xml";
    if (TerritoryParser::LoadFromFile(filePath, territoryData_)) {
        currentFilePath_ = filePath;
        fileLoaded_ = true;
        ClearSelection();
        std::cout << "Loaded " << territoryData_.getTotalZoneCount() << " zones from " << filePath << std::endl;
    } else {
        std::cerr << "Failed to load file: " << filePath << std::endl;
    }
#endif
}

void Application::ShowAddZoneDialog(float worldX, float worldZ) {
    newZoneX_ = worldX;
    newZoneZ_ = worldZ;
    
    // Collect unique territory type names from existing territories
    availableTerritoryTypes_.clear();
    std::set<std::string> uniqueTypes;
    for (const auto& territory : territoryData_.territories) {
        if (!territory.name.empty()) {
            uniqueTypes.insert(territory.name);
        }
    }
    
    availableTerritoryTypes_.assign(uniqueTypes.begin(), uniqueTypes.end());
    if (availableTerritoryTypes_.empty()) {
        availableTerritoryTypes_.push_back("NewTerritory");
    }
    
    selectedTerritoryTypeIndex_ = 0;
    showAddZoneDialog_ = true;
}

