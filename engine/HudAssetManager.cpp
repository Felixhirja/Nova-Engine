#include "HudAssetManager.h"
#include "SVGSurfaceLoader.h"
#include "SimpleJson.h"
#include <filesystem>
#include <sstream>
#include <algorithm>

// External dependencies are declared in SVGSurfaceLoader.h
#ifdef USE_SDL
extern SDL_Surface* LoadSVGSurface(const std::string& path);
extern SDL_Texture* compat_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
extern void compat_DestroySurface(SDL_Surface* surface);
#endif

HudAssetManager::~HudAssetManager() {
    UnloadAllAssets();
}

bool HudAssetManager::LoadConfiguration(const std::string& config_path) {
    SetError("");  // Clear previous errors
    
    if (!FileExists(config_path)) {
        SetError("Configuration file not found: " + config_path);
        return false;
    }
    
    std::string json_content = ReadFileContents(config_path);
    if (json_content.empty()) {
        SetError("Failed to read configuration file: " + config_path);
        return false;
    }
    
    LogInfo("Loading HUD configuration from: " + config_path);
    
    if (!ParseJsonConfiguration(json_content)) {
        SetError("Failed to parse JSON configuration");
        return false;
    }
    
    // Auto-discover assets if enabled
    if (auto_discovery_enabled_) {
        AutoDiscoverAssets();
    }
    
    LogInfo("HUD Asset Manager configuration loaded successfully");
    return true;
}

bool HudAssetManager::AutoDiscoverAssets() {
    LogInfo("Auto-discovering HUD assets in: " + ui_graphics_path_);
    
    discovered_assets_ = DiscoverAssetsInDirectory(ui_graphics_path_);
    last_discovery_time_ = std::time(nullptr);
    
    // Register any newly discovered assets that aren't already configured
    for (const std::string& asset_path : discovered_assets_) {
        std::string asset_name = std::filesystem::path(asset_path).stem().string();
        
        if (asset_configs_.find(asset_name) == asset_configs_.end()) {
            HudAssetConfig config;
            config.asset_path = asset_path;
            config.type = std::filesystem::path(asset_path).extension().string().substr(1); // Remove dot
            config.description = "Auto-discovered asset: " + asset_name;
            
            RegisterAsset(asset_name, config);
            LogInfo("Auto-registered HUD asset: " + asset_name + " (" + asset_path + ")");
        }
    }
    
    return true;
}

void HudAssetManager::RefreshAssetCache() {
    LogInfo("Refreshing HUD asset cache");
    
    // Check for file modifications and reload if necessary
    for (auto& [name, texture] : loaded_textures_) {
        long current_time = GetFileModificationTime(texture.asset_path);
        if (current_time > texture.last_modified) {
            LogInfo("Asset changed, reloading: " + name);
            UnloadAsset(name);
            LoadHudAsset(name);
        }
    }
    
    // Re-run auto discovery if enabled
    if (auto_discovery_enabled_) {
        AutoDiscoverAssets();
    }
}

bool HudAssetManager::LoadHudAsset(const std::string& asset_name) {
    auto config_it = asset_configs_.find(asset_name);
    if (config_it == asset_configs_.end()) {
        SetError("Asset configuration not found: " + asset_name);
        return false;
    }
    
    const HudAssetConfig& config = config_it->second;
    
    if (!config.enabled) {
        LogInfo("Asset disabled, skipping load: " + asset_name);
        return true;  // Not an error, just disabled
    }
    
    // Check if already loaded and up-to-date
    auto texture_it = loaded_textures_.find(asset_name);
    if (texture_it != loaded_textures_.end()) {
        long current_time = GetFileModificationTime(config.asset_path);
        if (current_time <= texture_it->second.last_modified) {
            return true;  // Already loaded and current
        } else {
            // File changed, unload and reload
            UnloadAsset(asset_name);
        }
    }
    
    LogInfo("Loading HUD asset: " + asset_name + " from " + config.asset_path);
    
    if (!LoadAssetFromFile(asset_name, config)) {
        SetError("Failed to load asset file: " + asset_name);
        return false;
    }
    
    return true;
}

HudTexture* HudAssetManager::GetHudTexture(const std::string& asset_name) {
    auto it = loaded_textures_.find(asset_name);
    if (it != loaded_textures_.end()) {
        return &it->second;
    }
    
    // Try to load it if not found
    if (LoadHudAsset(asset_name)) {
        it = loaded_textures_.find(asset_name);
        if (it != loaded_textures_.end()) {
            return &it->second;
        }
    }
    
    return nullptr;
}

bool HudAssetManager::IsAssetLoaded(const std::string& asset_name) {
    auto it = loaded_textures_.find(asset_name);
    return it != loaded_textures_.end() && (it->second.gl_loaded || it->second.sdl_loaded);
}

void HudAssetManager::UnloadAsset(const std::string& asset_name) {
    auto it = loaded_textures_.find(asset_name);
    if (it != loaded_textures_.end()) {
        CleanupTexture(it->second);
        loaded_textures_.erase(it);
        LogInfo("Unloaded HUD asset: " + asset_name);
    }
}

void HudAssetManager::UnloadAllAssets() {
    LogInfo("Unloading all HUD assets");
    
    for (auto& [name, texture] : loaded_textures_) {
        CleanupTexture(texture);
    }
    loaded_textures_.clear();
}

bool HudAssetManager::SetActiveLayout(const std::string& layout_name) {
    auto it = layouts_.find(layout_name);
    if (it == layouts_.end()) {
        SetError("Layout not found: " + layout_name);
        return false;
    }
    
    current_layout_ = layout_name;
    LogInfo("Set active HUD layout: " + layout_name);
    
    // Pre-load all assets for this layout
    const HudLayout& layout = it->second;
    for (const std::string& asset_name : layout.active_huds) {
        if (!LoadHudAsset(asset_name)) {
            LogWarning("Failed to pre-load asset for layout: " + asset_name);
        }
    }
    
    return true;
}

HudLayout* HudAssetManager::GetCurrentLayout() {
    auto it = layouts_.find(current_layout_);
    return (it != layouts_.end()) ? &it->second : nullptr;
}

std::vector<std::string> HudAssetManager::GetActiveHudNames() {
    HudLayout* layout = GetCurrentLayout();
    return layout ? layout->active_huds : std::vector<std::string>();
}

HudAnchor HudAssetManager::GetHudAnchor(const std::string& hud_name) {
    HudLayout* layout = GetCurrentLayout();
    if (layout) {
        auto it = layout->anchors.find(hud_name);
        if (it != layout->anchors.end()) {
            return it->second;
        }
    }
    
    // Return default anchor
    return HudAnchor{};
}

bool HudAssetManager::EnsureTextureLoaded(const std::string& asset_name, bool use_opengl) {
    if (!LoadHudAsset(asset_name)) {
        return false;
    }
    
    HudTexture* texture = GetHudTexture(asset_name);
    if (!texture) {
        return false;
    }
    
    if (use_opengl && !texture->gl_loaded && !texture->gl_failed) {
        // Load OpenGL texture if needed
        HudAssetConfig* config = GetAssetConfig(asset_name);
        if (config && config->type == "svg") {
            return LoadSVGAsset(config->asset_path, *texture);
        }
    }
    
    return texture->gl_loaded || texture->sdl_loaded;
}

unsigned int HudAssetManager::GetOpenGLTexture(const std::string& asset_name) {
    if (!EnsureTextureLoaded(asset_name, true)) {
        return 0;
    }
    
    HudTexture* texture = GetHudTexture(asset_name);
    return texture ? texture->gl_texture_id : 0;
}

#ifdef USE_SDL
SDL_Texture* HudAssetManager::GetSDLTexture(const std::string& asset_name, SDL_Renderer* renderer) {
    if (!EnsureTextureLoaded(asset_name, false)) {
        return nullptr;
    }
    
    HudTexture* texture = GetHudTexture(asset_name);
    if (!texture || !renderer) {
        return nullptr;
    }
    
    // Create SDL texture if not already created
    if (!texture->sdl_loaded && !texture->sdl_failed) {
        HudAssetConfig* config = GetAssetConfig(asset_name);
        if (config) {
            SDL_Surface* surface = LoadSVGSurface(config->asset_path);
            if (surface) {
                CreateSDLTexture(surface, renderer, *texture);
                compat_DestroySurface(surface);
            }
        }
    }
    
    return texture->sdl_texture;
}
#endif

std::string HudAssetManager::ResolveAssetPath(const std::string& relative_path) {
    // Simple path resolution - in a full implementation, this would handle
    // multiple search paths, environment variables, etc.
    if (relative_path.find("assets/") == 0) {
        return relative_path;  // Already absolute from project root
    }
    
    return ui_graphics_path_ + relative_path;
}

std::vector<std::string> HudAssetManager::DiscoverAssetsInDirectory(const std::string& directory) {
    std::vector<std::string> assets;
    
    try {
        if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    
                    if (std::find(supported_formats_.begin(), supported_formats_.end(), extension) 
                        != supported_formats_.end()) {
                        assets.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LogError("Error scanning directory " + directory + ": " + e.what());
    }
    
    return assets;
}

bool HudAssetManager::RegisterAsset(const std::string& name, const HudAssetConfig& config) {
    asset_configs_[name] = config;
    LogInfo("Registered HUD asset: " + name);
    return true;
}

HudAssetConfig* HudAssetManager::GetAssetConfig(const std::string& asset_name) {
    auto it = asset_configs_.find(asset_name);
    return (it != asset_configs_.end()) ? &it->second : nullptr;
}

std::vector<std::string> HudAssetManager::GetAllAssetNames() {
    std::vector<std::string> names;
    for (const auto& [name, config] : asset_configs_) {
        names.push_back(name);
    }
    return names;
}

void HudAssetManager::DumpConfiguration() {
    std::cout << "\n=== HUD Asset Manager Configuration ===" << std::endl;
    std::cout << "Auto-discovery enabled: " << (auto_discovery_enabled_ ? "Yes" : "No") << std::endl;
    std::cout << "Cache textures: " << (cache_textures_ ? "Yes" : "No") << std::endl;
    std::cout << "UI graphics path: " << ui_graphics_path_ << std::endl;
    std::cout << "Current layout: " << current_layout_ << std::endl;
    
    std::cout << "\nConfigured assets (" << asset_configs_.size() << "):" << std::endl;
    for (const auto& [name, config] : asset_configs_) {
        std::cout << "  " << name << " -> " << config.asset_path 
                  << " (" << config.type << ")" << std::endl;
    }
    
    std::cout << "\nLoaded textures (" << loaded_textures_.size() << "):" << std::endl;
    for (const auto& [name, texture] : loaded_textures_) {
        std::cout << "  " << name << " [GL:" << (texture.gl_loaded ? "✓" : "✗") 
                  << ", SDL:" << (texture.sdl_loaded ? "✓" : "✗") << "]" << std::endl;
    }
    
    std::cout << "\nLayouts (" << layouts_.size() << "):" << std::endl;
    for (const auto& [name, layout] : layouts_) {
        std::cout << "  " << name << " (" << layout.active_huds.size() << " HUDs)" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
}

void HudAssetManager::DumpLoadedAssets() {
    std::cout << "\n=== Loaded HUD Assets ===" << std::endl;
    for (const auto& [name, texture] : loaded_textures_) {
        std::cout << name << ":" << std::endl;
        std::cout << "  Path: " << texture.asset_path << std::endl;
        std::cout << "  Size: " << texture.width << "x" << texture.height << std::endl;
        std::cout << "  OpenGL: " << (texture.gl_loaded ? "Loaded" : "Not loaded") << std::endl;
        std::cout << "  SDL: " << (texture.sdl_loaded ? "Loaded" : "Not loaded") << std::endl;
        std::cout << "  Modified: " << texture.last_modified << std::endl;
    }
    std::cout << "=========================\n" << std::endl;
}

std::string HudAssetManager::GetSystemStatus() {
    std::ostringstream status;
    status << "HudAssetManager Status:\n";
    status << "  Assets configured: " << asset_configs_.size() << "\n";
    status << "  Assets loaded: " << loaded_textures_.size() << "\n";
    status << "  Current layout: " << current_layout_ << "\n";
    status << "  Auto-discovery: " << (auto_discovery_enabled_ ? "enabled" : "disabled") << "\n";
    status << "  Last error: " << (last_error_.empty() ? "none" : last_error_) << "\n";
    return status.str();
}

// Private implementation methods

bool HudAssetManager::LoadAssetFromFile(const std::string& asset_name, const HudAssetConfig& config) {
    std::string resolved_path = ResolveAssetPath(config.asset_path);
    
    if (!FileExists(resolved_path)) {
        SetError("Asset file not found: " + resolved_path);
        return false;
    }
    
    HudTexture texture;
    texture.asset_path = resolved_path;
    texture.last_modified = GetFileModificationTime(resolved_path);
    
    // Load based on asset type
    bool success = false;
    if (config.type == "svg") {
        success = LoadSVGAsset(resolved_path, texture);
    } else {
        success = LoadImageAsset(resolved_path, texture);
    }
    
    if (success) {
        loaded_textures_[asset_name] = std::move(texture);
        LogInfo("Successfully loaded asset: " + asset_name);
    } else {
        SetError("Failed to load asset data: " + asset_name);
    }
    
    return success;
}

bool HudAssetManager::LoadSVGAsset(const std::string& asset_path, HudTexture& texture) {
    LogInfo("Loading SVG asset: " + asset_path);
    
    // Create SVG rasterization options based on configuration
    SvgRasterizationOptions opts;
    opts.targetWidth = 1920;
    opts.targetHeight = 1080;
    opts.preserveAspectRatio = true;
    
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    
    if (!LoadSvgToRgba(asset_path, pixels, width, height, opts)) {
        LogError("Failed to rasterize SVG: " + asset_path);
        texture.gl_failed = true;
        return false;
    }
    
    texture.width = width;
    texture.height = height;
    
    // Create OpenGL texture
    if (!CreateOpenGLTexture(pixels, width, height, texture)) {
        LogError("Failed to create OpenGL texture for SVG: " + asset_path);
        texture.gl_failed = true;
        return false;
    }
    
    texture.gl_loaded = true;
    return true;
}

bool HudAssetManager::LoadImageAsset(const std::string& asset_path, HudTexture& texture) {
    // Placeholder for future image loading support
    LogWarning("Image asset loading not yet implemented: " + asset_path);
    return false;
}

bool HudAssetManager::ParseJsonConfiguration(const std::string& json_content) {
    using namespace simplejson;
    
    ParseResult result = Parse(json_content);
    if (!result.success) {
        SetError("JSON parse error: " + result.errorMessage + " at offset " + std::to_string(result.errorOffset));
        return false;
    }
    
    const JsonObject& root = result.value.AsObject();
    
    // Parse basic settings from hud_system_config
    if (root.find("hud_system_config") != root.end()) {
        const JsonObject& config = root.at("hud_system_config").AsObject();
        
        auto_discovery_enabled_ = config.find("auto_discovery") != config.end() ? 
                                 config.at("auto_discovery").AsBoolean(true) : true;
        
        cache_textures_ = config.find("cache_textures") != config.end() ?
                         config.at("cache_textures").AsBoolean(true) : true;
    }
    
    // Parse asset discovery settings
    if (root.find("asset_discovery") != root.end()) {
        const JsonObject& discovery = root.at("asset_discovery").AsObject();
        
        ui_graphics_path_ = discovery.find("ui_graphics_path") != discovery.end() ?
                           discovery.at("ui_graphics_path").AsString("assets/ui/graphics/") :
                           "assets/ui/graphics/";
        
        if (discovery.find("supported_formats") != discovery.end()) {
            const JsonArray& formats = discovery.at("supported_formats").AsArray();
            supported_formats_.clear();
            for (const auto& format : formats) {
                supported_formats_.push_back(format.AsString());
            }
        }
    }
    
    // Parse HUD assets
    if (root.find("hud_assets") != root.end()) {
        const JsonObject& assets = root.at("hud_assets").AsObject();
        
        for (const auto& [name, assetJson] : assets) {
            HudAssetConfig config;
            const JsonObject& assetObj = assetJson.AsObject();
            
            config.asset_path = assetObj.find("asset_path") != assetObj.end() ?
                               assetObj.at("asset_path").AsString() : "";
            
            config.type = assetObj.find("type") != assetObj.end() ?
                         assetObj.at("type").AsString("svg") : "svg";
            
            config.description = assetObj.find("description") != assetObj.end() ?
                                assetObj.at("description").AsString() : "";
            
            config.enabled = assetObj.find("enabled") != assetObj.end() ?
                            assetObj.at("enabled").AsBoolean(true) : true;
            
            config.layer = assetObj.find("layer") != assetObj.end() ?
                          static_cast<int>(assetObj.at("layer").AsNumber(1)) : 1;
            
            config.blend_mode = assetObj.find("blend_mode") != assetObj.end() ?
                               assetObj.at("blend_mode").AsString("blend") : "blend";
            
            // Parse target resolution
            if (assetObj.find("target_resolution") != assetObj.end()) {
                const JsonObject& resolution = assetObj.at("target_resolution").AsObject();
                config.target_resolution.width = static_cast<int>(resolution.at("width").AsNumber(1920));
                config.target_resolution.height = static_cast<int>(resolution.at("height").AsNumber(1080));
            }
            
            config.preserve_aspect_ratio = assetObj.find("preserve_aspect_ratio") != assetObj.end() ?
                                          assetObj.at("preserve_aspect_ratio").AsBoolean(true) : true;
            
            asset_configs_[name] = config;
            LogInfo("Parsed HUD asset config: " + name + " -> " + config.asset_path);
        }
    }
    
    // Parse layouts
    if (root.find("hud_layouts") != root.end()) {
        const JsonObject& layouts = root.at("hud_layouts").AsObject();
        
        for (const auto& [layoutName, layoutJson] : layouts) {
            HudLayout layout;
            const JsonObject& layoutObj = layoutJson.AsObject();
            
            layout.name = layoutObj.find("name") != layoutObj.end() ?
                         layoutObj.at("name").AsString() : layoutName;
            
            // Parse active HUDs
            if (layoutObj.find("active_huds") != layoutObj.end()) {
                const JsonArray& activeHuds = layoutObj.at("active_huds").AsArray();
                for (const auto& hudName : activeHuds) {
                    layout.active_huds.push_back(hudName.AsString());
                }
            }
            
            // Parse anchors
            if (layoutObj.find("anchors") != layoutObj.end()) {
                const JsonObject& anchors = layoutObj.at("anchors").AsObject();
                
                for (const auto& [hudName, anchorJson] : anchors) {
                    HudAnchor anchor;
                    const JsonObject& anchorObj = anchorJson.AsObject();
                    
                    anchor.x = static_cast<int>(anchorObj.at("x").AsNumber(0));
                    anchor.y = static_cast<int>(anchorObj.at("y").AsNumber(0));
                    anchor.anchor_type = anchorObj.find("anchor_type") != anchorObj.end() ?
                                        anchorObj.at("anchor_type").AsString("top_left") : "top_left";
                    
                    layout.anchors[hudName] = anchor;
                }
            }
            
            layouts_[layoutName] = layout;
            LogInfo("Parsed HUD layout: " + layoutName + " with " + std::to_string(layout.active_huds.size()) + " HUDs");
        }
    }
    
    return true;
}

std::vector<std::string> HudAssetManager::GetFilesInDirectory(const std::string& directory, 
                                                              const std::vector<std::string>& extensions) {
    return DiscoverAssetsInDirectory(directory);  // Reuse existing implementation
}

// Utility functions needed by the implementation
bool HudAssetManager::FileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

long HudAssetManager::GetFileModificationTime(const std::string& path) {
    try {
        auto ftime = std::filesystem::last_write_time(path);
        return std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

std::string HudAssetManager::ReadFileContents(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void HudAssetManager::CleanupTexture(HudTexture& texture) {
    if (texture.gl_loaded && texture.gl_texture_id != 0) {
#if defined(USE_GLFW) || defined(USE_SDL)
        glDeleteTextures(1, &texture.gl_texture_id);
#endif
        texture.gl_texture_id = 0;
        texture.gl_loaded = false;
    }
    
#ifdef USE_SDL
    if (texture.sdl_loaded && texture.sdl_texture != nullptr) {
        SDL_DestroyTexture(texture.sdl_texture);
        texture.sdl_texture = nullptr;
        texture.sdl_loaded = false;
    }
#endif
}

bool HudAssetManager::CreateOpenGLTexture(const std::vector<uint8_t>& pixel_data, int width, int height, HudTexture& texture) {
#if defined(USE_GLFW) || defined(USE_SDL)
    glGenTextures(1, &texture.gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, texture.gl_texture_id);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return true;
#else
    return false;
#endif
}

#ifdef USE_SDL
bool HudAssetManager::CreateSDLTexture(SDL_Surface* surface, SDL_Renderer* renderer, HudTexture& texture) {
    texture.sdl_texture = compat_CreateTextureFromSurface(renderer, surface);
    if (texture.sdl_texture) {
        texture.width = surface->w;
        texture.height = surface->h;
        texture.sdl_loaded = true;
        SDL_SetTextureBlendMode(texture.sdl_texture, SDL_BLENDMODE_BLEND);
        return true;
    }
    
    texture.sdl_failed = true;
    return false;
}
#endif

void HudAssetManager::SetError(const std::string& error) {
    last_error_ = error;
    if (!error.empty()) {
        LogError(error);
    }
}

void HudAssetManager::LogInfo(const std::string& message) {
    std::cout << "[HudAssetManager] INFO: " << message << std::endl;
}

void HudAssetManager::LogWarning(const std::string& message) {
    std::cout << "[HudAssetManager] WARNING: " << message << std::endl;
}

void HudAssetManager::LogError(const std::string& message) {
    std::cerr << "[HudAssetManager] ERROR: " << message << std::endl;
}

// Integration namespace implementation
namespace HudSystemIntegration {
    bool InitializeHudSystem() {
        auto& hud_manager = HudAssetManager::GetInstance();
        
        // Load default configuration
        if (!hud_manager.LoadConfiguration()) {
            std::cerr << "Failed to initialize HUD system: " << hud_manager.GetLastError() << std::endl;
            return false;
        }
        
        // Set default layout
        if (!hud_manager.SetActiveLayout("default")) {
            std::cerr << "Failed to set default HUD layout" << std::endl;
            return false;
        }
        
        std::cout << "HUD system initialized successfully" << std::endl;
        return true;
    }
    
    void ShutdownHudSystem() {
        auto& hud_manager = HudAssetManager::GetInstance();
        hud_manager.UnloadAllAssets();
        std::cout << "HUD system shutdown complete" << std::endl;
    }
    
    bool RefreshHudAssets() {
        auto& hud_manager = HudAssetManager::GetInstance();
        hud_manager.RefreshAssetCache();
        return true;
    }
    
    bool RenderHudLayer(const std::string& layer_name, int viewport_width, int viewport_height) {
        auto& hud_manager = HudAssetManager::GetInstance();
        
        // Get active HUDs for current layout
        std::vector<std::string> active_huds = hud_manager.GetActiveHudNames();
        
        for (const std::string& hud_name : active_huds) {
            if (!hud_manager.EnsureTextureLoaded(hud_name)) {
                continue;  // Skip failed assets
            }
            
            // Rendering logic would go here - this is a placeholder
            // In the actual implementation, this would integrate with
            // the existing Viewport3D rendering system
        }
        
        return true;
    }
    
    void ReportHudSystemStatus() {
        auto& hud_manager = HudAssetManager::GetInstance();
        std::cout << hud_manager.GetSystemStatus() << std::endl;
    }
}