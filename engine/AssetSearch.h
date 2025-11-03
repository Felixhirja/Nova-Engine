#pragma once

#include "AssetPipeline.h"
#include <string>
#include <vector>
#include <functional>
#include <regex>

namespace AssetPipeline {

/**
 * Asset Search System
 * Advanced search and filtering for asset discovery
 */

enum class SearchMode {
    Exact,        // Exact match
    Contains,     // Contains substring
    StartsWith,   // Starts with prefix
    EndsWith,     // Ends with suffix
    Regex,        // Regular expression
    Fuzzy         // Fuzzy matching
};

struct SearchCriteria {
    std::string query;
    SearchMode mode = SearchMode::Contains;
    
    // Type filters
    std::vector<AssetType> types;
    
    // State filters
    std::vector<AssetState> states;
    
    // Size filters
    size_t min_size = 0;
    size_t max_size = SIZE_MAX;
    
    // Date filters
    std::chrono::system_clock::time_point modified_after;
    std::chrono::system_clock::time_point modified_before;
    
    // Tag filters
    std::unordered_map<std::string, std::string> required_tags;
    
    // Platform filter
    Platform platform = Platform::All;
    
    // Priority filter
    int min_priority = INT_MIN;
    int max_priority = INT_MAX;
    
    // Dependency filters
    bool has_dependencies = false;
    bool has_dependents = false;
    
    // Result options
    size_t max_results = 1000;
    bool sort_by_relevance = true;
};

struct SearchResult {
    AssetMetadata metadata;
    float relevance_score = 0.0f;
    std::vector<std::string> match_highlights;
};

class AssetSearch {
public:
    static AssetSearch& GetInstance();

    // Basic search
    std::vector<SearchResult> Search(const std::string& query);
    std::vector<SearchResult> SearchWithCriteria(const SearchCriteria& criteria);
    
    // Quick filters
    std::vector<AssetMetadata> FindByName(const std::string& name);
    std::vector<AssetMetadata> FindByPath(const std::string& path_pattern);
    std::vector<AssetMetadata> FindByExtension(const std::string& extension);
    std::vector<AssetMetadata> FindByType(AssetType type);
    std::vector<AssetMetadata> FindByTag(const std::string& tag_key, const std::string& tag_value);
    
    // Advanced filters
    std::vector<AssetMetadata> FindLargeAssets(size_t min_size_mb);
    std::vector<AssetMetadata> FindUnusedAssets(const std::chrono::system_clock::time_point& since);
    std::vector<AssetMetadata> FindRecentlyModified(const std::chrono::hours& within);
    std::vector<AssetMetadata> FindByDependency(const std::string& dependency_path);
    std::vector<AssetMetadata> FindOrphans(); // Assets with no dependents
    
    // Full-text search
    std::vector<SearchResult> FullTextSearch(const std::string& text);
    
    // Fuzzy search
    std::vector<SearchResult> FuzzySearch(const std::string& query, float min_similarity = 0.7f);
    
    // Search history
    void AddToSearchHistory(const std::string& query);
    std::vector<std::string> GetSearchHistory(size_t count = 10);
    void ClearSearchHistory();
    
    // Saved searches
    void SaveSearch(const std::string& name, const SearchCriteria& criteria);
    std::optional<SearchCriteria> LoadSavedSearch(const std::string& name);
    std::vector<std::string> GetSavedSearches();
    void DeleteSavedSearch(const std::string& name);

private:
    AssetSearch() = default;
    
    float CalculateRelevance(const AssetMetadata& metadata, const SearchCriteria& criteria);
    float FuzzyMatch(const std::string& a, const std::string& b);
    bool MatchesCriteria(const AssetMetadata& metadata, const SearchCriteria& criteria);
    
    std::vector<std::string> search_history_;
    std::unordered_map<std::string, SearchCriteria> saved_searches_;
    mutable std::mutex mutex_;
};

} // namespace AssetPipeline
