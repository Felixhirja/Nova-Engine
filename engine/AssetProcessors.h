#pragma once

#include "AssetProcessingPipeline.h"
#include <iostream>
#include <fstream>

namespace assets {

// Base texture processor
class TextureProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::PNG || format == AssetFormat::JPG || 
               format == AssetFormat::TGA || format == AssetFormat::BMP;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[TextureProcessor] Processing texture: " << inputPath << std::endl;
        
        // For now, just copy the file (in production would do actual texture processing)
        std::ifstream src(inputPath, std::ios::binary);
        std::ofstream dst(outputPath, std::ios::binary);
        
        if (!src.is_open() || !dst.is_open()) {
            std::cout << "[TextureProcessor] Failed to open files for processing" << std::endl;
            return false;
        }
        
        dst << src.rdbuf();
        
        // Add texture-specific metadata
        metadata.properties["texture_processed"] = "true";
        metadata.properties["texture_format"] = pipeline_utils::FormatToString(metadata.format);
        
        // Platform-specific processing
        switch (metadata.platform) {
            case PlatformTarget::Mobile:
                metadata.properties["texture_compression"] = "ETC2";
                metadata.properties["max_size"] = "1024";
                break;
            case PlatformTarget::Console:
                metadata.properties["texture_compression"] = "BC7";
                metadata.properties["max_size"] = "2048";
                break;
            case PlatformTarget::Web:
                metadata.properties["texture_compression"] = "ASTC";
                metadata.properties["max_size"] = "512";
                break;
            default:
                metadata.properties["texture_compression"] = "none";
                metadata.properties["max_size"] = "4096";
                break;
        }
        
        std::cout << "[TextureProcessor] Successfully processed texture" << std::endl;
        return true;
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::PNG, AssetFormat::JPG, AssetFormat::TGA, AssetFormat::BMP};
    }

    std::string GetProcessorName() const override {
        return "TextureProcessor";
    }
};

// Model processor
class ModelProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::OBJ || format == AssetFormat::FBX || 
               format == AssetFormat::GLTF || format == AssetFormat::DAE;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[ModelProcessor] Processing model: " << inputPath << std::endl;
        
        // Copy file (placeholder for actual model processing)
        std::ifstream src(inputPath, std::ios::binary);
        std::ofstream dst(outputPath, std::ios::binary);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        dst << src.rdbuf();
        
        // Add model-specific metadata
        metadata.properties["model_processed"] = "true";
        metadata.properties["model_format"] = pipeline_utils::FormatToString(metadata.format);
        
        // Quality-based processing
        switch (metadata.quality) {
            case QualityLevel::Low:
                metadata.properties["polygon_reduction"] = "75%";
                metadata.properties["texture_resolution"] = "256";
                break;
            case QualityLevel::Medium:
                metadata.properties["polygon_reduction"] = "50%";
                metadata.properties["texture_resolution"] = "512";
                break;
            case QualityLevel::High:
                metadata.properties["polygon_reduction"] = "25%";
                metadata.properties["texture_resolution"] = "1024";
                break;
            case QualityLevel::Ultra:
                metadata.properties["polygon_reduction"] = "0%";
                metadata.properties["texture_resolution"] = "2048";
                break;
        }
        
        std::cout << "[ModelProcessor] Successfully processed model" << std::endl;
        return true;
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::OBJ, AssetFormat::FBX, AssetFormat::GLTF, AssetFormat::DAE};
    }

    std::string GetProcessorName() const override {
        return "ModelProcessor";
    }
};

// Shader processor
class ShaderProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::GLSL || format == AssetFormat::HLSL;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[ShaderProcessor] Processing shader: " << inputPath << std::endl;
        
        std::ifstream src(inputPath);
        std::ofstream dst(outputPath);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        // Read shader source
        std::string shaderSource((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
        
        // Add shader validation and preprocessing
        if (ValidateShaderSyntax(shaderSource)) {
            metadata.properties["shader_valid"] = "true";
            
            // Platform-specific shader compilation
            std::string processedSource = PreprocessShader(shaderSource, metadata.platform);
            dst << processedSource;
            
            metadata.properties["shader_processed"] = "true";
            metadata.properties["shader_platform"] = pipeline_utils::GetPlatformString(metadata.platform);
            
            std::cout << "[ShaderProcessor] Successfully processed shader" << std::endl;
            return true;
        } else {
            metadata.properties["shader_valid"] = "false";
            std::cout << "[ShaderProcessor] Shader validation failed" << std::endl;
            return false;
        }
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::GLSL, AssetFormat::HLSL};
    }

    std::string GetProcessorName() const override {
        return "ShaderProcessor";
    }

private:
    bool ValidateShaderSyntax(const std::string& source) {
        // Basic shader validation (check for required keywords)
        return source.find("main") != std::string::npos;
    }
    
    std::string PreprocessShader(const std::string& source, PlatformTarget platform) {
        std::string processed = source;
        
        // Add platform-specific preprocessing
        switch (platform) {
            case PlatformTarget::Mobile:
                processed = "#define MOBILE_TARGET\n" + processed;
                break;
            case PlatformTarget::Console:
                processed = "#define CONSOLE_TARGET\n" + processed;
                break;
            case PlatformTarget::Web:
                processed = "#define WEB_TARGET\n" + processed;
                break;
            default:
                processed = "#define DESKTOP_TARGET\n" + processed;
                break;
        }
        
        return processed;
    }
};

// Audio processor
class AudioProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::WAV || format == AssetFormat::MP3 || format == AssetFormat::OGG;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[AudioProcessor] Processing audio: " << inputPath << std::endl;
        
        std::ifstream src(inputPath, std::ios::binary);
        std::ofstream dst(outputPath, std::ios::binary);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        dst << src.rdbuf();
        
        // Add audio-specific metadata
        metadata.properties["audio_processed"] = "true";
        metadata.properties["audio_format"] = pipeline_utils::FormatToString(metadata.format);
        
        // Quality-based audio processing
        switch (metadata.quality) {
            case QualityLevel::Low:
                metadata.properties["sample_rate"] = "22050";
                metadata.properties["bit_rate"] = "96kbps";
                break;
            case QualityLevel::Medium:
                metadata.properties["sample_rate"] = "44100";
                metadata.properties["bit_rate"] = "128kbps";
                break;
            case QualityLevel::High:
                metadata.properties["sample_rate"] = "48000";
                metadata.properties["bit_rate"] = "256kbps";
                break;
            case QualityLevel::Ultra:
                metadata.properties["sample_rate"] = "96000";
                metadata.properties["bit_rate"] = "320kbps";
                break;
        }
        
        std::cout << "[AudioProcessor] Successfully processed audio" << std::endl;
        return true;
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::WAV, AssetFormat::MP3, AssetFormat::OGG};
    }

    std::string GetProcessorName() const override {
        return "AudioProcessor";
    }
};

// Font processor
class FontProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::TTF || format == AssetFormat::OTF;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[FontProcessor] Processing font: " << inputPath << std::endl;
        
        std::ifstream src(inputPath, std::ios::binary);
        std::ofstream dst(outputPath, std::ios::binary);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        dst << src.rdbuf();
        
        // Add font-specific metadata
        metadata.properties["font_processed"] = "true";
        metadata.properties["font_format"] = pipeline_utils::FormatToString(metadata.format);
        
        // Platform-specific font processing
        switch (metadata.platform) {
            case PlatformTarget::Mobile:
                metadata.properties["font_sizes"] = "12,14,16,18";
                metadata.properties["subset"] = "latin";
                break;
            case PlatformTarget::Web:
                metadata.properties["font_sizes"] = "10,12,14,16,18,20";
                metadata.properties["subset"] = "latin,latin-ext";
                break;
            default:
                metadata.properties["font_sizes"] = "8,10,12,14,16,18,20,24,32";
                metadata.properties["subset"] = "full";
                break;
        }
        
        std::cout << "[FontProcessor] Successfully processed font" << std::endl;
        return true;
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::TTF, AssetFormat::OTF};
    }

    std::string GetProcessorName() const override {
        return "FontProcessor";
    }
};

// Config processor
class ConfigProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::JSON || format == AssetFormat::XML || 
               format == AssetFormat::YAML || format == AssetFormat::INI;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[ConfigProcessor] Processing config: " << inputPath << std::endl;
        
        std::ifstream src(inputPath);
        std::ofstream dst(outputPath);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
        
        // Validate config format
        if (ValidateConfigFormat(content, metadata.format)) {
            // Process config (minify, validate, etc.)
            std::string processedContent = ProcessConfigContent(content, metadata.platform);
            dst << processedContent;
            
            metadata.properties["config_processed"] = "true";
            metadata.properties["config_format"] = pipeline_utils::FormatToString(metadata.format);
            metadata.properties["config_valid"] = "true";
            
            std::cout << "[ConfigProcessor] Successfully processed config" << std::endl;
            return true;
        } else {
            metadata.properties["config_valid"] = "false";
            std::cout << "[ConfigProcessor] Config validation failed" << std::endl;
            return false;
        }
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::JSON, AssetFormat::XML, AssetFormat::YAML, AssetFormat::INI};
    }

    std::string GetProcessorName() const override {
        return "ConfigProcessor";
    }

private:
    bool ValidateConfigFormat(const std::string& content, AssetFormat format) {
        switch (format) {
            case AssetFormat::JSON:
                return content.find("{") != std::string::npos && content.find("}") != std::string::npos;
            case AssetFormat::XML:
                return content.find("<") != std::string::npos && content.find(">") != std::string::npos;
            case AssetFormat::YAML:
                return content.find(":") != std::string::npos;
            case AssetFormat::INI:
                return content.find("[") != std::string::npos && content.find("]") != std::string::npos;
            default:
                return false;
        }
    }
    
    std::string ProcessConfigContent(const std::string& content, PlatformTarget platform) {
        // For now, just return the content as-is
        // In production, would minify, add platform-specific settings, etc.
        return content;
    }
};

// SVG/Sprite processor
class SpriteProcessor : public IAssetProcessor {
public:
    bool CanProcess(AssetFormat format) const override {
        return format == AssetFormat::SVG;
    }

    bool Process(AssetMetadata& metadata, const std::string& inputPath, const std::string& outputPath) override {
        std::cout << "[SpriteProcessor] Processing sprite: " << inputPath << std::endl;
        
        std::ifstream src(inputPath);
        std::ofstream dst(outputPath);
        
        if (!src.is_open() || !dst.is_open()) {
            return false;
        }
        
        std::string svgContent((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
        
        // Process SVG (optimize, rasterize for different sizes, etc.)
        std::string processedSvg = OptimizeSVG(svgContent);
        dst << processedSvg;
        
        metadata.properties["sprite_processed"] = "true";
        metadata.properties["sprite_format"] = "SVG";
        
        // Generate rasterized versions for different quality levels
        switch (metadata.quality) {
            case QualityLevel::Low:
                metadata.properties["raster_sizes"] = "16,32";
                break;
            case QualityLevel::Medium:
                metadata.properties["raster_sizes"] = "16,32,64";
                break;
            case QualityLevel::High:
                metadata.properties["raster_sizes"] = "16,32,64,128";
                break;
            case QualityLevel::Ultra:
                metadata.properties["raster_sizes"] = "16,32,64,128,256";
                break;
        }
        
        std::cout << "[SpriteProcessor] Successfully processed sprite" << std::endl;
        return true;
    }

    std::vector<AssetFormat> GetSupportedFormats() const override {
        return {AssetFormat::SVG};
    }

    std::string GetProcessorName() const override {
        return "SpriteProcessor";
    }

private:
    std::string OptimizeSVG(const std::string& svgContent) {
        // Basic SVG optimization (remove comments, unnecessary whitespace)
        std::string optimized = svgContent;
        
        // Remove comments
        size_t commentStart = optimized.find("<!--");
        while (commentStart != std::string::npos) {
            size_t commentEnd = optimized.find("-->", commentStart);
            if (commentEnd != std::string::npos) {
                optimized.erase(commentStart, commentEnd - commentStart + 3);
            }
            commentStart = optimized.find("<!--", commentStart);
        }
        
        return optimized;
    }
};

// Factory for creating processors
class ProcessorFactory {
public:
    static std::vector<std::unique_ptr<IAssetProcessor>> CreateAllProcessors() {
        std::vector<std::unique_ptr<IAssetProcessor>> processors;
        
        processors.push_back(std::make_unique<TextureProcessor>());
        processors.push_back(std::make_unique<ModelProcessor>());
        processors.push_back(std::make_unique<ShaderProcessor>());
        processors.push_back(std::make_unique<AudioProcessor>());
        processors.push_back(std::make_unique<FontProcessor>());
        processors.push_back(std::make_unique<ConfigProcessor>());
        processors.push_back(std::make_unique<SpriteProcessor>());
        
        return processors;
    }
};

} // namespace assets