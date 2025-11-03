#include "EditorCommands.h"

DeleteEntityCommand::DeleteEntityCommand(EntityManager* em, Entity entity)
    : entityManager_(em), entity_(entity) {
}

void DeleteEntityCommand::Execute() {
    if (!executed_ && entityManager_ && entityManager_->IsAlive(entity_)) {
        BackupComponents();
        entityManager_->DestroyEntity(entity_);
        executed_ = true;
    }
}

void DeleteEntityCommand::Undo() {
    if (executed_ && entityManager_) {
        Entity newEntity = entityManager_->CreateEntity();
        entity_ = newEntity;
        RestoreComponents();
    }
}

void DeleteEntityCommand::BackupComponents() {
    if (!entityManager_) return;
    
    if (auto* pos = entityManager_->GetComponent<Position>(entity_)) {
        backup_.position = std::make_shared<Position>(*pos);
    }
    if (auto* vel = entityManager_->GetComponent<Velocity>(entity_)) {
        backup_.velocity = std::make_shared<Velocity>(*vel);
    }
    if (auto* draw = entityManager_->GetComponent<DrawComponent>(entity_)) {
        backup_.draw = std::make_shared<DrawComponent>(*draw);
    }
    if (auto* player = entityManager_->GetComponent<PlayerController>(entity_)) {
        backup_.player = std::make_shared<PlayerController>(*player);
    }
    if (auto* name = entityManager_->GetComponent<Name>(entity_)) {
        backup_.name = std::make_shared<Name>(*name);
    }
}

void DeleteEntityCommand::RestoreComponents() {
    if (!entityManager_) return;
    
    if (backup_.position) {
        entityManager_->AddComponent(entity_, backup_.position);
    }
    if (backup_.velocity) {
        entityManager_->AddComponent(entity_, backup_.velocity);
    }
    if (backup_.draw) {
        entityManager_->AddComponent(entity_, backup_.draw);
    }
    if (backup_.player) {
        entityManager_->AddComponent(entity_, backup_.player);
    }
    if (backup_.name) {
        entityManager_->AddComponent(entity_, backup_.name);
    }
}

void DuplicateEntityCommand::Execute() {
    if (!executed_ && entityManager_ && entityManager_->IsAlive(sourceEntity_)) {
        duplicatedEntity_ = entityManager_->CreateEntity();
        
        if (auto* srcPos = entityManager_->GetComponent<Position>(sourceEntity_)) {
            auto newPos = std::make_shared<Position>(*srcPos);
            newPos->x += offsetX_;
            newPos->y += offsetY_;
            newPos->z += offsetZ_;
            entityManager_->AddComponent(duplicatedEntity_, newPos);
        }
        
        if (auto* srcVel = entityManager_->GetComponent<Velocity>(sourceEntity_)) {
            entityManager_->AddComponent(duplicatedEntity_, std::make_shared<Velocity>(*srcVel));
        }
        
        if (auto* srcDraw = entityManager_->GetComponent<DrawComponent>(sourceEntity_)) {
            entityManager_->AddComponent(duplicatedEntity_, std::make_shared<DrawComponent>(*srcDraw));
        }
        
        if (auto* srcName = entityManager_->GetComponent<Name>(sourceEntity_)) {
            auto newName = std::make_shared<Name>(*srcName);
            newName->value += " (Copy)";
            entityManager_->AddComponent(duplicatedEntity_, newName);
        }
        
        executed_ = true;
    }
}

void DuplicateEntityCommand::Undo() {
    if (executed_ && entityManager_ && entityManager_->IsAlive(duplicatedEntity_)) {
        entityManager_->DestroyEntity(duplicatedEntity_);
    }
}
