
#include "SceneManager.h"
#include <fstream>
#include <sstream>

SceneManager::SceneManager() {}

SceneManager::~SceneManager() {
    entities_.clear();
}

void SceneManager::AddEntity(const EntityData& e) {
    entities_.push_back(e);
}

void SceneManager::Update(double dt) {
    // Very small placeholder behavior: move all entities along X slowly
    for (auto &e : entities_) {
        e.transform.Translate(dt * 0.5, 0.0, 0.0);
    }
}

void SceneManager::ForEach(const std::function<void(EntityData*)>& fn) {
    for (auto &e : entities_) {
        fn(&e);
    }
}

bool SceneManager::Save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;
    for (auto const &e : entities_) {
        out << e.name << " " << e.transform.x << " " << e.transform.y << " " << e.transform.z << " " << e.textureHandle << "\n";
    }
    return true;
}

bool SceneManager::Load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    entities_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        EntityData e;
        ss >> e.name >> e.transform.x >> e.transform.y >> e.transform.z >> e.textureHandle;
        entities_.push_back(e);
    }
    return true;
}
