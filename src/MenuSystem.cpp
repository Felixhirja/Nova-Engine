#include "MenuSystem.h"
#include <cmath>
#include <algorithm>

MenuSystem::MenuSystem(const std::string& title)
    : title_(title)
    , selectedIndex_(0)
    , active_(true)
    , pulseTimer_(0.0f)
    , selectedItemAlpha_(1.0f)
{
}

void MenuSystem::AddItem(const std::string& text, std::function<void()> callback) {
    items_.emplace_back(text, callback);
    
    // If this is the first enabled item, select it
    if (selectedIndex_ == -1 || (selectedIndex_ == 0 && items_.size() == 1)) {
        selectedIndex_ = 0;
    }
}

void MenuSystem::AddItem(const MenuItem& item) {
    items_.push_back(item);
    
    // If this is the first enabled item, select it
    if (selectedIndex_ == -1 || (selectedIndex_ == 0 && items_.size() == 1)) {
        selectedIndex_ = 0;
    }
}

void MenuSystem::ClearItems() {
    items_.clear();
    selectedIndex_ = 0;
}

void MenuSystem::SetItemEnabled(int index, bool enabled) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].enabled = enabled;
        
        // If we disabled the currently selected item, find a new one
        if (!enabled && index == selectedIndex_) {
            FindNextSelectableItem();
        }
    }
}

void MenuSystem::SetItemVisible(int index, bool visible) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[index].visible = visible;
        
        // If we hid the currently selected item, find a new one
        if (!visible && index == selectedIndex_) {
            FindNextSelectableItem();
        }
    }
}

void MenuSystem::SelectNext() {
    if (items_.empty()) return;
    
    int startIndex = selectedIndex_;
    do {
        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(items_.size());
        
        if (items_[selectedIndex_].enabled && items_[selectedIndex_].visible) {
            return;
        }
        
        // Prevent infinite loop if no items are selectable
        if (selectedIndex_ == startIndex) {
            break;
        }
    } while (true);
}

void MenuSystem::SelectPrevious() {
    if (items_.empty()) return;
    
    int startIndex = selectedIndex_;
    do {
        selectedIndex_--;
        if (selectedIndex_ < 0) {
            selectedIndex_ = static_cast<int>(items_.size()) - 1;
        }
        
        if (items_[selectedIndex_].enabled && items_[selectedIndex_].visible) {
            return;
        }
        
        // Prevent infinite loop if no items are selectable
        if (selectedIndex_ == startIndex) {
            break;
        }
    } while (true);
}

void MenuSystem::SelectItem(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].enabled && items_[index].visible) {
            selectedIndex_ = index;
        }
    }
}

void MenuSystem::ActivateSelected() {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        const auto& item = items_[selectedIndex_];
        if (item.enabled && item.visible && item.callback) {
            item.callback();
        }
    }
}

bool MenuSystem::HandleMouseMove(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    int hoveredIndex = GetItemAt(mouseX, mouseY, screenWidth, screenHeight);
    
    if (hoveredIndex != -1 && hoveredIndex != selectedIndex_) {
        SelectItem(hoveredIndex);
        return true;
    }
    
    return false;
}

bool MenuSystem::HandleMouseClick(int mouseX, int mouseY, int screenWidth, int screenHeight) {
    int clickedIndex = GetItemAt(mouseX, mouseY, screenWidth, screenHeight);
    
    if (clickedIndex != -1) {
        SelectItem(clickedIndex);
        ActivateSelected();
        return true;
    }
    
    return false;
}

void MenuSystem::Update(double dt) {
    // Animate the selected item (pulsing effect)
    pulseTimer_ += static_cast<float>(dt);
    selectedItemAlpha_ = 0.7f + 0.3f * std::sin(pulseTimer_ * 3.0f);
}

void MenuSystem::GetRenderData(std::string& outTitle, std::vector<MenuItem>& outItems,
                               int& outSelectedIndex, MenuStyle& outStyle) const {
    outTitle = title_;
    outItems = items_;
    outSelectedIndex = selectedIndex_;
    outStyle = style_;
}

void MenuSystem::FindNextSelectableItem() {
    SelectNext();
}

void MenuSystem::FindPreviousSelectableItem() {
    SelectPrevious();
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
