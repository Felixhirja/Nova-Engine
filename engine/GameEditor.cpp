#include "GameEditor.h"
#include "EntityFactory.h"
#include "Viewport3D.h"
#include "TextRenderer.h"
#include "MainMenu.h"
#include "ecs/Components.h"
#include "editor/EditorCommands.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

// TODO: GameEditor Comprehensive Feature Roadmap
// 
// CORE EDITING FEATURES:
// [ ] Component Inspector: Real-time viewing/editing of entity components
// [ ] Transform Gizmos: Visual 3D handles for moving/rotating entities in world space
// [ ] Multi-selection: Select and manipulate multiple entities at once
// [ ] Copy/Paste/Duplicate: Clone entities with all their components
// [ ] Undo/Redo System: Full action history with ctrl+z/ctrl+y support
//
// ECS SYSTEM INTEGRATION:
// [ ] EntityManagerV2 Migration: Fully migrate editor to modern ECS archetype system
// [ ] Component Type Registry: Dynamic discovery and editing of all component types
// [ ] Archetype Inspector: View entity archetypes and component layouts for optimization
// [ ] Query Builder UI: Visual query construction for finding entities with specific components
// [ ] ECS Performance Monitor: Real-time archetype iteration speed and memory usage tracking
// [ ] Component Batch Operations: Add/remove components from multiple entities efficiently
// [ ] System Inspector: View and control ECS system execution order and timing
// [ ] Entity Relationships: Parent-child hierarchies and entity references
// [ ] Component Templates: Save/load component configurations as reusable templates
// [ ] Memory Layout Visualization: Show how components are stored in memory for optimization
// [ ] Parallel System Debugging: Visualize parallel ECS system execution and dependencies
// [ ] Entity Lifecycle Tracking: Monitor entity creation, destruction, and component changes
//
// WORLD EDITING:
// [ ] Terrain Editor: Height mapping and texture painting
// [ ] Environment Settings: Lighting, skybox, atmosphere configuration
// [ ] Physics Debugging: Collision shape visualization and testing
// [ ] Particle System Editor: Visual particle effect creation and tuning
//
// ASSET MANAGEMENT:
// [ ] Asset Browser: File explorer for models, textures, sounds
// [ ] Prefab System: Save/load entity templates with components
// [ ] Resource Monitor: Memory usage and performance profiling
// [ ] Hot Reload: Real-time asset updates without restart
//
// USER EXPERIENCE:
// [ ] Keyboard Shortcuts: Full hotkey system (delete, copy, etc.)
// [ ] Context Menus: Right-click actions for entities and world
// [ ] Toolbar: Quick access to common tools and modes
// [ ] Viewport Controls: Pan, zoom, orbit camera in edit mode
// [ ] Grid/Snap System: Align entities to grid positions
//
// PERSISTENCE:
// [ ] Scene Saving: Complete world state serialization to JSON/Binary
// [ ] Project Files: Manage multiple scenes and configurations
// [ ] Import/Export: Support for common 3D formats (OBJ, GLTF, etc.)
// [ ] Version Control: Git-friendly scene file format
//
// ADVANCED FEATURES:
// [ ] Scripting Integration: Lua/C++ script editing and debugging
// [ ] Animation Timeline: Keyframe animation editor for entities
// [ ] AI Behavior Editor: Visual behavior tree creation and testing
// [ ] Network Testing: Multiplayer simulation and debugging tools
//
// PERFORMANCE:
// [ ] Level-of-Detail: Automatic LOD generation and management
// [ ] Occlusion Culling: Visibility optimization for large scenes
// [ ] Batch Operations: Efficient multi-entity modifications
// [ ] Background Processing: Non-blocking operations for large scenes

GameEditor::GameEditor() {
    currentMenu_ = std::make_unique<MenuSystem>("GAME EDITOR");
    
    // Configure the menu style - disable background since we draw our own
    MenuSystem::MenuStyle style;
    style.drawBackground = false;             // Disable MenuSystem background
    style.backgroundPadding = 50.0f;          // Keep padding for text spacing
    style.titleColor = {255, 255, 255, 255};  // Bright white title
    style.selectedColor = {255, 255, 0, 255}; // Bright yellow selection
    style.normalColor = {200, 200, 200, 255}; // Light gray text
    currentMenu_->SetStyle(style);
    
    // TODO: Initialize additional editor systems
    // [ ] Create undo/redo command stack
    // [ ] Initialize transform gizmo renderer
    // [ ] Setup keyboard shortcut mappings
    // [ ] Create clipboard system for copy/paste
    // [ ] Initialize asset browser state
    // [ ] Setup ECS inspector integration with EntityManagerV2
    // [ ] Initialize component type registry for dynamic discovery
    // [ ] Create archetype visualization system
    // [ ] Setup query builder UI framework
    // [ ] Initialize entity relationship tracking
}

void GameEditor::Initialize() {
    BuildMainMenu();
    
    if (entityManager_) {
        componentInspector_ = std::make_unique<ComponentInspector>(entityManager_);
    }
    
    // TODO: Complete editor initialization
    // [ ] Load editor preferences from config file
    // [ ] Initialize 3D viewport camera for editor mode
    // [ ] Setup debug rendering systems (wireframes, bounds, etc.)
    // [ ] Create selection highlighting system
    // [ ] Initialize ECS debugging tools and inspectors
    // [ ] Setup component template library
    // [ ] Create entity hierarchy visualization
    // [ ] Initialize query performance monitoring
}

void GameEditor::Update(double deltaTime) {
    if (!IsActive()) return;
    
    if (currentMenu_) {
        currentMenu_->Update(deltaTime);
    }
    
    // Clear messages after a few seconds
    static double messageTimer = 0.0;
    messageTimer += deltaTime;
    if (messageTimer > 3.0) {
        ClearMessages();
        messageTimer = 0.0;
    }
    
    // TODO: Add core editor update systems
    // [ ] Update transform gizmos and handles
    // [ ] Process mouse picking for entity selection
    // [ ] Update camera controls in editor mode
    // [ ] Handle drag-and-drop operations
    // [ ] Update real-time component inspector
    // [ ] Process background asset loading
    // [ ] Update particle system previews
    // [ ] Monitor ECS performance metrics (archetype changes, system timing)
    // [ ] Update entity relationship dependencies
    // [ ] Process component batch operations
    // [ ] Update query builder results in real-time
    // [ ] Monitor memory usage and fragmentation
}

void GameEditor::Render(Viewport3D& viewport) {
    if (!IsActive()) return;
    
    // TODO: Add 3D editor visualization before UI rendering
    // [ ] Render selection outlines and highlights
    // [ ] Draw transform gizmos (move/rotate/scale handles)
    // [ ] Show collision bounds and physics debug shapes
    // [ ] Render grid and snap-to-grid indicators
    // [ ] Display entity labels and debug info in 3D space
    // [ ] Show camera frustums and light volumes
    // [ ] Render particle system bounding boxes
    
    // Add a background that EXACTLY matches MenuSystem's calculations
    if (auto* uiBatcher = viewport.GetUIBatcher()) {
        // Get the actual menu render data
        MainMenu::RenderData renderData;
        if (currentMenu_) {
            currentMenu_->GetRenderData(renderData.title, renderData.items, renderData.selectedIndex, renderData.style, 
                                       &renderData.subtitle, &renderData.footer, &renderData.selectedItemAlpha, &renderData.selectedItemScale);
        }
        
        // Use EXACT same calculations as MenuSystem in Viewport3D::RenderMenuOverlay
        const float centerX = static_cast<float>(viewport.GetWidth()) * 0.5f;
        const float baseY = static_cast<float>(viewport.GetHeight()) * 0.25f;
        
        // Calculate maxLineWidth exactly like MenuSystem does
        const FontSize titleFont = FontSize::Large;
        const FontSize subtitleFont = FontSize::Medium;
        const FontSize itemFont = FontSize::Medium;
        
        const int titleHeight = TextRenderer::GetFontHeight(titleFont);
        const int subtitleHeight = TextRenderer::GetFontHeight(subtitleFont);
        const int itemHeight = TextRenderer::GetFontHeight(itemFont);
        
        // Get visible items count
        std::vector<const MenuSystem::MenuItem*> visibleItems;
        for (const auto& item : renderData.items) {
            if (item.visible) {
                visibleItems.push_back(&item);
            }
        }
        
        // Calculate maxLineWidth exactly like MenuSystem
        int maxLineWidth = 0;
        if (!renderData.title.empty()) {
            maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(renderData.title, titleFont));
        }
        if (!renderData.subtitle.empty()) {
            maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(renderData.subtitle, subtitleFont));
        }
        for (const auto* item : visibleItems) {
            maxLineWidth = std::max(maxLineWidth, TextRenderer::MeasureText(item->text, itemFont));
        }
        maxLineWidth = std::max(maxLineWidth, 320); // MenuSystem minimum
        
        // Calculate contentHeight exactly like MenuSystem
        float cursorY = baseY;
        if (!renderData.title.empty()) {
            cursorY += static_cast<float>(titleHeight);
        }
        if (!renderData.subtitle.empty()) {
            cursorY += renderData.style.subtitleSpacing;
            cursorY += static_cast<float>(subtitleHeight);
        }
        cursorY += renderData.style.titleSpacing;
        for (size_t i = 0; i < visibleItems.size(); ++i) {
            cursorY += static_cast<float>(itemHeight);
            if (i + 1 < visibleItems.size()) {
                cursorY += renderData.style.itemSpacing;
            }
        }
        
        // Calculate background dimensions EXACTLY like MenuSystem
        const float backgroundWidth = static_cast<float>(maxLineWidth) + renderData.style.backgroundPadding * 2.0f;
        const float contentHeight = std::max(cursorY - baseY, static_cast<float>(itemHeight));
        const float backgroundHeight = contentHeight + renderData.style.backgroundPadding * 2.0f;
        const float backgroundLeft = centerX - backgroundWidth * 0.5f;
        const float backgroundTop = baseY - renderData.style.backgroundPadding;
        
        // Draw background exactly like MenuSystem does
        uiBatcher->AddQuad(backgroundLeft, backgroundTop, backgroundWidth, backgroundHeight,
                          0.05f, 0.05f, 0.25f, 0.92f);
        
        // Borders that perfectly frame the background
        const float borderSize = 2.0f;
        // Top border - extends beyond background edges
        uiBatcher->AddQuad(backgroundLeft - borderSize, backgroundTop - borderSize, 
                          backgroundWidth + borderSize * 2, borderSize,
                          1.0f, 0.6f, 0.1f, 0.9f);
        // Bottom border
        uiBatcher->AddQuad(backgroundLeft - borderSize, backgroundTop + backgroundHeight, 
                          backgroundWidth + borderSize * 2, borderSize,
                          1.0f, 0.6f, 0.1f, 0.9f);
        // Left border
        uiBatcher->AddQuad(backgroundLeft - borderSize, backgroundTop, 
                          borderSize, backgroundHeight,
                          1.0f, 0.6f, 0.1f, 0.9f);
        // Right border
        uiBatcher->AddQuad(backgroundLeft + backgroundWidth, backgroundTop, 
                          borderSize, backgroundHeight,
                          1.0f, 0.6f, 0.1f, 0.9f);
        
        // Flush to ensure background renders before menu
        uiBatcher->Flush();
    }
    
    // Render the editor menu on top of the properly sized background
    if (currentMenu_) {
        MainMenu::RenderData renderData;
        
        currentMenu_->GetRenderData(renderData.title, renderData.items, renderData.selectedIndex, renderData.style, 
                                   &renderData.subtitle, &renderData.footer, &renderData.selectedItemAlpha, &renderData.selectedItemScale);
        
        viewport.RenderMenuOverlay(renderData);
    }
    
    // Only render minimal status information to avoid UI conflicts
    if (state_.textInputMode) {
        const int inputY = viewport.GetHeight() / 2 + 150; // Below the menu
        const int inputX = viewport.GetWidth() / 4;
        
        // Render input prompt and text with fully opaque colors
        std::string prompt = ">>> " + state_.textInputPrompt + " <<<";
        TextRenderer::RenderText(prompt, inputX, inputY,
                                TextColor(1.0f, 1.0f, 0.0f), FontSize::Large);
        
        std::string input = "> " + state_.textInputBuffer + " <";
        TextRenderer::RenderText(input, inputX, inputY + 30,
                                TextColor(0.0f, 1.0f, 0.0f), FontSize::Large);
        
        TextRenderer::RenderText("ENTER to confirm | ESC to cancel", inputX, inputY + 60,
                                TextColor(1.0f, 0.5f, 0.0f), FontSize::Medium);
    }
    
    // TODO: Render additional UI panels and overlays
    // [ ] Component Inspector Panel: Real-time property editing
    // [ ] Asset Browser Panel: File explorer and preview
    // [ ] Scene Hierarchy Panel: Tree view of all entities
    // [ ] Tool Palette: Quick access to editing tools
    // [ ] Performance Monitor: FPS, memory, entity count
    // [ ] Console Log: Error messages and debug output
    // [ ] Mini-viewport: Alternative camera angles
}

void GameEditor::HandleKeyPress(int key) {
    if (!IsActive()) return;
    
    // Handle keyboard shortcuts
    #ifdef USE_GLFW
    // Check for Ctrl modifier
    bool ctrlPressed = (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                        glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    
    if (ctrlPressed) {
        if (key == GLFW_KEY_Z) {
            if (undoRedoSystem_->CanUndo()) {
                undoRedoSystem_->Undo();
                SetStatusMessage("Undo: " + undoRedoSystem_->GetRedoDescription());
            }
            return;
        } else if (key == GLFW_KEY_Y) {
            if (undoRedoSystem_->CanRedo()) {
                undoRedoSystem_->Redo();
                SetStatusMessage("Redo: " + undoRedoSystem_->GetRedoDescription());
            }
            return;
        } else if (key == GLFW_KEY_D) {
            DuplicateSelection();
            return;
        }
    }
    
    // Delete key
    if (key == GLFW_KEY_DELETE) {
        if (selectionManager_ && selectionManager_->HasSelection()) {
            auto multiCmd = std::make_unique<MultiEntityCommand>("Delete Selection");
            for (Entity e : selectionManager_->GetSelectedEntities()) {
                multiCmd->AddCommand(std::make_unique<DeleteEntityCommand>(entityManager_, e));
            }
            undoRedoSystem_->ExecuteCommand(std::move(multiCmd));
            selectionManager_->ClearSelection();
            SetStatusMessage("Deleted selected entities");
        }
        return;
    }
    
    // I key: Toggle Component Inspector
    if (key == GLFW_KEY_I) {
        state_.showComponentInspector = !state_.showComponentInspector;
        if (state_.showComponentInspector && selectionManager_->HasSelection()) {
            state_.mode = EditorMode::EntityEditor;
            BuildComponentInspectorMenu();
        }
        return;
    }
    #endif
    
    // TODO: Add more keyboard shortcuts
    // [ ] F key: Focus camera on selected entity
    // [ ] G/R/S: Grab/Rotate/Scale transform modes
    // [ ] Ctrl+S: Save current scene
    // [ ] Ctrl+O: Open scene file
    // [ ] Ctrl+N: New scene
    // [ ] Space: Toggle between game and editor camera
    
    // Handle text input mode
    if (state_.textInputMode) {
        if (key == GLFW_KEY_ENTER) {
            FinishTextInput();
        } else if (key == GLFW_KEY_ESCAPE) {
            CancelTextInput();
        } else if (key == GLFW_KEY_BACKSPACE && !state_.textInputBuffer.empty()) {
            state_.textInputBuffer.pop_back();
        }
        return;
    }
    
    // Handle menu navigation
    if (currentMenu_) {
        if (key == GLFW_KEY_UP) {
            currentMenu_->SelectPrevious();
        } else if (key == GLFW_KEY_DOWN) {
            currentMenu_->SelectNext();
        } else if (key == GLFW_KEY_ENTER) {
            currentMenu_->ActivateSelected();
        } else if (key == GLFW_KEY_ESCAPE) {
            if (state_.mode == EditorMode::MainMenu) {
                Toggle(); // Close editor
            } else {
                // Go back to main menu
                state_.mode = EditorMode::MainMenu;
                UpdateMenuForMode();
            }
        } else if (key == GLFW_KEY_TAB) {
            // Switch between different editor panels
            switch (state_.mode) {
                case EditorMode::MainMenu:
                    state_.mode = EditorMode::EntityList;
                    break;
                case EditorMode::EntityList:
                    state_.mode = EditorMode::CreateEntity;
                    break;
                case EditorMode::CreateEntity:
                    state_.mode = EditorMode::WorldSettings;
                    break;
                case EditorMode::WorldSettings:
                    state_.mode = EditorMode::MainMenu;
                    break;
                default:
                    state_.mode = EditorMode::MainMenu;
                    break;
            }
            UpdateMenuForMode();
        }
    }
}

void GameEditor::HandleTextInput(const std::string& text) {
    if (state_.textInputMode && !text.empty()) {
        // Filter out control characters
        for (char c : text) {
            if (c >= 32 && c < 127) { // Printable ASCII
                state_.textInputBuffer += c;
            }
        }
    }
}

void GameEditor::HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    if (!IsActive() || !currentMenu_) return;
    
    // TODO: Add advanced mouse interaction
    // [ ] 3D viewport mouse picking for entity selection
    // [ ] Transform gizmo interaction (click and drag handles)
    // [ ] Right-click context menus
    // [ ] Multi-selection with shift+click and drag boxes
    // [ ] Camera orbit/pan controls when not over UI
    // [ ] Asset drag-and-drop from browser to scene
    
    currentMenu_->HandleMouseClick(mouseX, mouseY, screenWidth, screenHeight);
}

void GameEditor::Toggle() {
    if (state_.mode == EditorMode::Disabled) {
        state_.mode = EditorMode::MainMenu;
        UpdateMenuForMode();
        SetStatusMessage("Game Editor Activated");
    } else {
        state_.mode = EditorMode::Disabled;
        state_.selectedEntity = 0;
        state_.textInputMode = false;
        ClearMessages();
    }
}

void GameEditor::CreateEntity(const std::string& type, double x, double y, double z) {
    if (!entityManager_) {
        SetErrorMessage("No entity manager available");
        return;
    }
    
    auto createCmd = std::make_unique<CreateEntityCommand>(
        entityManager_,
        [this, type, x, y, z]() {
            EntityFactory factory(*entityManager_);
            auto result = factory.CreateSpaceship(type, x, y, z);
            return result.success ? result.entity : 0;
        }
    );
    
    undoRedoSystem_->ExecuteCommand(std::move(createCmd));
    SetStatusMessage("Created " + type + " entity at (" + 
                    std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
    
    // TODO: Expand entity creation system
    // [ ] Support for all entity types (not just spaceships)
    // [ ] Component template system for rapid entity setup
    // [ ] Prefab instantiation from saved templates
    // [ ] Random placement with configurable constraints
    // [ ] Batch creation for multiple entities at once
    // [ ] Validation of entity placement (collision checking)
}

void GameEditor::DeleteEntity(Entity entity) {
    if (!entityManager_ || entity == 0) {
        SetErrorMessage("Invalid entity for deletion");
        return;
    }
    
    if (entityManager_->IsAlive(entity)) {
        auto deleteCmd = std::make_unique<DeleteEntityCommand>(entityManager_, entity);
        undoRedoSystem_->ExecuteCommand(std::move(deleteCmd));
        SetStatusMessage("Deleted entity " + std::to_string(entity));
        
        if (state_.selectedEntity == entity) {
            state_.selectedEntity = 0;
        }
        if (selectionManager_->IsSelected(entity)) {
            selectionManager_->DeselectEntity(entity);
        }
    } else {
        SetErrorMessage("Entity " + std::to_string(entity) + " is not alive");
    }
}

void GameEditor::SelectEntity(Entity entity, bool additive) {
    if (!entityManager_ || entity == 0) return;
    
    if (entityManager_->IsAlive(entity)) {
        state_.selectedEntity = entity;
        selectionManager_->SelectEntity(entity, additive);
        
        std::string msg = "Selected entity " + std::to_string(entity);
        if (selectionManager_->GetSelectionCount() > 1) {
            msg += " (" + std::to_string(selectionManager_->GetSelectionCount()) + " total)";
        }
        SetStatusMessage(msg);
        
        // Switch to entity editor mode
        state_.mode = EditorMode::EntityEditor;
        UpdateMenuForMode();
    } else {
        SetErrorMessage("Cannot select dead entity " + std::to_string(entity));
    }
}

void GameEditor::MoveEntity(Entity entity, double x, double y, double z) {
    if (!entityManager_ || entity == 0) return;
    
    auto moveCmd = std::make_unique<MoveEntityCommand>(entityManager_, entity, x, y, z);
    undoRedoSystem_->ExecuteCommand(std::move(moveCmd));
    SetStatusMessage("Moved entity " + std::to_string(entity) + 
                    " to (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
}

void GameEditor::DuplicateEntity(Entity entity) {
    if (!entityManager_ || entity == 0) return;
    
    auto dupCmd = std::make_unique<DuplicateEntityCommand>(entityManager_, entity);
    undoRedoSystem_->ExecuteCommand(std::move(dupCmd));
    SetStatusMessage("Duplicated entity " + std::to_string(entity));
}

void GameEditor::DuplicateSelection() {
    if (!selectionManager_ || !selectionManager_->HasSelection()) return;
    
    auto multiCmd = std::make_unique<MultiEntityCommand>("Duplicate Selection");
    for (Entity e : selectionManager_->GetSelectedEntities()) {
        multiCmd->AddCommand(std::make_unique<DuplicateEntityCommand>(entityManager_, e, 5.0, 0.0, 5.0));
    }
    undoRedoSystem_->ExecuteCommand(std::move(multiCmd));
    SetStatusMessage("Duplicated " + std::to_string(selectionManager_->GetSelectionCount()) + " entities");
}

void GameEditor::BuildMainMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("*** NOVA ENGINE EDITOR ***");
    
    std::string subtitle = ">>> Real-time Level and Entity Editor <<<";
    if (undoRedoSystem_) {
        if (undoRedoSystem_->CanUndo()) {
            subtitle += " | Undo: Ctrl+Z";
        }
        if (undoRedoSystem_->CanRedo()) {
            subtitle += " | Redo: Ctrl+Y";
        }
    }
    currentMenu_->SetSubtitle(subtitle);
    
    currentMenu_->AddItem(">> ENTITY LIST <<", [this]() {
        state_.mode = EditorMode::EntityList;
        UpdateMenuForMode();
    });
    
    currentMenu_->AddItem(">> CREATE ENTITY <<", [this]() {
        state_.mode = EditorMode::CreateEntity;
        UpdateMenuForMode();
    });
    
    currentMenu_->AddItem(">> WORLD SETTINGS <<", [this]() {
        state_.mode = EditorMode::WorldSettings;
        UpdateMenuForMode();
    });
    
    std::string undoText = ">> UNDO <<";
    if (undoRedoSystem_ && undoRedoSystem_->CanUndo()) {
        undoText += " (Ctrl+Z)";
    } else {
        undoText += " [Disabled]";
    }
    currentMenu_->AddItem(undoText, [this]() {
        if (undoRedoSystem_ && undoRedoSystem_->CanUndo()) {
            undoRedoSystem_->Undo();
            SetStatusMessage("Undo: " + undoRedoSystem_->GetRedoDescription());
            UpdateMenuForMode();
        }
    });
    
    std::string redoText = ">> REDO <<";
    if (undoRedoSystem_ && undoRedoSystem_->CanRedo()) {
        redoText += " (Ctrl+Y)";
    } else {
        redoText += " [Disabled]";
    }
    currentMenu_->AddItem(redoText, [this]() {
        if (undoRedoSystem_ && undoRedoSystem_->CanRedo()) {
            undoRedoSystem_->Redo();
            SetStatusMessage("Redo: " + undoRedoSystem_->GetRedoDescription());
            UpdateMenuForMode();
        }
    });
    
    currentMenu_->AddItem(">> EXPORT WORLD <<", [this]() {
        if (ExportWorldToJson("world_export.json")) {
            SetStatusMessage("World exported to world_export.json");
        } else {
            SetErrorMessage("Failed to export world");
        }
    });
    
    currentMenu_->AddItem(">> IMPORT WORLD <<", [this]() {
        if (ImportWorldFromJson("world_export.json")) {
            SetStatusMessage("World imported from world_export.json");
        } else {
            SetErrorMessage("Failed to import world");
        }
    });
    
    currentMenu_->AddItem(">> CLOSE EDITOR <<", [this]() {
        Toggle();
    });
    
    // TODO: Expand main menu with additional editor modes
    // [ ] Scene Management: New/Open/Save scene files
    // [ ] Asset Browser: Model/texture/sound management
    // [ ] Physics Debugger: Collision shape visualization
    // [ ] Performance Profiler: FPS/memory/entity monitoring
    // [ ] Script Editor: Lua/C++ code editing and debugging
    // [ ] Animation Timeline: Keyframe animation creation
    // [ ] Particle Editor: Visual effect creation and tuning
}

void GameEditor::BuildEntityListMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("*** ENTITY BROWSER ***");
    currentMenu_->SetSubtitle(">>> Select entity to inspect and modify <<<");
    
    auto entities = GetAllEntities();
    
    if (entities.empty()) {
        currentMenu_->AddItem("!!! NO ENTITIES FOUND !!!", nullptr);
    } else {
        for (Entity entity : entities) {
            std::string displayName = GetEntityDisplayName(entity);
            currentMenu_->AddItem(displayName, [this, entity]() {
                SelectEntity(entity);
            });
        }
    }
    
    currentMenu_->AddItem("<< BACK TO MAIN MENU >>", [this]() {
        state_.mode = EditorMode::MainMenu;
        UpdateMenuForMode();
    });
}

void GameEditor::BuildEntityEditorMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("🔧 ENTITY EDITOR");
    
    if (state_.selectedEntity == 0) {
        currentMenu_->SetSubtitle("⚠️ No entity selected");
        currentMenu_->AddItem("⬅️ Back to Entity List", [this]() {
            state_.mode = EditorMode::EntityList;
            UpdateMenuForMode();
        });
        return;
    }
    
    std::string subtitle = "Editing Entity " + std::to_string(state_.selectedEntity);
    if (componentInspector_) {
        subtitle += " | " + componentInspector_->GetComponentSummary(state_.selectedEntity);
    }
    currentMenu_->SetSubtitle(subtitle);
    
    currentMenu_->AddItem("🔍 Component Inspector (I)", [this]() {
        state_.showComponentInspector = true;
        BuildComponentInspectorMenu();
    });
    
    currentMenu_->AddItem("📍 Move Entity", [this]() {
        StartTextInput("Enter new position (x,y,z)", "0,0,0");
    });
    
    currentMenu_->AddItem("📋 Duplicate (Ctrl+D)", [this]() {
        DuplicateEntity(state_.selectedEntity);
    });
    
    currentMenu_->AddItem("🗑️ Delete Entity (Del)", [this]() {
        DeleteEntity(state_.selectedEntity);
        state_.mode = EditorMode::EntityList;
        UpdateMenuForMode();
    });
    
    currentMenu_->AddItem("⬅️ Back to Entity List", [this]() {
        state_.mode = EditorMode::EntityList;
        UpdateMenuForMode();
    });
    
    // TODO: Expand entity editor with comprehensive component management
    // [ ] Add Component: Add new components from available types
    // [ ] Remove Component: Safe component removal with validation
    // [ ] Transform Controls: 3D gizmos for position/rotation/scale
    // [ ] Animation Preview: Play entity animations in preview mode
    // [ ] Physics Testing: Test collision and physics behavior
    // [ ] AI Behavior: Edit and test AI state machines
    // [ ] Material Editor: Change textures and rendering properties
}

void GameEditor::BuildCreateEntityMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("➕ CREATE ENTITY");
    currentMenu_->SetSubtitle("Choose entity type and spawn location");
    
    // TODO: Expand entity creation with comprehensive type system
    // [ ] Environment Objects: Asteroids, space stations, planets
    // [ ] Weapons and Projectiles: Missiles, lasers, bullets
    // [ ] Interactive Objects: Pickups, triggers, spawn points
    // [ ] UI Elements: 3D UI panels, holographic displays
    // [ ] Particle Systems: Explosions, engine trails, ambient effects
    // [ ] Lighting: Point lights, spotlights, area lights
    // [ ] Audio Sources: 3D positioned sound emitters
    // [ ] Custom Prefabs: User-defined entity templates
    
    // Entity types with icons
    currentMenu_->AddItem("🚀 Spaceship", [this]() {
        state_.newEntityType = "spaceship";
        StartTextInput("Enter position (x,y,z)", "0,0,0");
    });
    
    currentMenu_->AddItem("⚔️ Fighter", [this]() {
        state_.newEntityType = "fighter";
        StartTextInput("Enter position (x,y,z)", "0,0,0");
    });
    
    currentMenu_->AddItem("🔍 Scout", [this]() {
        state_.newEntityType = "scout";
        StartTextInput("Enter position (x,y,z)", "0,0,0");
    });
    
    currentMenu_->AddItem("🛡️ Patrol", [this]() {
        state_.newEntityType = "patrol";
        StartTextInput("Enter position (x,y,z)", "0,0,0");
    });
    
    currentMenu_->AddItem("⬅️ Back to Main Menu", [this]() {
        state_.mode = EditorMode::MainMenu;
        UpdateMenuForMode();
    });
}

void GameEditor::BuildWorldSettingsMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("🌍 WORLD SETTINGS");
    currentMenu_->SetSubtitle("Global world configuration and management");
    
    // TODO: Expand world settings with comprehensive environment control
    // [ ] Lighting Settings: Ambient, directional, and dynamic lighting
    // [ ] Physics Configuration: Gravity, collision layers, simulation speed
    // [ ] Atmosphere Settings: Fog, skybox, environmental effects
    // [ ] Audio Configuration: Global volume, 3D audio settings, reverb
    // [ ] Performance Settings: LOD distances, culling, shadow quality
    // [ ] Weather System: Dynamic weather, particle effects, wind
    // [ ] Time of Day: Day/night cycle, sun position, lighting transitions
    // [ ] Terrain Settings: Height maps, texture blending, vegetation
    
    currentMenu_->AddItem("🧹 Clear All Entities", [this]() {
        if (!entityManager_) return;
        
        auto entities = GetAllEntities();
        int deletedCount = 0;
        for (Entity entity : entities) {
            if (entity != 1) { // Don't delete player entity
                entityManager_->DestroyEntity(entity);
                deletedCount++;
            }
        }
        SetStatusMessage("Cleared " + std::to_string(deletedCount) + " entities (preserved player)");
    });
    
    currentMenu_->AddItem("🎯 Spawn Test Squadron", [this]() {
        CreateEntity("fighter", -50, 0, 0);
        CreateEntity("scout", 50, 0, 0);
        CreateEntity("patrol", 0, 0, 50);
        SetStatusMessage("Spawned test squadron (Fighter, Scout, Patrol)");
    });
    
    currentMenu_->AddItem("⬅️ Back to Main Menu", [this]() {
        state_.mode = EditorMode::MainMenu;
        UpdateMenuForMode();
    });
}

void GameEditor::BuildComponentInspectorMenu() {
    currentMenu_->ClearItems();
    currentMenu_->SetTitle("🔍 COMPONENT INSPECTOR");
    
    if (!componentInspector_ || state_.selectedEntity == 0) {
        currentMenu_->SetSubtitle("⚠️ No entity selected");
        currentMenu_->AddItem("⬅️ Back", [this]() {
            state_.showComponentInspector = false;
            state_.mode = EditorMode::EntityEditor;
            UpdateMenuForMode();
        });
        return;
    }
    
    auto components = componentInspector_->InspectEntity(state_.selectedEntity);
    
    if (components.empty()) {
        currentMenu_->SetSubtitle("Entity " + std::to_string(state_.selectedEntity) + " has no components");
    } else {
        currentMenu_->SetSubtitle("Entity " + std::to_string(state_.selectedEntity) + " | " + 
                                  std::to_string(components.size()) + " components");
    }
    
    for (const auto& comp : components) {
        std::string componentDisplay = "▶ " + comp.typeName;
        currentMenu_->AddItem(componentDisplay, nullptr);
        
        for (const auto& prop : comp.properties) {
            std::string propDisplay = "  • " + prop.name + ": " + prop.value;
            currentMenu_->AddItem(propDisplay, nullptr);
        }
    }
    
    currentMenu_->AddItem("", nullptr);
    currentMenu_->AddItem("⬅️ Back to Entity Editor", [this]() {
        state_.showComponentInspector = false;
        state_.mode = EditorMode::EntityEditor;
        UpdateMenuForMode();
    });
}

void GameEditor::UpdateMenuForMode() {
    if (state_.showComponentInspector) {
        BuildComponentInspectorMenu();
        return;
    }
    
    switch (state_.mode) {
        case EditorMode::MainMenu:
            BuildMainMenu();
            break;
        case EditorMode::EntityList:
            BuildEntityListMenu();
            break;
        case EditorMode::EntityEditor:
            BuildEntityEditorMenu();
            break;
        case EditorMode::CreateEntity:
            BuildCreateEntityMenu();
            break;
        case EditorMode::WorldSettings:
            BuildWorldSettingsMenu();
            break;
        default:
            break;
    }
}

std::vector<Entity> GameEditor::GetAllEntities() {
    std::vector<Entity> entities;
    
    if (!entityManager_) return entities;
    
    // Get all entities with Position component (most entities should have this)
    entityManager_->ForEach<Position>([&entities](Entity e, Position&) {
        entities.push_back(e);
    });
    
    // Sort by entity ID
    std::sort(entities.begin(), entities.end());
    
    return entities;
}

std::string GameEditor::GetEntityDisplayName(Entity entity) {
    if (!entityManager_) return "Entity " + std::to_string(entity);
    
    std::string name = "";
    
    // Add type marker and name with dramatic styling
    if (entity == 1) {
        name += "[PLAYER] ";
    } else {
        name += "[ENTITY-" + std::to_string(entity) + "] ";
    }
    
    // Add position info if available
    if (auto* pos = entityManager_->GetComponent<Position>(entity)) {
        name += "POS(" + std::to_string((int)pos->x) + "," + 
                std::to_string((int)pos->y) + "," + std::to_string((int)pos->z) + ")";
    }
    
    // Add component count for debugging
    int componentCount = 0;
    if (entityManager_->GetComponent<Position>(entity)) componentCount++;
    if (entityManager_->GetComponent<Velocity>(entity)) componentCount++;
    if (entityManager_->GetComponent<DrawComponent>(entity)) componentCount++;
    if (entityManager_->GetComponent<PlayerController>(entity)) componentCount++;
    
    if (componentCount > 0) {
        name += " COMP:" + std::to_string(componentCount);
    }
    
    return name;
}

void GameEditor::StartTextInput(const std::string& prompt, const std::string& defaultValue) {
    state_.textInputMode = true;
    state_.textInputPrompt = prompt;
    state_.textInputBuffer = defaultValue;
}

void GameEditor::FinishTextInput() {
    if (!state_.textInputMode) return;
    
    std::string input = state_.textInputBuffer;
    state_.textInputMode = false;
    state_.textInputBuffer.clear();
    
    // Handle different input contexts
    if (state_.mode == EditorMode::CreateEntity) {
        // Parse position for entity creation
        double x = 0, y = 0, z = 0;
        if (sscanf(input.c_str(), "%lf,%lf,%lf", &x, &y, &z) == 3) {
            CreateEntity(state_.newEntityType, x, y, z);
        } else {
            SetErrorMessage("Invalid position format. Use: x,y,z");
        }
    } else if (state_.mode == EditorMode::EntityEditor) {
        // Parse position for entity movement
        double x = 0, y = 0, z = 0;
        if (sscanf(input.c_str(), "%lf,%lf,%lf", &x, &y, &z) == 3) {
            MoveEntity(state_.selectedEntity, x, y, z);
        } else {
            SetErrorMessage("Invalid position format. Use: x,y,z");
        }
    }
}

void GameEditor::CancelTextInput() {
    state_.textInputMode = false;
    state_.textInputBuffer.clear();
    state_.textInputPrompt.clear();
}

bool GameEditor::ExportWorldToJson(const std::string& filename) {
    // TODO: Implement comprehensive world serialization
    // [ ] Serialize all entities with components to JSON
    // [ ] Include world settings (lighting, physics, etc.)
    // [ ] Save camera positions and editor preferences
    // [ ] Export asset references and dependencies
    // [ ] Support for incremental/partial exports
    // [ ] Validation and error checking during export
    // [ ] Progress reporting for large worlds
    // [ ] Compression options for file size optimization
    
    SetStatusMessage("Export feature coming soon");
    return false;
}

bool GameEditor::ImportWorldFromJson(const std::string& filename) {
    // TODO: Implement comprehensive world deserialization
    // [ ] Load and validate JSON world format
    // [ ] Recreate entities with all components
    // [ ] Restore world settings and configuration
    // [ ] Handle missing assets gracefully
    // [ ] Support for version migration between formats
    // [ ] Progress reporting and cancellation support
    // [ ] Merge import (add to existing scene)
    // [ ] Replace import (clear scene first)
    
    SetStatusMessage("Import feature coming soon");
    return false;
}

void GameEditor::SetStatusMessage(const std::string& message) {
    state_.statusMessage = message;
    state_.errorMessage.clear();
}

void GameEditor::SetErrorMessage(const std::string& message) {
    state_.errorMessage = message;
    state_.statusMessage.clear();
}

void GameEditor::ClearMessages() {
    state_.statusMessage.clear();
    state_.errorMessage.clear();
}