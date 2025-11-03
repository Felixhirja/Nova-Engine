#pragma once

#include "EditorCommand.h"
#include <vector>
#include <memory>
#include <string>

/**
 * UndoRedoSystem - Manages command history for editor actions
 * Supports undo (Ctrl+Z) and redo (Ctrl+Y/Ctrl+Shift+Z)
 */
class UndoRedoSystem {
public:
    UndoRedoSystem(size_t maxHistorySize = 100);
    
    void ExecuteCommand(std::unique_ptr<EditorCommand> command);
    
    bool CanUndo() const;
    bool CanRedo() const;
    
    void Undo();
    void Redo();
    
    void Clear();
    
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    
    size_t GetHistorySize() const { return history_.size(); }
    size_t GetCurrentIndex() const { return currentIndex_; }
    
private:
    std::vector<std::unique_ptr<EditorCommand>> history_;
    size_t currentIndex_ = 0;
    size_t maxHistorySize_;
    
    void TrimHistory();
};
