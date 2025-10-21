#pragma once

#include "MenuSystem.h"
#include <functional>

/**
 * MainMenu - The main menu screen for Star Engine
 * 
 * Provides:
 * - New Game
 * - Continue (if save exists)
 * - Settings
 * - Quit
 */
class MainMenu {
public:
    enum class Action {
        None,
        NewGame,
        Continue,
        Settings,
        Credits,
        Quit
    };

    MainMenu();
    ~MainMenu() = default;
    
    // Update and input
    void Update(double dt);
    void HandleKeyPress(int key);  // For Enter, Esc, etc.
    void HandleMouseMove(int mouseX, int mouseY, int screenWidth, int screenHeight);
    void HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight);
    
    // Rendering
    struct RenderData {
        std::string title;
        std::string subtitle;
        std::string footer;
        std::vector<MenuSystem::MenuItem> items;
        int selectedIndex = -1;
        MenuSystem::MenuStyle style;
        float selectedItemAlpha = 1.0f;
        float selectedItemScale = 1.0f;
    };

    RenderData GetRenderData() const;
    
    // State
    Action GetLastAction() const { return lastAction_; }
    void ClearLastAction() { lastAction_ = Action::None; }
    bool IsActive() const { return menu_.IsActive(); }
    void SetActive(bool active) { menu_.SetActive(active); }
    
    // Configuration
    void SetHasSaveGame(bool hasSave);
    bool GetHasSaveGame() const { return hasSaveGame_; }
    
private:
    MenuSystem menu_;
    Action lastAction_;
    bool hasSaveGame_;
    
    // Menu item indices
    static constexpr int ITEM_NEW_GAME = 0;
    static constexpr int ITEM_CONTINUE = 1;
    static constexpr int ITEM_SETTINGS = 2;
    static constexpr int ITEM_CREDITS = 3;
    static constexpr int ITEM_QUIT = 4;

    void BuildMenu();
    void OnNewGame();
    void OnContinue();
    void OnSettings();
    void OnCredits();
    void OnQuit();

    bool ActivateShortcutKey(int key);
};
