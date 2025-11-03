#pragma once

#include "MenuSystem.h"
#include "ecs/EntityManager.h"
#include "editor/UndoRedoSystem.h"
#include "editor/ComponentInspector.h"
#include "editor/SelectionManager.h"
#include <string>
#include <vector>
#include <memory>

/**
 * GameEditor - In-game editor for Nova Engine
 * 
 * Features:
 * - Entity creation and deletion
 * - Position and component editing
 * - Real-time entity inspection
 * - JSON export/import
 * - Integration with existing systems
 * 
 * Controls:
 * - E key: Toggle editor mode
 * - Arrow keys: Navigate menu
 * - Enter: Select option
 * - Tab: Switch editor panels
 * - Esc: Close editor/cancel
 */
class GameEditor {
public:
    enum class EditorMode {
        Disabled,
        MainMenu,
        EntityList,
        EntityEditor,
        CreateEntity,
        WorldSettings
    };

    enum class EntityTool {
        Select,
        Move,
        Rotate,
        Scale,
        Delete
    };

    struct EditorState {
        EditorMode mode = EditorMode::Disabled;
        EntityTool activeTool = EntityTool::Select;
        Entity selectedEntity = 0;
        std::string statusMessage;
        std::string errorMessage;
        
        // Entity creation
        std::string newEntityType = "spaceship";
        double newEntityX = 0.0;
        double newEntityY = 0.0; 
        double newEntityZ = 0.0;
        
        // Text input
        bool textInputMode = false;
        std::string textInputBuffer;
        std::string textInputPrompt;
        
        // Inspector mode
        bool showComponentInspector = false;
        int inspectorScrollOffset = 0;
    };

    GameEditor();
    ~GameEditor() = default;

    // Core functions
    void Initialize();
    void Update(double deltaTime);
    void Render(class Viewport3D& viewport);

    // Input handling
    void HandleKeyPress(int key);
    void HandleTextInput(const std::string& text);
    void HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight);

    // State management
    bool IsActive() const { return state_.mode != EditorMode::Disabled; }
    void Toggle();
    void SetEntityManager(EntityManager* em) { entityManager_ = em; }

    // Editor operations
    void CreateEntity(const std::string& type, double x, double y, double z);
    void DeleteEntity(Entity entity);
    void SelectEntity(Entity entity, bool additive = false);
    void MoveEntity(Entity entity, double x, double y, double z);
    void DuplicateEntity(Entity entity);
    void DuplicateSelection();

    // Import/Export
    bool ExportWorldToJson(const std::string& filename);
    bool ImportWorldFromJson(const std::string& filename);

private:
    EditorState state_;
    EntityManager* entityManager_ = nullptr;
    std::unique_ptr<MenuSystem> currentMenu_;
    
    // Editor systems
    std::unique_ptr<UndoRedoSystem> undoRedoSystem_;
    std::unique_ptr<ComponentInspector> componentInspector_;
    std::unique_ptr<SelectionManager> selectionManager_;
    
    // UI Management
    void BuildMainMenu();
    void BuildEntityListMenu();
    void BuildEntityEditorMenu();
    void BuildCreateEntityMenu();
    void BuildWorldSettingsMenu();
    void BuildComponentInspectorMenu();
    
    void UpdateMenuForMode();
    void HandleMenuAction(const std::string& action);
    
    // Entity operations
    std::vector<Entity> GetAllEntities();
    std::string GetEntityDisplayName(Entity entity);
    void UpdateSelectedEntityPosition();
    
    // Text input system
    void StartTextInput(const std::string& prompt, const std::string& defaultValue = "");
    void FinishTextInput();
    void CancelTextInput();
    
    // Utility
    void SetStatusMessage(const std::string& message);
    void SetErrorMessage(const std::string& message);
    void ClearMessages();
};