#pragma once

#include "../SimpleJson.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace ContentManagement {

/**
 * ContentLocalization: Multi-language content management system
 * 
 * Features:
 * - Multi-language support for all content
 * - Translation workflow management
 * - Missing translation detection
 * - Locale-specific content variants
 * - Pluralization and gender support
 * - RTL (Right-to-Left) language support
 * - String interpolation and formatting
 */
class ContentLocalization {
public:
    enum class LocaleFormat {
        ISO_639_1,      // Two-letter code (en, fr, de)
        ISO_639_3,      // Three-letter code (eng, fra, deu)
        BCP_47          // Language tag (en-US, fr-FR, zh-CN)
    };

    struct LocaleInfo {
        std::string code;           // e.g., "en-US"
        std::string language;       // e.g., "English"
        std::string nativeName;     // e.g., "English"
        std::string region;         // e.g., "United States"
        bool isRTL;                 // Right-to-left language
        std::string pluralRules;    // CLDR plural rules
        std::string dateFormat;
        std::string numberFormat;
    };

    struct TranslationEntry {
        std::string key;
        std::unordered_map<std::string, std::string> translations;  // locale -> text
        std::string context;        // Context for translators
        std::string comment;        // Developer comment
        int maxLength;              // Character limit (UI constraints)
        std::vector<std::string> placeholders;  // {0}, {name}, etc.
        bool needsReview;
        std::chrono::system_clock::time_point lastModified;
    };

    struct LocalizationStats {
        int totalStrings;
        int translatedStrings;
        int missingTranslations;
        int outdatedTranslations;
        double completionPercentage;
        std::vector<std::string> missingLocales;
    };

    ContentLocalization();
    ~ContentLocalization();

    // Initialization
    bool Initialize(const std::string& localizationDirectory);
    bool LoadLocale(const std::string& localeCode);
    bool LoadAllLocales();
    
    // Locale Management
    void RegisterLocale(const LocaleInfo& locale);
    void SetCurrentLocale(const std::string& localeCode);
    std::string GetCurrentLocale() const;
    std::vector<std::string> GetAvailableLocales() const;
    const LocaleInfo* GetLocaleInfo(const std::string& localeCode) const;
    
    // Translation
    std::string Translate(const std::string& key, const std::string& localeCode = "") const;
    std::string TranslateWithPlaceholders(const std::string& key, const std::unordered_map<std::string, std::string>& placeholders, const std::string& localeCode = "") const;
    std::string TranslatePlural(const std::string& key, int count, const std::string& localeCode = "") const;
    
    // Content Localization
    bool LocalizeContent(simplejson::JsonObject& content, const std::string& targetLocale);
    bool LocalizeField(simplejson::JsonValue& field, const std::string& fieldName, const std::string& targetLocale);
    std::unique_ptr<simplejson::JsonObject> GetLocalizedContent(const std::string& contentId, const std::string& localeCode) const;
    
    // Translation Management
    void AddTranslation(const std::string& key, const std::string& localeCode, const std::string& text);
    void UpdateTranslation(const std::string& key, const std::string& localeCode, const std::string& text);
    void RemoveTranslation(const std::string& key);
    bool HasTranslation(const std::string& key, const std::string& localeCode) const;
    
    const TranslationEntry* GetTranslationEntry(const std::string& key) const;
    std::vector<std::string> GetAllTranslationKeys() const;
    
    // Translation Workflow
    std::vector<std::string> GetMissingTranslations(const std::string& localeCode) const;
    std::vector<std::string> GetOutdatedTranslations(const std::string& localeCode) const;
    void MarkForReview(const std::string& key, bool needsReview = true);
    
    // Export for Translators
    bool ExportForTranslation(const std::string& localeCode, const std::string& outputPath, const std::string& format = "xliff");
    bool ImportTranslations(const std::string& filePath, const std::string& format = "xliff");
    
    // Statistics
    LocalizationStats GetStats(const std::string& localeCode) const;
    std::string GenerateProgressReport() const;
    
    // String Extraction
    std::vector<std::string> ExtractStringsFromContent(const simplejson::JsonObject& content);
    void ScanContentForTranslations(const std::string& contentDirectory);
    
    // Validation
    bool ValidateTranslations(const std::string& localeCode, std::vector<std::string>& errors) const;
    bool CheckPlaceholderConsistency(const std::string& key, std::vector<std::string>& errors) const;
    bool CheckLengthConstraints(const std::string& key, const std::string& localeCode, std::vector<std::string>& errors) const;
    
    // Fallback
    void SetFallbackLocale(const std::string& localeCode);
    std::string GetFallbackLocale() const;
    
    // UI Integration
    void RenderLocalizationEditor();
    void RenderTranslationBrowser();
    void RenderLocaleSelector();
    
private:
    std::string GetTranslationInternal(const std::string& key, const std::string& localeCode) const;
    std::string ApplyPluralRules(const std::string& text, int count, const std::string& localeCode) const;
    std::string FormatWithPlaceholders(const std::string& text, const std::unordered_map<std::string, std::string>& placeholders) const;
    
    std::vector<std::string> ExtractPlaceholders(const std::string& text) const;
    bool IsLocalizable(const simplejson::JsonValue& value) const;
    
    std::unordered_map<std::string, LocaleInfo> locales_;
    std::unordered_map<std::string, TranslationEntry> translations_;
    std::string currentLocale_;
    std::string fallbackLocale_;
    std::string localizationDirectory_;
};

} // namespace ContentManagement
