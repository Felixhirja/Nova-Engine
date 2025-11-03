#pragma once

#include "../ecs/EntityManager.h"
#include <vector>
#include <set>
#include <functional>

/**
 * SelectionManager - Multi-entity selection and manipulation
 */
class SelectionManager {
public:
    SelectionManager();
    
    void SelectEntity(Entity entity, bool additive = false);
    void DeselectEntity(Entity entity);
    void ClearSelection();
    
    bool IsSelected(Entity entity) const;
    
    const std::set<Entity>& GetSelectedEntities() const { return selectedEntities_; }
    Entity GetPrimarySelection() const { return primarySelection_; }
    
    size_t GetSelectionCount() const { return selectedEntities_.size(); }
    bool HasSelection() const { return !selectedEntities_.empty(); }
    
    void SetOnSelectionChanged(std::function<void()> callback) {
        onSelectionChanged_ = callback;
    }
    
private:
    std::set<Entity> selectedEntities_;
    Entity primarySelection_ = 0;
    std::function<void()> onSelectionChanged_;
    
    void NotifySelectionChanged();
};
