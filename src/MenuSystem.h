#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * MenuSystem - Generic menu system for creating interactive menus
 * 
 * Features:
 * - Menu items with text, callbacks, and enabled state
 * - Keyboard navigation (arrow keys, Enter, Esc)
 * - Mouse support (hover, click)
 * - Customizable colors and styling
 * - Hierarchical menus (submenus)
 */
class MenuSystem {
public:
    struct MenuItem {
        std::string text;
        std::function<void()> callback;
        bool enabled = true;
        bool visible = true;
        int id = -1;  // Optional ID for special items
        std::string description;  // Optional extended text for tooltips/details
        std::string shortcutHint; // Display hint for keyboard/controller shortcuts
        bool isSeparator = false; // Non-selectable visual separator

        MenuItem(const std::string& t, std::function<void()> cb = nullptr)
            : text(t), callback(cb) {}
    };
    
    struct MenuStyle {
        // Colors (RGBA, 0-255)
        struct Color {
            unsigned char r, g, b, a;
            Color(unsigned char red = 255, unsigned char green = 255, 
                  unsigned char blue = 255, unsigned char alpha = 255)
                : r(red), g(green), b(blue), a(alpha) {}
        };
        
        Color titleColor{255, 200, 50, 255};        // Gold/yellow title
        Color normalColor{200, 200, 200, 255};      // Light gray normal items
        Color selectedColor{255, 255, 255, 255};    // White selected item
        Color disabledColor{100, 100, 100, 255};    // Dark gray disabled
        Color backgroundColor{0, 0, 0, 180};        // Semi-transparent black background
        Color subtitleColor{200, 220, 255, 255};    // Subtle blue subtitle text
        Color footerColor{150, 150, 150, 255};      // Muted footer text

        float titleFontSize = 48.0f;
        float itemFontSize = 24.0f;
        float itemSpacing = 40.0f;      // Vertical spacing between items
        float titleSpacing = 80.0f;     // Space between title and first item
        float subtitleFontSize = 28.0f;
        float subtitleSpacing = 30.0f;  // Space between title and subtitle
        float footerFontSize = 20.0f;
        float footerSpacing = 70.0f;    // Space below last item before footer text
        bool drawBackground = true;
        float backgroundPadding = 40.0f;
        float selectedPulseSpeed = 3.0f;
        float selectedPulseMinAlpha = 0.7f;
        float selectedPulseMaxAlpha = 1.0f;
        float selectedScaleAmplitude = 0.05f;
    };
    
    MenuSystem(const std::string& title = "Menu");
    ~MenuSystem() = default;
    
    // Menu item management
    void AddItem(const std::string& text, std::function<void()> callback = nullptr);
    void AddItem(const MenuItem& item);
    void ClearItems();
    void SetItemEnabled(int index, bool enabled);
    void SetItemVisible(int index, bool visible);
    
    // Navigation
    void SelectNext();
    void SelectPrevious();
    void SelectItem(int index);
    int GetSelectedIndex() const { return selectedIndex_; }
    void ActivateSelected();
    
    // Mouse interaction
    bool HandleMouseMove(int mouseX, int mouseY, int screenWidth, int screenHeight);
    bool HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight);
    
    // Update and rendering
    void Update(double dt);
    void GetRenderData(std::string& outTitle, std::vector<MenuItem>& outItems,
                       int& outSelectedIndex, MenuStyle& outStyle,
                       std::string* outSubtitle = nullptr, std::string* outFooter = nullptr,
                       float* outSelectedAlpha = nullptr, float* outSelectedScale = nullptr) const;

    // Configuration
    void SetTitle(const std::string& title) { title_ = title; }
    const std::string& GetTitle() const { return title_; }
    void SetStyle(const MenuStyle& style) { style_ = style; }
    const MenuStyle& GetStyle() const { return style_; }
    void SetSubtitle(const std::string& subtitle) { subtitle_ = subtitle; }
    const std::string& GetSubtitle() const { return subtitle_; }
    void SetFooter(const std::string& footer) { footer_ = footer; }
    const std::string& GetFooter() const { return footer_; }

    void SetItemText(int index, const std::string& text);
    void SetItemDescription(int index, const std::string& description);
    void SetItemShortcutHint(int index, const std::string& shortcutHint);
    const MenuItem* GetItem(int index) const;
    int GetItemCount() const { return static_cast<int>(items_.size()); }

    // State
    bool IsActive() const { return active_; }
    void SetActive(bool active) { active_ = active; }

private:
    std::string title_;
    std::string subtitle_;
    std::string footer_;
    std::vector<MenuItem> items_;
    int selectedIndex_ = -1;
    MenuStyle style_;
    bool active_ = true;

    // Animation state
    float pulseTimer_ = 0.0f;
    float selectedItemAlpha_ = 1.0f;
    float selectedItemScale_ = 1.0f;

    // Helper methods
    void FindNextSelectableItem();
    void FindPreviousSelectableItem();
    int GetItemAt(int mouseX, int mouseY, int screenWidth, int screenHeight) const;
    void EnsureValidSelection();
    bool IsSelectable(int index) const;
};
