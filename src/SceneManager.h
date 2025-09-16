#pragma once

#include <vector>
#include <string>
#include <functional>
#include "Transform.h"

// Lightweight in-place entity representation used by SceneManager.
struct EntityData {
    std::string name = "entity";
    Transform transform;
    int textureHandle = 0;
    // animation
    int currentFrame = 0;
    double frameTimer = 0.0;
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void AddEntity(const EntityData& e);
    void Update(double dt);

    // Call a function for each entity (pointer for possible mutation)
    void ForEach(const std::function<void(EntityData*)>& fn);

    // Simple save to text file (one entity per line serialized as: name x y z texture)
    bool Save(const std::string& path) const;
    bool Load(const std::string& path);

private:
    std::vector<EntityData> entities_;
};
