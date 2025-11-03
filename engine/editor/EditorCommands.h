#pragma once

#include "EditorCommand.h"
#include "../ecs/EntityManager.h"
#include "../ecs/Components.h"
#include <memory>
#include <vector>
#include <functional>

/**
 * CreateEntityCommand - Create a new entity
 */
class CreateEntityCommand : public EditorCommand {
public:
    CreateEntityCommand(EntityManager* em, std::function<Entity()> createFunc)
        : entityManager_(em), createFunc_(createFunc) {}
    
    void Execute() override {
        if (!executed_) {
            createdEntity_ = createFunc_();
            executed_ = true;
        }
    }
    
    void Undo() override {
        if (executed_ && entityManager_) {
            entityManager_->DestroyEntity(createdEntity_);
        }
    }
    
    std::string GetDescription() const override {
        return "Create Entity";
    }
    
    Entity GetCreatedEntity() const { return createdEntity_; }
    
private:
    EntityManager* entityManager_;
    std::function<Entity()> createFunc_;
    Entity createdEntity_ = 0;
};

/**
 * DeleteEntityCommand - Delete an entity
 */
class DeleteEntityCommand : public EditorCommand {
public:
    DeleteEntityCommand(EntityManager* em, Entity entity);
    
    void Execute() override;
    void Undo() override;
    
    std::string GetDescription() const override {
        return "Delete Entity " + std::to_string(entity_);
    }
    
private:
    EntityManager* entityManager_;
    Entity entity_;
    
    // Store component data for undo
    struct ComponentBackup {
        std::shared_ptr<Position> position;
        std::shared_ptr<Velocity> velocity;
        std::shared_ptr<DrawComponent> draw;
        std::shared_ptr<PlayerController> player;
        std::shared_ptr<Name> name;
    };
    ComponentBackup backup_;
    
    void BackupComponents();
    void RestoreComponents();
};

/**
 * MoveEntityCommand - Move entity to new position
 */
class MoveEntityCommand : public EditorCommand {
public:
    MoveEntityCommand(EntityManager* em, Entity entity, double x, double y, double z)
        : entityManager_(em), entity_(entity), newX_(x), newY_(y), newZ_(z) {}
    
    void Execute() override {
        if (!executed_ && entityManager_) {
            if (auto* pos = entityManager_->GetComponent<Position>(entity_)) {
                oldX_ = pos->x;
                oldY_ = pos->y;
                oldZ_ = pos->z;
                pos->x = newX_;
                pos->y = newY_;
                pos->z = newZ_;
                executed_ = true;
            }
        }
    }
    
    void Undo() override {
        if (executed_ && entityManager_) {
            if (auto* pos = entityManager_->GetComponent<Position>(entity_)) {
                pos->x = oldX_;
                pos->y = oldY_;
                pos->z = oldZ_;
            }
        }
    }
    
    std::string GetDescription() const override {
        return "Move Entity " + std::to_string(entity_);
    }
    
private:
    EntityManager* entityManager_;
    Entity entity_;
    double newX_, newY_, newZ_;
    double oldX_ = 0, oldY_ = 0, oldZ_ = 0;
};

/**
 * DuplicateEntityCommand - Duplicate an existing entity
 */
class DuplicateEntityCommand : public EditorCommand {
public:
    DuplicateEntityCommand(EntityManager* em, Entity source, double offsetX = 5.0, double offsetY = 0.0, double offsetZ = 0.0)
        : entityManager_(em), sourceEntity_(source), offsetX_(offsetX), offsetY_(offsetY), offsetZ_(offsetZ) {}
    
    void Execute() override;
    void Undo() override;
    
    std::string GetDescription() const override {
        return "Duplicate Entity " + std::to_string(sourceEntity_);
    }
    
    Entity GetDuplicatedEntity() const { return duplicatedEntity_; }
    
private:
    EntityManager* entityManager_;
    Entity sourceEntity_;
    Entity duplicatedEntity_ = 0;
    double offsetX_, offsetY_, offsetZ_;
};

/**
 * MultiEntityCommand - Execute multiple commands as one atomic operation
 */
class MultiEntityCommand : public EditorCommand {
public:
    MultiEntityCommand(const std::string& description) : description_(description) {}
    
    void AddCommand(std::unique_ptr<EditorCommand> command) {
        commands_.push_back(std::move(command));
    }
    
    void Execute() override {
        for (auto& cmd : commands_) {
            if (!cmd->IsExecuted()) {
                cmd->Execute();
            }
        }
        executed_ = true;
    }
    
    void Undo() override {
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->Undo();
        }
    }
    
    std::string GetDescription() const override {
        return description_;
    }
    
private:
    std::vector<std::unique_ptr<EditorCommand>> commands_;
    std::string description_;
};
