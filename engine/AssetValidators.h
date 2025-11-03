#pragma once

#include "AssetProcessingPipeline.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cstring>

namespace assets {

// Base texture validator
class TextureValidator : public IAssetValidator {
public:
    bool CanValidate(AssetType type) const override {
        return type == AssetType::Texture;
    }

    ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) override {
        ValidationResult result;
        
        std::cout << "[TextureValidator] Validating texture: " << filePath << std::endl;
        
        // Check file exists and has content
        if (!pipeline_utils::FileExists(filePath)) {
            result.isValid = false;
            result.errors.push_back("File does not exist");
            return result;
        }
        
        size_t fileSize = pipeline_utils::GetFileSize(filePath);
        if (fileSize == 0) {
            result.isValid = false;
            result.errors.push_back("File is empty");
            return result;
        }
        
        // Check file size limits
        const size_t MAX_TEXTURE_SIZE = 64 * 1024 * 1024; // 64MB
        if (fileSize > MAX_TEXTURE_SIZE) {
            result.isValid = false;
            result.errors.push_back("Texture file too large (max 64MB)");
            return result;
        }
        
        // Validate texture format by magic bytes
        if (!ValidateTextureFormat(filePath, metadata.format)) {
            result.isValid = false;
            result.errors.push_back("Invalid texture format or corrupted file");
            return result;
        }
        
        // Check texture dimensions (if we can read them)
        auto dimensions = GetTextureDimensions(filePath, metadata.format);
        if (dimensions.first > 0 && dimensions.second > 0) {
            result.properties["width"] = std::to_string(dimensions.first);
            result.properties["height"] = std::to_string(dimensions.second);
            
            // Validate power-of-two dimensions for certain platforms
            if (metadata.platform == PlatformTarget::Mobile) {
                if (!IsPowerOfTwo(dimensions.first) || !IsPowerOfTwo(dimensions.second)) {
                    result.warnings.push_back("Non-power-of-two textures may perform poorly on mobile");
                }
            }
            
            // Check maximum texture size for platform
            size_t maxDimension = GetMaxTextureSize(metadata.platform);
            if (dimensions.first > maxDimension || dimensions.second > maxDimension) {
                result.warnings.push_back("Texture dimensions exceed recommended maximum for platform");
            }
        }
        
        // Additional format-specific validation
        switch (metadata.format) {
            case AssetFormat::PNG:
                ValidatePNG(filePath, result);
                break;
            case AssetFormat::JPG:
                ValidateJPG(filePath, result);
                break;
            default:
                break;
        }
        
        result.properties["file_size"] = std::to_string(fileSize);
        result.properties["format"] = pipeline_utils::FormatToString(metadata.format);
        
        std::cout << "[TextureValidator] Validation " << (result.isValid ? "passed" : "failed") << std::endl;
        return result;
    }

    std::string GetValidatorName() const override {
        return "TextureValidator";
    }

private:
    bool ValidateTextureFormat(const std::string& filePath, AssetFormat format) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;
        
        char magic[8] = {0};
        file.read(magic, sizeof(magic));
        
        switch (format) {
            case AssetFormat::PNG:
                return magic[0] == '\x89' && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G';
            case AssetFormat::JPG:
                return magic[0] == '\xFF' && magic[1] == '\xD8';
            case AssetFormat::BMP:
                return magic[0] == 'B' && magic[1] == 'M';
            case AssetFormat::TGA:
                // TGA doesn't have a reliable magic number, just check extension
                return true;
            default:
                return false;
        }
    }
    
    std::pair<size_t, size_t> GetTextureDimensions(const std::string& filePath, AssetFormat format) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return {0, 0};
        
        switch (format) {
            case AssetFormat::PNG:
                return GetPNGDimensions(file);
            case AssetFormat::JPG:
                return GetJPGDimensions(file);
            case AssetFormat::BMP:
                return GetBMPDimensions(file);
            default:
                return {0, 0};
        }
    }
    
    std::pair<size_t, size_t> GetPNGDimensions(std::ifstream& file) {
        file.seekg(16); // Skip PNG signature and IHDR chunk type
        uint32_t width, height;
        file.read(reinterpret_cast<char*>(&width), 4);
        file.read(reinterpret_cast<char*>(&height), 4);
        
        // Convert from big-endian
        width = ((width & 0xFF000000) >> 24) | ((width & 0x00FF0000) >> 8) |
                ((width & 0x0000FF00) << 8) | ((width & 0x000000FF) << 24);
        height = ((height & 0xFF000000) >> 24) | ((height & 0x00FF0000) >> 8) |
                 ((height & 0x0000FF00) << 8) | ((height & 0x000000FF) << 24);
        
        return {width, height};
    }
    
    std::pair<size_t, size_t> GetJPGDimensions(std::ifstream& file) {
        // Simplified JPEG dimension reading
        // In production, would use proper JPEG library
        return {0, 0};
    }
    
    std::pair<size_t, size_t> GetBMPDimensions(std::ifstream& file) {
        file.seekg(18); // Skip BMP header to width/height
        uint32_t width, height;
        file.read(reinterpret_cast<char*>(&width), 4);
        file.read(reinterpret_cast<char*>(&height), 4);
        return {width, height};
    }
    
    bool IsPowerOfTwo(size_t value) {
        return value != 0 && (value & (value - 1)) == 0;
    }
    
    size_t GetMaxTextureSize(PlatformTarget platform) {
        switch (platform) {
            case PlatformTarget::Mobile: return 2048;
            case PlatformTarget::Console: return 4096;
            case PlatformTarget::Web: return 2048;
            default: return 8192;
        }
    }
    
    void ValidatePNG(const std::string& filePath, ValidationResult& result) {
        // Additional PNG-specific validation
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return;
        
        // Check for critical chunks
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0);
        
        if (fileSize > 1024 * 1024) { // 1MB
            result.warnings.push_back("Large PNG file - consider compression or format change");
        }
    }
    
    void ValidateJPG(const std::string& filePath, ValidationResult& result) {
        // Additional JPEG-specific validation
        result.properties["jpeg_validated"] = "true";
    }
};

// Model validator
class ModelValidator : public IAssetValidator {
public:
    bool CanValidate(AssetType type) const override {
        return type == AssetType::Model;
    }

    ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) override {
        ValidationResult result;
        
        std::cout << "[ModelValidator] Validating model: " << filePath << std::endl;
        
        if (!pipeline_utils::FileExists(filePath)) {
            result.isValid = false;
            result.errors.push_back("Model file does not exist");
            return result;
        }
        
        size_t fileSize = pipeline_utils::GetFileSize(filePath);
        if (fileSize == 0) {
            result.isValid = false;
            result.errors.push_back("Model file is empty");
            return result;
        }
        
        // Check file size limits
        const size_t MAX_MODEL_SIZE = 100 * 1024 * 1024; // 100MB
        if (fileSize > MAX_MODEL_SIZE) {
            result.warnings.push_back("Large model file - consider optimization");
        }
        
        // Format-specific validation
        switch (metadata.format) {
            case AssetFormat::OBJ:
                ValidateOBJ(filePath, result);
                break;
            case AssetFormat::GLTF:
                ValidateGLTF(filePath, result);
                break;
            default:
                result.properties["basic_validation"] = "true";
                break;
        }
        
        result.properties["file_size"] = std::to_string(fileSize);
        
        std::cout << "[ModelValidator] Model validation " << (result.isValid ? "passed" : "failed") << std::endl;
        return result;
    }

    std::string GetValidatorName() const override {
        return "ModelValidator";
    }

private:
    void ValidateOBJ(const std::string& filePath, ValidationResult& result) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.isValid = false;
            result.errors.push_back("Cannot open OBJ file");
            return;
        }
        
        std::string line;
        size_t vertexCount = 0;
        size_t faceCount = 0;
        bool hasMaterials = false;
        bool hasTexCoords = false;
        bool hasNormals = false;
        
        while (std::getline(file, line)) {
            if (line.length() < 2) continue;
            
            if (line.substr(0, 2) == "v ") {
                vertexCount++;
            } else if (line.substr(0, 2) == "f ") {
                faceCount++;
            } else if (line.substr(0, 6) == "mtllib") {
                hasMaterials = true;
            } else if (line.substr(0, 2) == "vt") {
                hasTexCoords = true;
            } else if (line.substr(0, 2) == "vn") {
                hasNormals = true;
            }
        }
        
        result.properties["vertex_count"] = std::to_string(vertexCount);
        result.properties["face_count"] = std::to_string(faceCount);
        result.properties["has_materials"] = hasMaterials ? "true" : "false";
        result.properties["has_texcoords"] = hasTexCoords ? "true" : "false";
        result.properties["has_normals"] = hasNormals ? "true" : "false";
        
        if (vertexCount == 0) {
            result.isValid = false;
            result.errors.push_back("OBJ file contains no vertices");
        } else if (vertexCount > 1000000) {
            result.warnings.push_back("High polygon count model - consider LOD generation");
        }
        
        if (!hasNormals) {
            result.warnings.push_back("Model has no normals - may need normal generation");
        }
    }
    
    void ValidateGLTF(const std::string& filePath, ValidationResult& result) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.isValid = false;
            result.errors.push_back("Cannot open GLTF file");
            return;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Basic JSON structure validation for GLTF
        if (content.find("\"asset\"") == std::string::npos) {
            result.isValid = false;
            result.errors.push_back("Invalid GLTF - missing asset information");
            return;
        }
        
        if (content.find("\"scene\"") != std::string::npos) {
            result.properties["has_scene"] = "true";
        }
        
        if (content.find("\"meshes\"") != std::string::npos) {
            result.properties["has_meshes"] = "true";
        }
        
        if (content.find("\"materials\"") != std::string::npos) {
            result.properties["has_materials"] = "true";
        }
        
        if (content.find("\"animations\"") != std::string::npos) {
            result.properties["has_animations"] = "true";
        }
    }
};

// Shader validator
class ShaderValidator : public IAssetValidator {
public:
    bool CanValidate(AssetType type) const override {
        return type == AssetType::Shader;
    }

    ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) override {
        ValidationResult result;
        
        std::cout << "[ShaderValidator] Validating shader: " << filePath << std::endl;
        
        if (!pipeline_utils::FileExists(filePath)) {
            result.isValid = false;
            result.errors.push_back("Shader file does not exist");
            return result;
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.isValid = false;
            result.errors.push_back("Cannot open shader file");
            return result;
        }
        
        std::string shaderSource((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Basic shader validation
        if (shaderSource.empty()) {
            result.isValid = false;
            result.errors.push_back("Shader file is empty");
            return result;
        }
        
        // Check for main function
        if (shaderSource.find("main") == std::string::npos) {
            result.isValid = false;
            result.errors.push_back("Shader missing main function");
            return result;
        }
        
        // GLSL-specific validation
        if (metadata.format == AssetFormat::GLSL) {
            ValidateGLSL(shaderSource, result);
        }
        
        // Check for common shader patterns
        if (shaderSource.find("uniform") != std::string::npos) {
            result.properties["has_uniforms"] = "true";
        }
        
        if (shaderSource.find("attribute") != std::string::npos || 
            shaderSource.find("in ") != std::string::npos) {
            result.properties["has_inputs"] = "true";
        }
        
        if (shaderSource.find("varying") != std::string::npos || 
            shaderSource.find("out ") != std::string::npos) {
            result.properties["has_outputs"] = "true";
        }
        
        result.properties["line_count"] = std::to_string(std::count(shaderSource.begin(), shaderSource.end(), '\n'));
        
        std::cout << "[ShaderValidator] Shader validation " << (result.isValid ? "passed" : "failed") << std::endl;
        return result;
    }

    std::string GetValidatorName() const override {
        return "ShaderValidator";
    }

private:
    void ValidateGLSL(const std::string& source, ValidationResult& result) {
        // Check for version directive
        if (source.find("#version") == std::string::npos) {
            result.warnings.push_back("GLSL shader missing version directive");
        }
        
        // Check for deprecated features
        if (source.find("attribute") != std::string::npos) {
            result.warnings.push_back("Using deprecated 'attribute' keyword - consider 'in'");
        }
        
        if (source.find("varying") != std::string::npos) {
            result.warnings.push_back("Using deprecated 'varying' keyword - consider 'in'/'out'");
        }
        
        // Basic syntax checks
        size_t braceOpen = std::count(source.begin(), source.end(), '{');
        size_t braceClose = std::count(source.begin(), source.end(), '}');
        
        if (braceOpen != braceClose) {
            result.isValid = false;
            result.errors.push_back("Mismatched braces in shader");
        }
    }
};

// Audio validator
class AudioValidator : public IAssetValidator {
public:
    bool CanValidate(AssetType type) const override {
        return type == AssetType::Audio;
    }

    ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) override {
        ValidationResult result;
        
        std::cout << "[AudioValidator] Validating audio: " << filePath << std::endl;
        
        if (!pipeline_utils::FileExists(filePath)) {
            result.isValid = false;
            result.errors.push_back("Audio file does not exist");
            return result;
        }
        
        size_t fileSize = pipeline_utils::GetFileSize(filePath);
        if (fileSize == 0) {
            result.isValid = false;
            result.errors.push_back("Audio file is empty");
            return result;
        }
        
        // Check file size limits
        const size_t MAX_AUDIO_SIZE = 50 * 1024 * 1024; // 50MB
        if (fileSize > MAX_AUDIO_SIZE) {
            result.warnings.push_back("Large audio file - consider compression");
        }
        
        // Format-specific validation
        switch (metadata.format) {
            case AssetFormat::WAV:
                ValidateWAV(filePath, result);
                break;
            case AssetFormat::MP3:
                ValidateMP3(filePath, result);
                break;
            case AssetFormat::OGG:
                ValidateOGG(filePath, result);
                break;
            default:
                break;
        }
        
        result.properties["file_size"] = std::to_string(fileSize);
        
        std::cout << "[AudioValidator] Audio validation " << (result.isValid ? "passed" : "failed") << std::endl;
        return result;
    }

    std::string GetValidatorName() const override {
        return "AudioValidator";
    }

private:
    void ValidateWAV(const std::string& filePath, ValidationResult& result) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return;
        
        char header[44];
        file.read(header, 44);
        
        // Check RIFF header
        if (strncmp(header, "RIFF", 4) != 0) {
            result.isValid = false;
            result.errors.push_back("Invalid WAV file - missing RIFF header");
            return;
        }
        
        // Check WAVE format
        if (strncmp(header + 8, "WAVE", 4) != 0) {
            result.isValid = false;
            result.errors.push_back("Invalid WAV file - not WAVE format");
            return;
        }
        
        result.properties["audio_format"] = "WAV";
        result.properties["validated"] = "true";
    }
    
    void ValidateMP3(const std::string& filePath, ValidationResult& result) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return;
        
        char header[3];
        file.read(header, 3);
        
        // Check for ID3 tag or MP3 frame sync
        if (strncmp(header, "ID3", 3) == 0 || 
            (header[0] == '\xFF' && (header[1] & 0xE0) == 0xE0)) {
            result.properties["audio_format"] = "MP3";
            result.properties["validated"] = "true";
        } else {
            result.warnings.push_back("Possibly invalid MP3 file format");
        }
    }
    
    void ValidateOGG(const std::string& filePath, ValidationResult& result) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return;
        
        char header[4];
        file.read(header, 4);
        
        // Check OGG magic number
        if (strncmp(header, "OggS", 4) == 0) {
            result.properties["audio_format"] = "OGG";
            result.properties["validated"] = "true";
        } else {
            result.isValid = false;
            result.errors.push_back("Invalid OGG file format");
        }
    }
};

// Config validator
class ConfigValidator : public IAssetValidator {
public:
    bool CanValidate(AssetType type) const override {
        return type == AssetType::Config;
    }

    ValidationResult Validate(const AssetMetadata& metadata, const std::string& filePath) override {
        ValidationResult result;
        
        std::cout << "[ConfigValidator] Validating config: " << filePath << std::endl;
        
        if (!pipeline_utils::FileExists(filePath)) {
            result.isValid = false;
            result.errors.push_back("Config file does not exist");
            return result;
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.isValid = false;
            result.errors.push_back("Cannot open config file");
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        if (content.empty()) {
            result.isValid = false;
            result.errors.push_back("Config file is empty");
            return result;
        }
        
        // Format-specific validation
        switch (metadata.format) {
            case AssetFormat::JSON:
                ValidateJSON(content, result);
                break;
            case AssetFormat::XML:
                ValidateXML(content, result);
                break;
            case AssetFormat::YAML:
                ValidateYAML(content, result);
                break;
            case AssetFormat::INI:
                ValidateINI(content, result);
                break;
            default:
                break;
        }
        
        result.properties["line_count"] = std::to_string(std::count(content.begin(), content.end(), '\n'));
        result.properties["char_count"] = std::to_string(content.length());
        
        std::cout << "[ConfigValidator] Config validation " << (result.isValid ? "passed" : "failed") << std::endl;
        return result;
    }

    std::string GetValidatorName() const override {
        return "ConfigValidator";
    }

private:
    void ValidateJSON(const std::string& content, ValidationResult& result) {
        // Basic JSON validation
        size_t braceOpen = std::count(content.begin(), content.end(), '{');
        size_t braceClose = std::count(content.begin(), content.end(), '}');
        size_t bracketOpen = std::count(content.begin(), content.end(), '[');
        size_t bracketClose = std::count(content.begin(), content.end(), ']');
        
        if (braceOpen != braceClose) {
            result.isValid = false;
            result.errors.push_back("Mismatched curly braces in JSON");
            return;
        }
        
        if (bracketOpen != bracketClose) {
            result.isValid = false;
            result.errors.push_back("Mismatched square brackets in JSON");
            return;
        }
        
        result.properties["json_objects"] = std::to_string(braceOpen);
        result.properties["json_arrays"] = std::to_string(bracketOpen);
    }
    
    void ValidateXML(const std::string& content, ValidationResult& result) {
        // Basic XML validation
        if (content.find("<?xml") == std::string::npos) {
            result.warnings.push_back("XML missing declaration");
        }
        
        // Simple tag matching
        std::regex tagRegex("<([^/][^>]*)>");
        std::regex closeTagRegex("</([^>]+)>");
        
        auto tagsBegin = std::sregex_iterator(content.begin(), content.end(), tagRegex);
        auto tagsEnd = std::sregex_iterator();
        
        result.properties["xml_tags"] = std::to_string(std::distance(tagsBegin, tagsEnd));
    }
    
    void ValidateYAML(const std::string& content, ValidationResult& result) {
        // Basic YAML validation
        if (content.find(":") == std::string::npos) {
            result.warnings.push_back("YAML file may be missing key-value pairs");
        }
        
        size_t colonCount = std::count(content.begin(), content.end(), ':');
        result.properties["yaml_pairs"] = std::to_string(colonCount);
    }
    
    void ValidateINI(const std::string& content, ValidationResult& result) {
        // Basic INI validation
        size_t sectionCount = 0;
        size_t keyCount = 0;
        
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            line = pipeline_utils::Trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            
            if (line[0] == '[' && line.back() == ']') {
                sectionCount++;
            } else if (line.find('=') != std::string::npos) {
                keyCount++;
            }
        }
        
        result.properties["ini_sections"] = std::to_string(sectionCount);
        result.properties["ini_keys"] = std::to_string(keyCount);
        
        if (sectionCount == 0) {
            result.warnings.push_back("INI file has no sections");
        }
    }
};

// Factory for creating validators
class ValidatorFactory {
public:
    static std::vector<std::unique_ptr<IAssetValidator>> CreateAllValidators() {
        std::vector<std::unique_ptr<IAssetValidator>> validators;
        
        validators.push_back(std::make_unique<TextureValidator>());
        validators.push_back(std::make_unique<ModelValidator>());
        validators.push_back(std::make_unique<ShaderValidator>());
        validators.push_back(std::make_unique<AudioValidator>());
        validators.push_back(std::make_unique<ConfigValidator>());
        
        return validators;
    }
};

} // namespace assets