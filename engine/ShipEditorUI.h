#pragma once

#include "ShipBuilder.h"
#include <memory>
#include <string>

/**
 * ShipEditorUI - ImGui-based visual ship editor
 * 
 * Features:
 * - Drag-and-drop component installation
 * - 2D ship layout visualization
 * - Real-time performance metrics display
 * - Component compatibility warnings
 * - Preset loadout selection
 * - Ship customization panel
 * - Hangar management interface
 */

namespace ShipBuilding {

class ShipEditorUI {
public:
    ShipEditorUI(ShipBuilder* builder);
    ~ShipEditorUI();
    
    /**
     * Main render function - call once per frame
     */
    void Render();
    
    /**
     * Open the ship editor for a specific ship
     */
    void OpenEditor(std::shared_ptr<ShipLoadout> ship);
    
    /**
     * Close the ship editor
     */
    void CloseEditor();
    
    /**
     * Check if editor is open
     */
    bool IsOpen() const { return isOpen_; }
    
    /**
     * Set current player ID for hangar management
     */
    void SetPlayerId(const std::string& playerId) { playerId_ = playerId; }

private:
    // UI Rendering functions
    void RenderMainWindow();
    void RenderHangarPanel();
    void RenderHullSelector();
    void RenderShipLayout();
    void RenderComponentCatalog();
    void RenderHardpointDetails();
    void RenderPerformancePanel();
    void RenderCustomizationPanel();
    void RenderPresetSelector();
    void RenderInsurancePanel();
    void RenderValidationWarnings();
    
    // Drag-and-drop handling
    void HandleComponentDragDrop();
    void BeginComponentDrag(std::shared_ptr<ComponentDefinition> component);
    void AcceptComponentDrop(const std::string& hardpointId);
    
    // Helper functions
    void SelectHardpoint(const std::string& hardpointId);
    void UpdatePerformanceMetrics();
    void ApplyPreset(PresetType preset);
    void SaveCurrentShip();
    void CreateNewShip(const std::string& hullId);
    
    // Visualization
    void DrawShipHardpoints();
    void DrawHardpointIcon(const Hardpoint& hp, float x, float y, float size);
    void DrawComponentIcon(const ComponentDefinition& comp, float size);
    
    // Data
    ShipBuilder* builder_;
    std::shared_ptr<ShipLoadout> currentShip_;
    PerformanceMetrics cachedMetrics_;
    std::string playerId_;
    
    // UI State
    bool isOpen_ = false;
    bool showHangar_ = false;
    bool showHullSelector_ = false;
    bool showPresets_ = false;
    bool showCustomization_ = false;
    bool showInsurance_ = false;
    
    std::string selectedHardpoint_;
    std::shared_ptr<ComponentDefinition> draggedComponent_;
    bool isDragging_ = false;
    
    // Filters
    ComponentType componentFilter_ = ComponentType::Engine;
    std::string searchQuery_;
    
    // Layout
    float shipViewWidth_ = 400.0f;
    float shipViewHeight_ = 400.0f;
    
    // Colors
    struct UIColors {
        float primary[3] = {0.2f, 0.6f, 0.9f};
        float secondary[3] = {0.9f, 0.6f, 0.2f};
        float success[3] = {0.2f, 0.9f, 0.2f};
        float warning[3] = {0.9f, 0.9f, 0.2f};
        float error[3] = {0.9f, 0.2f, 0.2f};
        float background[4] = {0.1f, 0.1f, 0.1f, 0.9f};
    } colors_;
};

} // namespace ShipBuilding
