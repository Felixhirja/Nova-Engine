#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

#include "ShaderProgram.h"

namespace Nova {

/**
 * ShaderManager centralizes the loading, caching, and hot reloading of GLSL shader programs.
 *
 * The manager keeps one ShaderProgram instance per logical shader name. Subsequent requests for the
 * same shader return a shared_ptr to the cached instance, eliminating redundant compilation work.
 *
 * Basic usage:
 *   ShaderManager manager;
 *   auto shader = manager.LoadShader("core.basic", "shaders/core/basic.vert", "shaders/core/basic.frag");
 *   shader->Use();
 */
class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    /**
     * Load a shader (vertex+fragment pair) identified by a logical name.
     * If the shader was previously loaded, the cached ShaderProgram is returned.
     *
     * @param name        Logical identifier (e.g. "core.basic")
     * @param vertexPath  Path to the vertex shader source file
     * @param fragmentPath Path to the fragment shader source file
     * @param forceReload If true, the shader will be recompiled even if already cached
     * @return Shared pointer to the compiled ShaderProgram, or nullptr on failure
     */
    std::shared_ptr<ShaderProgram> LoadShader(const std::string& name,
                                              const std::string& vertexPath,
                                              const std::string& fragmentPath,
                                              bool forceReload = false);

    /**
     * Retrieve a previously loaded shader by name.
     * Returns nullptr if the shader has not been loaded yet.
     */
    std::shared_ptr<ShaderProgram> GetShader(const std::string& name) const;

    /**
     * Reload a shader from disk using its original source paths.
     * Returns true if the reload succeeded.
     */
    bool ReloadShader(const std::string& name);

    /**
     * Iterate over all cached shaders and reload any whose source files have changed on disk.
     *
     * @return The number of shaders successfully reloaded.
     */
    int ReloadModifiedShaders();

    /**
     * Reload every cached shader regardless of timestamp.
     * Returns the number of shaders successfully reloaded.
     */
    int ReloadAll();

    /**
     * Remove all shaders from the cache and release their GPU resources.
     */
    void Clear();

    /**
     * Check whether a shader with the given name is present in the cache.
     */
    bool HasShader(const std::string& name) const;

private:
    struct ShaderRecord {
        std::string vertexPath;
        std::string fragmentPath;
        std::shared_ptr<ShaderProgram> program;
        std::filesystem::file_time_type vertexTimestamp{};
        std::filesystem::file_time_type fragmentTimestamp{};
    };

    std::unordered_map<std::string, ShaderRecord> shaders_;

    static std::filesystem::file_time_type GetLastWriteTime(const std::string& path);
};

} // namespace Nova
