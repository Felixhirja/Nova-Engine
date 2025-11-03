#include "UndoRedoSystem.h"
#include <algorithm>

UndoRedoSystem::UndoRedoSystem(size_t maxHistorySize)
    : maxHistorySize_(maxHistorySize) {
}

void UndoRedoSystem::ExecuteCommand(std::unique_ptr<EditorCommand> command) {
    if (!command) return;
    
    command->Execute();
    
    // Remove any redo history when executing a new command
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }
    
    history_.push_back(std::move(command));
    currentIndex_ = history_.size();
    
    TrimHistory();
}

bool UndoRedoSystem::CanUndo() const {
    return currentIndex_ > 0;
}

bool UndoRedoSystem::CanRedo() const {
    return currentIndex_ < history_.size();
}

void UndoRedoSystem::Undo() {
    if (!CanUndo()) return;
    
    --currentIndex_;
    history_[currentIndex_]->Undo();
}

void UndoRedoSystem::Redo() {
    if (!CanRedo()) return;
    
    history_[currentIndex_]->Execute();
    ++currentIndex_;
}

void UndoRedoSystem::Clear() {
    history_.clear();
    currentIndex_ = 0;
}

std::string UndoRedoSystem::GetUndoDescription() const {
    if (!CanUndo()) return "";
    return history_[currentIndex_ - 1]->GetDescription();
}

std::string UndoRedoSystem::GetRedoDescription() const {
    if (!CanRedo()) return "";
    return history_[currentIndex_]->GetDescription();
}

void UndoRedoSystem::TrimHistory() {
    if (history_.size() > maxHistorySize_) {
        size_t excess = history_.size() - maxHistorySize_;
        history_.erase(history_.begin(), history_.begin() + excess);
        currentIndex_ = std::min(currentIndex_, history_.size());
    }
}
