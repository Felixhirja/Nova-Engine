#include "MainMenu.h"

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

MainMenu::MainMenu()
    : menu_("STAR ENGINE")
    , lastAction_(Action::None)
    , hasSaveGame_(false)
{
    BuildMenu();
    
    // Customize the menu style
    auto style = menu_.GetStyle();
    style.titleColor = {255, 220, 100, 255};  // Golden title
    style.titleFontSize = 64.0f;
    style.itemFontSize = 32.0f;
    style.itemSpacing = 50.0f;
    style.titleSpacing = 120.0f;
    menu_.SetStyle(style);
}

void MainMenu::BuildMenu() {
    menu_.ClearItems();
    
    // New Game
    menu_.AddItem("New Game", [this]() { OnNewGame(); });
    
    // Continue (disabled if no save)
    MenuSystem::MenuItem continueItem("Continue", [this]() { OnContinue(); });
    continueItem.enabled = hasSaveGame_;
    menu_.AddItem(continueItem);
    
    // Settings
    menu_.AddItem("Settings", [this]() { OnSettings(); });
    
    // Quit
    menu_.AddItem("Quit", [this]() { OnQuit(); });
}

void MainMenu::SetHasSaveGame(bool hasSave) {
    if (hasSaveGame_ != hasSave) {
        hasSaveGame_ = hasSave;
        menu_.SetItemEnabled(ITEM_CONTINUE, hasSave);
    }
}

void MainMenu::Update(double dt) {
    menu_.Update(dt);
}

void MainMenu::HandleKeyPress(int key) {
#ifdef USE_GLFW
    if (key == GLFW_KEY_UP) {
        menu_.SelectPrevious();
    }
    else if (key == GLFW_KEY_DOWN) {
        menu_.SelectNext();
    }
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        menu_.ActivateSelected();
    }
    else if (key == GLFW_KEY_ESCAPE) {
        lastAction_ = Action::Quit;
    }
#else
    // SDL or generic handling
    if (key == 'w' || key == 'W') {
        menu_.SelectPrevious();
    }
    else if (key == 's' || key == 'S') {
        menu_.SelectNext();
    }
    else if (key == '\n' || key == '\r') {
        menu_.ActivateSelected();
    }
    else if (key == 27) {  // ESC
        lastAction_ = Action::Quit;
    }
#endif
}

void MainMenu::HandleMouseMove(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    menu_.HandleMouseMove(mouseX, mouseY, screenWidth, screenHeight);
}

void MainMenu::HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    menu_.HandleMouseClick(mouseX, mouseY, screenWidth, screenHeight);
}

void MainMenu::OnNewGame() {
    lastAction_ = Action::NewGame;
}

void MainMenu::OnContinue() {
    lastAction_ = Action::Continue;
}

void MainMenu::OnSettings() {
    lastAction_ = Action::Settings;
}

void MainMenu::OnQuit() {
    lastAction_ = Action::Quit;
}
