#include "MainMenu.h"
#include <cctype>

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

MainMenu::MainMenu()
    : menu_("STAR ENGINE")
    , lastAction_(Action::None)
    , hasSaveGame_(false)
{
    // Customize the menu style before building to ensure derived data uses the new look
    auto style = menu_.GetStyle();
    style.titleColor = {255, 220, 100, 255};   // Golden title
    style.subtitleColor = {190, 210, 255, 255};
    style.footerColor = {160, 170, 180, 255};
    style.titleFontSize = 64.0f;
    style.subtitleFontSize = 30.0f;
    style.itemFontSize = 32.0f;
    style.itemSpacing = 52.0f;
    style.titleSpacing = 120.0f;
    style.subtitleSpacing = 36.0f;
    style.footerSpacing = 90.0f;
    style.backgroundPadding = 60.0f;
    style.selectedPulseSpeed = 2.5f;
    style.selectedPulseMinAlpha = 0.6f;
    style.selectedPulseMaxAlpha = 1.0f;
    style.selectedScaleAmplitude = 0.08f;
    menu_.SetStyle(style);

    menu_.SetSubtitle("Command Interface v0.3 — Prepare for Launch");
    menu_.SetFooter("Use W/S or Arrow Keys to navigate • Enter to confirm • Esc to exit");

    BuildMenu();
}

void MainMenu::BuildMenu() {
    menu_.ClearItems();

    // New Game
    MenuSystem::MenuItem newGame("New Campaign", [this]() { OnNewGame(); });
    newGame.description = "Launch a brand-new expedition from the bridge.";
    newGame.shortcutHint = "N";
    menu_.AddItem(newGame);

    // Continue (disabled if no save)
    MenuSystem::MenuItem continueItem("Continue", [this]() { OnContinue(); });
    continueItem.enabled = hasSaveGame_;
    continueItem.description = hasSaveGame_
        ? "Resume your latest mission from the command log."
        : "No flight data detected yet. Complete a mission to unlock.";
    continueItem.shortcutHint = "C";
    menu_.AddItem(continueItem);

    // Settings
    MenuSystem::MenuItem settingsItem("Settings", [this]() { OnSettings(); });
    settingsItem.description = "Adjust audio, video, and control preferences.";
    settingsItem.shortcutHint = "S";
    menu_.AddItem(settingsItem);

    // Credits
    MenuSystem::MenuItem creditsItem("Credits", [this]() { OnCredits(); });
    creditsItem.description = "Meet the crew responsible for Nova Engine.";
    creditsItem.shortcutHint = "R";
    menu_.AddItem(creditsItem);

    // Quit
    MenuSystem::MenuItem quitItem("Quit", [this]() { OnQuit(); });
    quitItem.description = "Shut down the simulation and return to reality. (Esc also works.)";
    quitItem.shortcutHint = "Q";
    menu_.AddItem(quitItem);
}

void MainMenu::SetHasSaveGame(bool hasSave) {
    if (hasSaveGame_ != hasSave) {
        hasSaveGame_ = hasSave;
        menu_.SetItemEnabled(ITEM_CONTINUE, hasSave);
        menu_.SetItemText(ITEM_CONTINUE, hasSave ? "Continue" : "Continue (Unavailable)");
        menu_.SetItemDescription(ITEM_CONTINUE,
            hasSave
                ? "Resume your latest mission from the command log."
                : "No flight data detected yet. Complete a mission to unlock.");
    }
}

void MainMenu::Update(double dt) {
    menu_.Update(dt);
}

MainMenu::RenderData MainMenu::GetRenderData() const {
    RenderData data;
    menu_.GetRenderData(data.title, data.items, data.selectedIndex, data.style,
                        &data.subtitle, &data.footer, &data.selectedItemAlpha,
                        &data.selectedItemScale);
    return data;
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
    else if (ActivateShortcutKey(key)) {
        return;
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
    else if (ActivateShortcutKey(key)) {
        return;
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

void MainMenu::OnCredits() {
    lastAction_ = Action::Credits;
}

void MainMenu::OnQuit() {
    lastAction_ = Action::Quit;
}

bool MainMenu::ActivateShortcutKey(int key) {
    char shortcutChar = '\0';

#ifdef USE_GLFW
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        shortcutChar = static_cast<char>('A' + (key - GLFW_KEY_A));
    }
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        shortcutChar = static_cast<char>('0' + (key - GLFW_KEY_0));
    }
#else
    if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
        (key >= '0' && key <= '9')) {
        shortcutChar = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    }
#endif

    if (shortcutChar == '\0') {
        return false;
    }

    std::string target(1, shortcutChar);

    for (int i = 0; i < menu_.GetItemCount(); ++i) {
        const auto* item = menu_.GetItem(i);
        if (!item || !item->enabled || !item->visible || item->isSeparator) {
            continue;
        }

        if (item->shortcutHint.empty()) {
            continue;
        }

        std::string normalizedHint;
        normalizedHint.reserve(item->shortcutHint.size());
        for (char c : item->shortcutHint) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                normalizedHint.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            }
        }

        if (normalizedHint == target) {
            menu_.SelectItem(i);
            menu_.ActivateSelected();
            return true;
        }
    }

    return false;
}
