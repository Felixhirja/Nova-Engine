#include "MenuSystem.h"
#include <cmath>
#include <algorithm>

MenuSystem::MenuSystem(const std::string& title)
    : title_(title)
    , selectedIndex_(-1)
    , active_(true)
    , pulseTimer_(0.0f)
    , selectedItemAlpha_(1.0f)
    , selectedItemScale_(1.0f)
{
}

void MenuSystem::AddItem(const std::string& text, std::function<void()> callback) {
    items_.emplace_back(text, callback);
    
    EnsureValidSelection();
}

void MenuSystem::AddItem(const MenuItem& item) {
    items_.push_back(item);

    EnsureValidSelection();
}

void MenuSystem::ClearItems() {
    items_.clear();
    selectedIndex_ = -1;
}

void MenuSystem::SetItemEnabled(int index, bool enabled) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].enabled = enabled;

        // If we disabled the currently selected item, find a new one
        if (!enabled && index == selectedIndex_) {
            FindNextSelectableItem();
        }

        EnsureValidSelection();
    }
}

void MenuSystem::SetItemVisible(int index, bool visible) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].visible = visible;

        // If we hid the currently selected item, find a new one
        if (!visible && index == selectedIndex_) {
            FindNextSelectableItem();
        }

        EnsureValidSelection();
    }
}

void MenuSystem::SelectNext() {
    if (items_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    int startIndex = selectedIndex_ < 0 ? 0 : selectedIndex_;
    int currentIndex = startIndex;

    for (size_t i = 0; i < items_.size(); ++i) {
        currentIndex = (currentIndex + 1) % static_cast<int>(items_.size());
        if (IsSelectable(currentIndex)) {
            selectedIndex_ = currentIndex;
            return;
        }
    }

    if (!IsSelectable(selectedIndex_)) {
        selectedIndex_ = -1;
    }
}

void MenuSystem::SelectPrevious() {
    if (items_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    int startIndex = selectedIndex_ < 0 ? 0 : selectedIndex_;
    int currentIndex = startIndex;

    for (size_t i = 0; i < items_.size(); ++i) {
        currentIndex--;
        if (currentIndex < 0) {
            currentIndex = static_cast<int>(items_.size()) - 1;
        }

        if (IsSelectable(currentIndex)) {
            selectedIndex_ = currentIndex;
            return;
        }
    }

    if (!IsSelectable(selectedIndex_)) {
        selectedIndex_ = -1;
    }
}

void MenuSystem::SelectItem(int index) {
    if (IsSelectable(index)) {
        selectedIndex_ = index;
    }
}

void MenuSystem::ActivateSelected() {
    if (IsSelectable(selectedIndex_)) {
        const auto& item = items_[selectedIndex_];
        if (item.callback) {
            item.callback();
        }
    }
}

bool MenuSystem::HandleMouseMove(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    int hoveredIndex = GetItemAt(mouseX, mouseY, screenWidth, screenHeight);

    if (hoveredIndex != -1 && hoveredIndex != selectedIndex_ && IsSelectable(hoveredIndex)) {
        SelectItem(hoveredIndex);
        return true;
    }

    return false;
}

bool MenuSystem::HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    int clickedIndex = GetItemAt(mouseX, mouseY, screenWidth, screenHeight);

    if (clickedIndex != -1 && IsSelectable(clickedIndex)) {
        SelectItem(clickedIndex);
        ActivateSelected();
        return true;
    }

    return false;
}

void MenuSystem::Update(double dt) {
    // Animate the selected item (pulsing effect)
    pulseTimer_ += static_cast<float>(dt);
    float pulseSpeed = std::max(0.1f, style_.selectedPulseSpeed);
    float minAlpha = std::clamp(style_.selectedPulseMinAlpha, 0.0f, 1.0f);
    float maxAlpha = std::clamp(style_.selectedPulseMaxAlpha, minAlpha, 1.0f);
    float alphaRange = maxAlpha - minAlpha;
    float oscillation = (std::sin(pulseTimer_ * pulseSpeed) + 1.0f) * 0.5f;
    selectedItemAlpha_ = minAlpha + alphaRange * oscillation;

    float scaleAmplitude = std::max(0.0f, style_.selectedScaleAmplitude);
    selectedItemScale_ = 1.0f + scaleAmplitude * std::sin(pulseTimer_ * pulseSpeed * 0.5f);
}

void MenuSystem::GetRenderData(std::string& outTitle, std::vector<MenuItem>& outItems,
                               int& outSelectedIndex, MenuStyle& outStyle,
                               std::string* outSubtitle, std::string* outFooter,
                               float* outSelectedAlpha, float* outSelectedScale) const {
    outTitle = title_;
    outItems = items_;
    outSelectedIndex = selectedIndex_;
    outStyle = style_;
    if (outSubtitle) {
        *outSubtitle = subtitle_;
    }
    if (outFooter) {
        *outFooter = footer_;
    }
    if (outSelectedAlpha) {
        *outSelectedAlpha = selectedItemAlpha_;
    }
    if (outSelectedScale) {
        *outSelectedScale = selectedItemScale_;
    }
}

void MenuSystem::FindNextSelectableItem() {
    SelectNext();
    EnsureValidSelection();
}

void MenuSystem::FindPreviousSelectableItem() {
    SelectPrevious();
    EnsureValidSelection();
}

int MenuSystem::GetItemAt(int mouseX, int mouseY, int screenWidth, int screenHeight) const {
    // Calculate menu layout (centered)
    float centerX = screenWidth * 0.5f;
    float startY = screenHeight * 0.35f + style_.titleSpacing;
    
    // Check each visible item
    int visibleIndex = 0;
    for (size_t i = 0; i < items_.size(); i++) {
        if (!items_[i].visible) continue;

        float itemY = startY + visibleIndex * style_.itemSpacing;
        float itemHeight = style_.itemFontSize + 10.0f;  // Add some padding

        // Approximate text width (rough estimate)
        float textWidth = items_[i].text.length() * style_.itemFontSize * 0.5f;
        float itemLeft = centerX - textWidth * 0.5f;
        float itemRight = centerX + textWidth * 0.5f;

        if (mouseX >= itemLeft && mouseX <= itemRight &&
            mouseY >= itemY - itemHeight * 0.5f && mouseY <= itemY + itemHeight * 0.5f) {
            return static_cast<int>(i);
        }

        visibleIndex++;
    }

    return -1;
}

void MenuSystem::EnsureValidSelection() {
    if (items_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    if (IsSelectable(selectedIndex_)) {
        return;
    }

    selectedIndex_ = -1;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (IsSelectable(static_cast<int>(i))) {
            selectedIndex_ = static_cast<int>(i);
            break;
        }
    }
}

bool MenuSystem::IsSelectable(int index) const {
    return index >= 0 && index < static_cast<int>(items_.size()) &&
           items_[index].visible && items_[index].enabled && !items_[index].isSeparator;
}

void MenuSystem::SetItemText(int index, const std::string& text) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].text = text;
    }
}

void MenuSystem::SetItemDescription(int index, const std::string& description) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].description = description;
    }
}

void MenuSystem::SetItemShortcutHint(int index, const std::string& shortcutHint) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].shortcutHint = shortcutHint;
    }
}

const MenuSystem::MenuItem* MenuSystem::GetItem(int index) const {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return &items_[index];
    }
    return nullptr;
}
