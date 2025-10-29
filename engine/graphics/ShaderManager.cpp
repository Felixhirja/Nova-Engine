#include "ShaderManager.h"

#include <iostream>

namespace Nova {
namespace {
    void LogShaderError(const std::string& name, const ShaderProgram& program) {
        const std::string& log = program.GetErrorLog();
        if (!log.empty()) {
            std::cerr << "ShaderManager: shader '" << name << "' failed: " << log << std::endl;
        } else {
            std::cerr << "ShaderManager: shader '" << name << "' failed to compile/link (no log available)" << std::endl;
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderManager::LoadShader(const std::string& name,
                                                         const std::string& vertexPath,
                                                         const std::string& fragmentPath,
                                                         bool forceReload) {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        ShaderRecord& record = it->second;
        if (!record.program) {
            record.program = std::make_shared<ShaderProgram>();
        }

        if (forceReload || !record.program->IsValid()) {
            if (!record.program->LoadFromFiles(vertexPath, fragmentPath)) {
                LogShaderError(name, *record.program);
                return nullptr;
            }
            record.vertexTimestamp = GetLastWriteTime(vertexPath);
            record.fragmentTimestamp = GetLastWriteTime(fragmentPath);
        }

        record.vertexPath = vertexPath;
        record.fragmentPath = fragmentPath;
        return record.program;
    }

    ShaderRecord record;
    record.vertexPath = vertexPath;
    record.fragmentPath = fragmentPath;
    record.program = std::make_shared<ShaderProgram>();

    if (!record.program->LoadFromFiles(vertexPath, fragmentPath)) {
        LogShaderError(name, *record.program);
        return nullptr;
    }

    record.vertexTimestamp = GetLastWriteTime(vertexPath);
    record.fragmentTimestamp = GetLastWriteTime(fragmentPath);

    auto result = shaders_.emplace(name, std::move(record));
    return result.first->second.program;
}

std::shared_ptr<ShaderProgram> ShaderManager::GetShader(const std::string& name) const {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second.program;
    }
    return nullptr;
}

bool ShaderManager::ReloadShader(const std::string& name) {
    auto it = shaders_.find(name);
    if (it == shaders_.end()) {
        return false;
    }

    ShaderRecord& record = it->second;
    if (!record.program) {
        record.program = std::make_shared<ShaderProgram>();
    }

    if (!record.program->LoadFromFiles(record.vertexPath, record.fragmentPath)) {
        LogShaderError(name, *record.program);
        return false;
    }

    record.vertexTimestamp = GetLastWriteTime(record.vertexPath);
    record.fragmentTimestamp = GetLastWriteTime(record.fragmentPath);
    return true;
}

int ShaderManager::ReloadModifiedShaders() {
    int reloaded = 0;

    for (auto& pair : shaders_) {
        const std::string& name = pair.first;
        ShaderRecord& record = pair.second;

        const auto currentVertex = GetLastWriteTime(record.vertexPath);
        const auto currentFragment = GetLastWriteTime(record.fragmentPath);

        const bool vertexChanged = currentVertex != std::filesystem::file_time_type{} &&
                                   record.vertexTimestamp != std::filesystem::file_time_type{} &&
                                   currentVertex != record.vertexTimestamp;
        const bool fragmentChanged = currentFragment != std::filesystem::file_time_type{} &&
                                     record.fragmentTimestamp != std::filesystem::file_time_type{} &&
                                     currentFragment != record.fragmentTimestamp;

        if (vertexChanged || fragmentChanged) {
            if (ReloadShader(name)) {
                ++reloaded;
            }
        }
    }

    return reloaded;
}

int ShaderManager::ReloadAll() {
    int count = 0;
    for (auto& pair : shaders_) {
        if (ReloadShader(pair.first)) {
            ++count;
        }
    }
    return count;
}

void ShaderManager::Clear() {
    for (auto& pair : shaders_) {
        if (pair.second.program) {
            pair.second.program->Cleanup();
            pair.second.program.reset();
        }
    }
    shaders_.clear();
}

bool ShaderManager::HasShader(const std::string& name) const {
    return shaders_.find(name) != shaders_.end();
}

std::filesystem::file_time_type ShaderManager::GetLastWriteTime(const std::string& path) {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return std::filesystem::file_time_type{};
    }
    return time;
}

} // namespace Nova
