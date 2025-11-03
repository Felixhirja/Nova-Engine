#pragma once

#include <memory>
#include <string>

/**
 * EditorCommand - Base class for undoable editor actions
 * Implements the Command pattern for undo/redo functionality
 */
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
    
    bool IsExecuted() const { return executed_; }
    
protected:
    bool executed_ = false;
};
