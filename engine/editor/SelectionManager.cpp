#include "SelectionManager.h"

SelectionManager::SelectionManager() {
}

void SelectionManager::SelectEntity(Entity entity, bool additive) {
    if (!additive) {
        selectedEntities_.clear();
    }
    
    selectedEntities_.insert(entity);
    primarySelection_ = entity;
    
    NotifySelectionChanged();
}

void SelectionManager::DeselectEntity(Entity entity) {
    selectedEntities_.erase(entity);
    
    if (primarySelection_ == entity) {
        primarySelection_ = selectedEntities_.empty() ? 0 : *selectedEntities_.begin();
    }
    
    NotifySelectionChanged();
}

void SelectionManager::ClearSelection() {
    selectedEntities_.clear();
    primarySelection_ = 0;
    NotifySelectionChanged();
}

bool SelectionManager::IsSelected(Entity entity) const {
    return selectedEntities_.find(entity) != selectedEntities_.end();
}

void SelectionManager::NotifySelectionChanged() {
    if (onSelectionChanged_) {
        onSelectionChanged_();
    }
}
