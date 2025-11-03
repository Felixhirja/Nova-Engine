#include "ShipEditorUI.h"
#include "../lib/imgui/imgui.h"
#include <iostream>
#include <algorithm>

namespace ShipBuilding {

ShipEditorUI::ShipEditorUI(ShipBuilder* builder) 
    : builder_(builder) {
    std::cout << "[ShipEditorUI] Initialized ship editor UI" << std::endl;
}

ShipEditorUI::~ShipEditorUI() = default;

void ShipEditorUI::OpenEditor(std::shared_ptr<ShipLoadout> ship) {
    currentShip_ = ship;
    isOpen_ = true;
    
    if (currentShip_) {
        UpdatePerformanceMetrics();
    }
    
    std::cout << "[ShipEditorUI] Opened ship editor" << std::endl;
}

void ShipEditorUI::CloseEditor() {
    isOpen_ = false;
    currentShip_ = nullptr;
    selectedHardpoint_.clear();
    std::cout << "[ShipEditorUI] Closed ship editor" << std::endl;
}

void ShipEditorUI::Render() {
    if (!isOpen_) return;
    
    RenderMainWindow();
}

void ShipEditorUI::RenderMainWindow() {
    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Ship Editor", &isOpen_, ImGuiWindowFlags_MenuBar)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Ship")) {
                    showHullSelector_ = true;
                }
                if (ImGui::MenuItem("Save Ship")) {
                    SaveCurrentShip();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close")) {
                    CloseEditor();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Hangar", nullptr, &showHangar_);
                ImGui::MenuItem("Presets", nullptr, &showPresets_);
                ImGui::MenuItem("Customization", nullptr, &showCustomization_);
                ImGui::MenuItem("Insurance", nullptr, &showInsurance_);
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Main content
        if (!currentShip_) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "No ship loaded. Create a new ship or load from hangar.");
            if (ImGui::Button("Create New Ship", ImVec2(200, 40))) {
                showHullSelector_ = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Open Hangar", ImVec2(200, 40))) {
                showHangar_ = true;
            }
        } else {
            // Split layout: Ship view on left, component catalog and details on right
            ImGui::BeginChild("LeftPane", ImVec2(600, 0), true);
            {
                RenderShipLayout();
                ImGui::Separator();
                RenderPerformancePanel();
                ImGui::Separator();
                RenderValidationWarnings();
            }
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
            {
                RenderComponentCatalog();
                ImGui::Separator();
                RenderHardpointDetails();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    
    // Popup windows
    if (showHangar_) RenderHangarPanel();
    if (showHullSelector_) RenderHullSelector();
    if (showPresets_) RenderPresetSelector();
    if (showCustomization_) RenderCustomizationPanel();
    if (showInsurance_) RenderInsurancePanel();
}

void ShipEditorUI::RenderHangarPanel() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Hangar", &showHangar_)) {
        auto ships = builder_->GetHangarShips(playerId_);
        
        ImGui::Text("Ships in Hangar: %zu", ships.size());
        ImGui::Separator();
        
        if (ImGui::BeginTable("HangarTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Hull");
            ImGui::TableSetupColumn("Class");
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();
            
            for (const auto& ship : ships) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", ship->customName.empty() ? ship->name.c_str() : ship->customName.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", ship->hull->name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", ship->hull->className.c_str());
                
                ImGui::TableNextColumn();
                auto metrics = builder_->CalculatePerformance(*ship);
                ImGui::Text("$%.0f", metrics.totalCost);
                
                ImGui::TableNextColumn();
                if (ImGui::Button(("Edit##" + ship->id).c_str())) {
                    OpenEditor(ship);
                    showHangar_ = false;
                }
                ImGui::SameLine();
                if (ImGui::Button(("Remove##" + ship->id).c_str())) {
                    builder_->RemoveFromHangar(ship->id, playerId_);
                }
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void ShipEditorUI::RenderHullSelector() {
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Select Hull", &showHullSelector_)) {
        auto hulls = builder_->GetAvailableHulls();
        
        ImGui::Text("Available Hulls: %zu", hulls.size());
        ImGui::Separator();
        
        if (ImGui::BeginTable("HullTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Class");
            ImGui::TableSetupColumn("Cost");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();
            
            for (const auto& hull : hulls) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", hull->name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", hull->className.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("$%.0f", hull->cost);
                
                ImGui::TableNextColumn();
                if (ImGui::Button(("Select##" + hull->id).c_str())) {
                    CreateNewShip(hull->id);
                    showHullSelector_ = false;
                }
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void ShipEditorUI::RenderShipLayout() {
    if (!currentShip_ || !currentShip_->hull) return;
    
    ImGui::Text("Ship: %s", currentShip_->customName.empty() ? 
                currentShip_->name.c_str() : currentShip_->customName.c_str());
    ImGui::Text("Hull: %s (%s)", currentShip_->hull->name.c_str(), 
                currentShip_->hull->className.c_str());
    
    ImGui::Separator();
    
    // Ship visualization area
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(shipViewWidth_, shipViewHeight_);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                            IM_COL32(20, 20, 30, 255));
    
    // Ship silhouette (placeholder - centered rectangle)
    float ship_w = 200.0f;
    float ship_h = 120.0f;
    float ship_x = canvas_pos.x + (canvas_size.x - ship_w) * 0.5f;
    float ship_y = canvas_pos.y + (canvas_size.y - ship_h) * 0.5f;
    
    draw_list->AddRect(ImVec2(ship_x, ship_y), 
                      ImVec2(ship_x + ship_w, ship_y + ship_h),
                      IM_COL32(100, 100, 150, 255), 0.0f, 0, 2.0f);
    
    // Draw hardpoints
    DrawShipHardpoints();
    
    ImGui::Dummy(canvas_size);  // Reserve space
    
    // Hardpoint list
    ImGui::Separator();
    ImGui::Text("Hardpoints:");
    
    if (ImGui::BeginTable("HardpointTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Component");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();
        
        for (auto& hp : currentShip_->hull->hardpoints) {
            ImGui::TableNextRow();
            
            // Highlight selected hardpoint
            if (hp.id == selectedHardpoint_) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 
                                      IM_COL32(50, 80, 120, 100));
            }
            
            ImGui::TableNextColumn();
            if (ImGui::Selectable(hp.id.c_str(), hp.id == selectedHardpoint_,
                                 ImGuiSelectableFlags_SpanAllColumns)) {
                SelectHardpoint(hp.id);
            }
            
            ImGui::TableNextColumn();
            const char* typeStr = "Universal";
            switch (hp.type) {
                case HardpointType::Weapon: typeStr = "Weapon"; break;
                case HardpointType::Engine: typeStr = "Engine"; break;
                case HardpointType::Utility: typeStr = "Utility"; break;
                case HardpointType::Internal: typeStr = "Internal"; break;
                case HardpointType::External: typeStr = "External"; break;
                default: break;
            }
            ImGui::Text("%s", typeStr);
            
            ImGui::TableNextColumn();
            if (hp.occupied && hp.installedComponent) {
                ImGui::Text("%s", hp.installedComponent->name.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Empty");
            }
            
            ImGui::TableNextColumn();
            if (hp.occupied) {
                if (ImGui::Button(("Remove##" + hp.id).c_str())) {
                    builder_->RemoveComponent(*currentShip_, hp.id);
                    UpdatePerformanceMetrics();
                }
            } else {
                ImGui::TextDisabled("---");
            }
            
            // Drag-drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT")) {
                    AcceptComponentDrop(hp.id);
                }
                ImGui::EndDragDropTarget();
            }
        }
        
        ImGui::EndTable();
    }
}

void ShipEditorUI::RenderComponentCatalog() {
    ImGui::Text("Component Catalog");
    
    // Component type filter
    const char* typeNames[] = {
        "Engine", "Weapon", "Shield", "Sensor", "PowerPlant",
        "CargoHold", "LifeSupport", "FuelTank", "Thruster",
        "Armor", "Computer", "ECM", "Mining", "Repair"
    };
    int currentType = static_cast<int>(componentFilter_);
    if (ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
        componentFilter_ = static_cast<ComponentType>(currentType);
    }
    
    // Search filter
    char searchBuf[256];
    strncpy(searchBuf, searchQuery_.c_str(), sizeof(searchBuf));
    if (ImGui::InputText("Search", searchBuf, sizeof(searchBuf))) {
        searchQuery_ = searchBuf;
    }
    
    ImGui::Separator();
    
    // Component list
    auto components = builder_->GetComponentsByType(componentFilter_);
    
    ImGui::BeginChild("ComponentList", ImVec2(0, 350), true);
    
    for (const auto& comp : components) {
        // Apply search filter
        if (!searchQuery_.empty()) {
            std::string nameLower = comp->name;
            std::string queryLower = searchQuery_;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
            
            if (nameLower.find(queryLower) == std::string::npos) {
                continue;
            }
        }
        
        ImGui::PushID(comp->id.c_str());
        
        // Component item
        bool selected = false;
        if (ImGui::Selectable(comp->name.c_str(), &selected)) {
            // Component selected
        }
        
        // Drag source
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("COMPONENT", comp->id.c_str(), comp->id.size() + 1);
            ImGui::Text("Installing: %s", comp->name.c_str());
            BeginComponentDrag(comp);
            ImGui::EndDragDropSource();
        }
        
        // Tooltip with details
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", comp->name.c_str());
            ImGui::Separator();
            ImGui::Text("%s", comp->description.c_str());
            ImGui::Text("Cost: $%.0f", comp->cost);
            ImGui::Text("Mass: %.1f tons", comp->mass);
            ImGui::Text("Power: %.1f MW", comp->powerDraw);
            ImGui::EndTooltip();
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

void ShipEditorUI::RenderHardpointDetails() {
    ImGui::Text("Hardpoint Details");
    ImGui::Separator();
    
    if (selectedHardpoint_.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No hardpoint selected");
        return;
    }
    
    // Find selected hardpoint
    const Hardpoint* hp = nullptr;
    if (currentShip_ && currentShip_->hull) {
        for (const auto& hardpoint : currentShip_->hull->hardpoints) {
            if (hardpoint.id == selectedHardpoint_) {
                hp = &hardpoint;
                break;
            }
        }
    }
    
    if (!hp) return;
    
    ImGui::Text("Slot: %s", hp->id.c_str());
    
    if (hp->occupied && hp->installedComponent) {
        auto comp = hp->installedComponent;
        ImGui::Separator();
        ImGui::Text("Component: %s", comp->name.c_str());
        ImGui::TextWrapped("%s", comp->description.c_str());
        ImGui::Separator();
        ImGui::Text("Stats:");
        ImGui::BulletText("Mass: %.1f tons", comp->mass);
        ImGui::BulletText("Power: %.1f MW", comp->powerDraw);
        ImGui::BulletText("Cost: $%.0f", comp->cost);
        
        // Component-specific stats
        for (const auto& [key, value] : comp->stats) {
            ImGui::BulletText("%s: %.1f", key.c_str(), value);
        }
        
        // Upgrade options
        auto upgrades = builder_->GetUpgradeOptions(comp->id);
        if (!upgrades.empty()) {
            ImGui::Separator();
            ImGui::Text("Upgrade Options:");
            for (const auto& upgrade : upgrades) {
                if (ImGui::Button(upgrade->name.c_str())) {
                    builder_->RemoveComponent(*currentShip_, hp->id);
                    builder_->InstallComponent(*currentShip_, hp->id, upgrade->id);
                    UpdatePerformanceMetrics();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button("Remove Component", ImVec2(-1, 0))) {
            builder_->RemoveComponent(*currentShip_, hp->id);
            UpdatePerformanceMetrics();
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Slot is empty");
        
        // Show compatible components
        if (currentShip_) {
            auto compatible = builder_->GetCompatibleComponents(*currentShip_, hp->id);
            if (!compatible.empty()) {
                ImGui::Separator();
                ImGui::Text("Compatible Components:");
                ImGui::BeginChild("CompatibleList", ImVec2(0, 150), true);
                for (const auto& comp : compatible) {
                    if (ImGui::Selectable(comp->name.c_str())) {
                        builder_->InstallComponent(*currentShip_, hp->id, comp->id);
                        UpdatePerformanceMetrics();
                    }
                }
                ImGui::EndChild();
            }
        }
    }
}

void ShipEditorUI::RenderPerformancePanel() {
    if (!currentShip_) return;
    
    ImGui::Text("Performance Metrics");
    ImGui::Separator();
    
    const auto& m = cachedMetrics_;
    
    // Propulsion
    ImGui::Text("Propulsion:");
    ImGui::BulletText("Max Speed: %.0f m/s", m.maxSpeed);
    ImGui::BulletText("Acceleration: %.1f m/s²", m.acceleration);
    ImGui::BulletText("Maneuverability: %.1f deg/s", m.maneuverability);
    
    // Combat
    ImGui::Text("Combat:");
    ImGui::BulletText("Firepower: %.0f DPS", m.totalFirepower);
    ImGui::BulletText("Shield: %.0f HP", m.shieldStrength);
    ImGui::BulletText("Armor: %.0f", m.armorRating);
    
    // Power
    ImGui::Text("Power:");
    ImGui::SameLine(150);
    if (m.powerBalance >= 0) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "+%.1f MW", m.powerBalance);
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%.1f MW", m.powerBalance);
    }
    ImGui::ProgressBar(m.powerConsumption / m.powerGeneration, ImVec2(-1, 0));
    
    // Thermal
    ImGui::Text("Cooling:");
    ImGui::SameLine(150);
    if (m.thermalBalance >= 0) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "+%.1f", m.thermalBalance);
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%.1f", m.thermalBalance);
    }
    ImGui::ProgressBar(m.heatGeneration / m.coolingCapacity, ImVec2(-1, 0));
    
    // Mass & Cost
    ImGui::Text("Total Mass: %.0f tons", m.totalMass);
    ImGui::Text("Total Cost: $%.0f", m.totalCost);
}

void ShipEditorUI::RenderCustomizationPanel() {
    if (!ImGui::Begin("Ship Customization", &showCustomization_) || !currentShip_) {
        ImGui::End();
        return;
    }
    
    // Ship name
    char nameBuf[256];
    strncpy(nameBuf, currentShip_->customName.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Ship Name", nameBuf, sizeof(nameBuf))) {
        builder_->SetShipName(*currentShip_, nameBuf);
    }
    
    ImGui::Separator();
    
    // Paint job
    ImGui::Text("Paint Job:");
    float primary[3] = {currentShip_->paintJob.primaryR, 
                       currentShip_->paintJob.primaryG, 
                       currentShip_->paintJob.primaryB};
    float secondary[3] = {currentShip_->paintJob.secondaryR,
                         currentShip_->paintJob.secondaryG,
                         currentShip_->paintJob.secondaryB};
    
    if (ImGui::ColorEdit3("Primary Color", primary)) {
        builder_->SetPaintJob(*currentShip_, 
                            primary[0], primary[1], primary[2],
                            secondary[0], secondary[1], secondary[2]);
    }
    
    if (ImGui::ColorEdit3("Secondary Color", secondary)) {
        builder_->SetPaintJob(*currentShip_,
                            primary[0], primary[1], primary[2],
                            secondary[0], secondary[1], secondary[2]);
    }
    
    ImGui::Separator();
    
    // Decals
    ImGui::Text("Decals:");
    // TODO: Decal selection UI
    
    ImGui::End();
}

void ShipEditorUI::RenderPresetSelector() {
    if (!ImGui::Begin("Preset Loadouts", &showPresets_)) {
        ImGui::End();
        return;
    }
    
    auto presets = builder_->GetAvailablePresets();
    
    for (const auto& [type, name] : presets) {
        if (ImGui::Button(name.c_str(), ImVec2(-1, 0))) {
            ApplyPreset(type);
            showPresets_ = false;
        }
    }
    
    ImGui::End();
}

void ShipEditorUI::RenderInsurancePanel() {
    if (!ImGui::Begin("Ship Insurance", &showInsurance_) || !currentShip_) {
        ImGui::End();
        return;
    }
    
    double cost = builder_->CalculateInsuranceCost(*currentShip_);
    
    ImGui::Text("Ship Value: $%.0f", cachedMetrics_.totalCost);
    ImGui::Text("Insurance Cost: $%.0f", cost);
    ImGui::Text("Payout: $%.0f (90%%)", cachedMetrics_.totalCost * 0.9);
    
    ImGui::Separator();
    
    if (currentShip_->insured) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Ship is INSURED");
        if (ImGui::Button("Cancel Insurance")) {
            currentShip_->insured = false;
        }
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Ship is NOT INSURED");
        if (ImGui::Button("Purchase Insurance")) {
            builder_->PurchaseInsurance(*currentShip_);
        }
    }
    
    ImGui::End();
}

void ShipEditorUI::RenderValidationWarnings() {
    if (cachedMetrics_.errors.empty() && cachedMetrics_.warnings.empty()) {
        return;
    }
    
    ImGui::Text("Validation:");
    
    for (const auto& error : cachedMetrics_.errors) {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "ERROR: %s", error.c_str());
    }
    
    for (const auto& warning : cachedMetrics_.warnings) {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "WARNING: %s", warning.c_str());
    }
}

// === Helper Functions ===

void ShipEditorUI::SelectHardpoint(const std::string& hardpointId) {
    selectedHardpoint_ = hardpointId;
}

void ShipEditorUI::UpdatePerformanceMetrics() {
    if (currentShip_) {
        cachedMetrics_ = builder_->CalculatePerformance(*currentShip_);
    }
}

void ShipEditorUI::ApplyPreset(PresetType preset) {
    auto ship = builder_->LoadPreset(preset);
    if (ship) {
        OpenEditor(ship);
    }
}

void ShipEditorUI::SaveCurrentShip() {
    if (currentShip_) {
        // TODO: Implement save dialog
        std::cout << "[ShipEditorUI] Saving ship..." << std::endl;
    }
}

void ShipEditorUI::CreateNewShip(const std::string& hullId) {
    auto ship = builder_->CreateShip(hullId);
    if (ship) {
        builder_->AddToHangar(ship, playerId_);
        OpenEditor(ship);
    }
}

void ShipEditorUI::BeginComponentDrag(std::shared_ptr<ComponentDefinition> component) {
    draggedComponent_ = component;
    isDragging_ = true;
}

void ShipEditorUI::AcceptComponentDrop(const std::string& hardpointId) {
    if (draggedComponent_ && currentShip_) {
        bool success = builder_->InstallComponent(*currentShip_, hardpointId, draggedComponent_->id);
        if (success) {
            UpdatePerformanceMetrics();
        }
    }
    
    draggedComponent_ = nullptr;
    isDragging_ = false;
}

void ShipEditorUI::DrawShipHardpoints() {
    // TODO: Draw hardpoint positions on ship visualization
}

void ShipEditorUI::DrawHardpointIcon(const Hardpoint& hp, float x, float y, float size) {
    // TODO: Draw hardpoint icon based on type
}

void ShipEditorUI::DrawComponentIcon(const ComponentDefinition& comp, float size) {
    // TODO: Draw component icon based on type
}

} // namespace ShipBuilding
