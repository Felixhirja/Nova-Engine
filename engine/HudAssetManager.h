#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>

// Forward declarations for cross-platform compatibility
#ifdef USE_SDL
#include <SDL.h>
#endif

#if defined(USE_GLFW) || defined(USE_SDL)
#include <glad/glad.h>
#endif

/**
 * Automated HUD Asset Management System
 * 
 * Provides comprehensive auto-loading capabilities for HUD assets:
 * - JSON-based configuration with asset discovery
 * - Cross-platform texture management (OpenGL/SDL)
 * - Dynamic asset path resolution
 * - Cached loading with automatic invalidation
 * - Layout management and anchoring system
 * 
 * Integration with Nova Engine auto-loading pattern:
 * - Follows same principles as CargoContainer auto-loading
 * - Automatic asset discovery and registration
 * - Build system integration for configuration updates
 * - Comprehensive error handling and fallbacks
 */

struct HudAssetConfig {
    std::string asset_path;
    std::string type;
    struct {
        int width = 1920;
        int height = 1080;
    } target_resolution;
    bool preserve_aspect_ratio = true;
    std::string blend_mode = "blend";
    int layer = 1;
    bool enabled = true;
    std::string description;
};

struct HudAnchor {
    int x = 0;
    int y = 0;
    std::string anchor_type = "top_left";
};

struct HudLayout {
    std::string name;
    std::vector<std::string> active_huds;
    std::unordered_map<std::string, HudAnchor> anchors;
};

struct HudTexture {
    // OpenGL texture data
    unsigned int gl_texture_id = 0;
    bool gl_loaded = false;
    bool gl_failed = false;
    
    // SDL texture data
#ifdef USE_SDL
    SDL_Texture* sdl_texture = nullptr;
#else
    void* sdl_texture = nullptr;  // Placeholder when SDL not available
#endif
    bool sdl_loaded = false;
    bool sdl_failed = false;
    
    // Common properties
    int width = 0;
    int height = 0;
    std::string asset_path;
    long last_modified = 0;  // For cache invalidation
};

class HudAssetManager {
public:
    static HudAssetManager& GetInstance() {
        static HudAssetManager instance;
        return instance;
    }

    // Core auto-loading functionality
    bool LoadConfiguration(const std::string& config_path = "assets/ui/config/hud_config.json");
    bool AutoDiscoverAssets();
    void RefreshAssetCache();
    
    // Asset management
    bool LoadHudAsset(const std::string& asset_name);
    HudTexture* GetHudTexture(const std::string& asset_name);
    bool IsAssetLoaded(const std::string& asset_name);
    void UnloadAsset(const std::string& asset_name);
    void UnloadAllAssets();
    
    // Layout management
    bool SetActiveLayout(const std::string& layout_name);
    HudLayout* GetCurrentLayout();
    std::vector<std::string> GetActiveHudNames();
    HudAnchor GetHudAnchor(const std::string& hud_name);
    
    // Cross-platform rendering support
    bool EnsureTextureLoaded(const std::string& asset_name, bool use_opengl = true);
    unsigned int GetOpenGLTexture(const std::string& asset_name);
#ifdef USE_SDL
    SDL_Texture* GetSDLTexture(const std::string& asset_name, SDL_Renderer* renderer);
#endif
    
    // Asset discovery and path resolution
    std::string ResolveAssetPath(const std::string& relative_path);
    std::vector<std::string> DiscoverAssetsInDirectory(const std::string& directory);
    bool RegisterAsset(const std::string& name, const HudAssetConfig& config);
    
    // Configuration access
    HudAssetConfig* GetAssetConfig(const std::string& asset_name);
    std::vector<std::string> GetAllAssetNames();
    bool IsAutoDiscoveryEnabled() const { return auto_discovery_enabled_; }
    
    // Error handling and diagnostics
    std::string GetLastError() const { return last_error_; }
    void ClearError() { last_error_.clear(); }
    bool HasErrors() const { return !last_error_.empty(); }
    
    // Debug and diagnostics
    void DumpConfiguration();
    void DumpLoadedAssets();
    std::string GetSystemStatus();

private:
    HudAssetManager() = default;
    ~HudAssetManager();
    
    // Internal loading functions
    bool LoadAssetFromFile(const std::string& asset_name, const HudAssetConfig& config);
    bool LoadSVGAsset(const std::string& asset_path, HudTexture& texture);
    bool LoadImageAsset(const std::string& asset_path, HudTexture& texture);
    
    // JSON parsing helpers
    bool ParseJsonConfiguration(const std::string& json_content);
    
    // Path and file utilities
    bool FileExists(const std::string& path);
    long GetFileModificationTime(const std::string& path);
    std::string ReadFileContents(const std::string& path);
    std::vector<std::string> GetFilesInDirectory(const std::string& directory, 
                                                 const std::vector<std::string>& extensions);
    
    // Texture management helpers
    void CleanupTexture(HudTexture& texture);
    bool CreateOpenGLTexture(const std::vector<uint8_t>& pixel_data, int width, int height, HudTexture& texture);
#ifdef USE_SDL
    bool CreateSDLTexture(SDL_Surface* surface, SDL_Renderer* renderer, HudTexture& texture);
#endif
    
    // Internal state
    std::unordered_map<std::string, HudAssetConfig> asset_configs_;
    std::unordered_map<std::string, HudTexture> loaded_textures_;
    std::unordered_map<std::string, HudLayout> layouts_;
    std::string current_layout_ = "default";
    
    // Configuration settings
    bool auto_discovery_enabled_ = true;
    bool cache_textures_ = true;
    std::string ui_graphics_path_ = "assets/ui/graphics/";
    std::vector<std::string> supported_formats_ = {".svg", ".png", ".jpg"};
    
    // Error tracking
    std::string last_error_;
    
    // Asset discovery cache
    std::vector<std::string> discovered_assets_;
    long last_discovery_time_ = 0;
    
    // Helper function declarations
    void SetError(const std::string& error);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);
    void LogError(const std::string& message);
};

// Convenience macros for auto-loading HUD integration
#define HUD_AUTO_LOAD(asset_name) \
    HudAssetManager::GetInstance().EnsureTextureLoaded(asset_name)

#define HUD_GET_TEXTURE_GL(asset_name) \
    HudAssetManager::GetInstance().GetOpenGLTexture(asset_name)

#ifdef USE_SDL
#define HUD_GET_TEXTURE_SDL(asset_name, renderer) \
    HudAssetManager::GetInstance().GetSDLTexture(asset_name, renderer)
#endif

#define HUD_REGISTER_ASSET(name, path, type) \
    do { \
        HudAssetConfig config; \
        config.asset_path = path; \
        config.type = type; \
        HudAssetManager::GetInstance().RegisterAsset(name, config); \
    } while(0)

// Integration with Nova Engine's auto-loading system
namespace HudSystemIntegration {
    // Auto-initialization hook for engine startup
    bool InitializeHudSystem();
    
    // Auto-cleanup hook for engine shutdown
    void ShutdownHudSystem();
    
    // Auto-discovery hook for asset changes
    bool RefreshHudAssets();
    
    // Integration with Viewport3D rendering
    bool RenderHudLayer(const std::string& layer_name, int viewport_width, int viewport_height);
    
    // Error reporting integration
    void ReportHudSystemStatus();
}