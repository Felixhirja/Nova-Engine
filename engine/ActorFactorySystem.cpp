#include "ActorFactorySystem.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

ActorFactorySystem& ActorFactorySystem::GetInstance() {
    static ActorFactorySystem instance;
    return instance;
}

void ActorFactorySystem::RegisterFactory(const std::string& actorType, 
                                        FactoryFunction factory,
                                        const std::string& category,
                                        const std::vector<std::string>& dependencies) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (debugMode_) {
        std::cout << "[ActorFactory] Registering factory: " << actorType 
                  << " (category: " << category << ")" << std::endl;
    }
    
    factories_[actorType] = factory;
    
    FactoryMetadata metadata;
    metadata.actorType = actorType;
    metadata.category = category;
    metadata.dependencies = dependencies;
    metadata.lastUsed = std::chrono::steady_clock::now();
    metadata_[actorType] = metadata;
    
    // Validate on registration
    std::string errorMsg;
    if (!ValidateFactory(actorType, errorMsg)) {
        std::cerr << "[ActorFactory] Warning: Factory validation failed for " 
                  << actorType << ": " << errorMsg << std::endl;
    }
}

bool ActorFactorySystem::ValidateFactory(const std::string& actorType, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if factory exists
    auto factIt = factories_.find(actorType);
    if (factIt == factories_.end()) {
        errorMsg = "Factory not registered";
        return false;
    }
    
    // Check if metadata exists
    auto metaIt = metadata_.find(actorType);
    if (metaIt == metadata_.end()) {
        errorMsg = "Factory metadata missing";
        return false;
    }
    
    // Check dependencies
    if (!CheckDependencies(actorType, errorMsg)) {
        metadata_[actorType].isValid = false;
        metadata_[actorType].validationErrors = errorMsg;
        return false;
    }
    
    // Try creating test instance
    try {
        auto testActor = factIt->second();
        if (!testActor) {
            errorMsg = "Factory returned null actor";
            metadata_[actorType].isValid = false;
            metadata_[actorType].validationErrors = errorMsg;
            return false;
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Factory threw exception: ") + e.what();
        metadata_[actorType].isValid = false;
        metadata_[actorType].validationErrors = errorMsg;
        return false;
    }
    
    metadata_[actorType].isValid = true;
    metadata_[actorType].validationErrors.clear();
    return true;
}

bool ActorFactorySystem::ValidateAllFactories() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool allValid = true;
    for (const auto& [actorType, factory] : factories_) {
        std::string errorMsg;
        if (!ValidateFactory(actorType, errorMsg)) {
            std::cerr << "[ActorFactory] Validation failed for " << actorType 
                      << ": " << errorMsg << std::endl;
            allValid = false;
        }
    }
    
    return allValid;
}

ActorFactorySystem::FactoryResult ActorFactorySystem::CreateActor(
    const std::string& actorType,
    EntityManager& entityManager,
    Entity entity) {
    
    FactoryResult result;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if factory exists
    auto factIt = factories_.find(actorType);
    if (factIt == factories_.end()) {
        result.errorMessage = "Factory not registered for type: " + actorType;
        LogCreation(actorType, false, 0.0);
        return result;
    }
    
    // Check validation status
    auto metaIt = metadata_.find(actorType);
    if (metaIt != metadata_.end() && !metaIt->second.isValid) {
        result.errorMessage = "Factory validation failed: " + metaIt->second.validationErrors;
        LogCreation(actorType, false, 0.0);
        return result;
    }
    
    try {
        // Create actor
        result.actor = factIt->second();
        
        if (!result.actor) {
            result.errorMessage = "Factory returned null actor";
            LogCreation(actorType, false, 0.0);
            return result;
        }
        
        // Attach context
        ActorContext context(entityManager, entity);
        result.actor->AttachContext(std::move(context));
        
        // Initialize actor
        result.actor->Initialize();
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception during creation: ") + e.what();
        result.actor.reset();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.creationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // Update metrics
    UpdateMetrics(actorType, result.creationTimeMs);
    LogCreation(actorType, result.success, result.creationTimeMs);
    
    return result;
}

ActorFactorySystem::FactoryResult ActorFactorySystem::CreateFromTemplate(
    const std::string& templateName,
    EntityManager& entityManager,
    Entity entity) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find template
    auto tempIt = templates_.find(templateName);
    if (tempIt == templates_.end()) {
        FactoryResult result;
        result.errorMessage = "Template not found: " + templateName;
        return result;
    }
    
    // Update template usage
    tempIt->second.usageCount++;
    
    // Create from base type
    return CreateActor(tempIt->second.baseType, entityManager, entity);
}

void ActorFactorySystem::RegisterTemplate(const std::string& templateName,
                                         const std::string& baseType,
                                         const std::unordered_map<std::string, std::string>& parameters) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ActorTemplate templ;
    templ.templateName = templateName;
    templ.baseType = baseType;
    templ.parameters = parameters;
    templ.createdAt = std::chrono::steady_clock::now();
    
    templates_[templateName] = templ;
    
    if (debugMode_) {
        std::cout << "[ActorFactory] Registered template: " << templateName 
                  << " (base: " << baseType << ")" << std::endl;
    }
}

void ActorFactorySystem::EnableCaching(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    cachingEnabled_ = enable;
    
    if (debugMode_) {
        std::cout << "[ActorFactory] Caching " << (enable ? "enabled" : "disabled") << std::endl;
    }
}

void ActorFactorySystem::ClearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Currently no cache data to clear, but hook is here for future implementation
    
    if (debugMode_) {
        std::cout << "[ActorFactory] Cache cleared" << std::endl;
    }
}

size_t ActorFactorySystem::GetCacheSize() const {
    return 0; // No cache currently implemented
}

const ActorFactorySystem::FactoryMetadata& ActorFactorySystem::GetFactoryMetadata(
    const std::string& actorType) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    static FactoryMetadata emptyMetadata;
    auto it = metadata_.find(actorType);
    return (it != metadata_.end()) ? it->second : emptyMetadata;
}

ActorFactorySystem::PerformanceMetrics ActorFactorySystem::GetPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

std::vector<std::string> ActorFactorySystem::GetMostUsedActorTypes(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::pair<std::string, size_t>> usage;
    for (const auto& [type, meta] : metadata_) {
        usage.push_back({type, meta.creationCount});
    }
    
    std::sort(usage.begin(), usage.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> result;
    for (size_t i = 0; i < std::min(count, usage.size()); ++i) {
        result.push_back(usage[i].first);
    }
    
    return result;
}

void ActorFactorySystem::EnableDebugMode(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    debugMode_ = enable;
    std::cout << "[ActorFactory] Debug mode " << (enable ? "enabled" : "disabled") << std::endl;
}

void ActorFactorySystem::LogFactoryState(const std::string& actorType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto metaIt = metadata_.find(actorType);
    if (metaIt == metadata_.end()) {
        std::cout << "[ActorFactory] No metadata for " << actorType << std::endl;
        return;
    }
    
    const auto& meta = metaIt->second;
    std::cout << "\n=== Factory State: " << actorType << " ===" << std::endl;
    std::cout << "  Category: " << meta.category << std::endl;
    std::cout << "  Valid: " << (meta.isValid ? "Yes" : "No") << std::endl;
    std::cout << "  Creation Count: " << meta.creationCount << std::endl;
    std::cout << "  Avg Creation Time: " << std::fixed << std::setprecision(3) 
              << meta.avgCreationTime << " ms" << std::endl;
    std::cout << "  Dependencies: " << meta.dependencies.size() << std::endl;
    for (const auto& dep : meta.dependencies) {
        std::cout << "    - " << dep << std::endl;
    }
    if (!meta.validationErrors.empty()) {
        std::cout << "  Validation Errors: " << meta.validationErrors << std::endl;
    }
    std::cout << std::endl;
}

void ActorFactorySystem::LogAllFactories() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "\n=== All Registered Factories ===" << std::endl;
    std::cout << "Total: " << factories_.size() << std::endl;
    
    for (const auto& [actorType, meta] : metadata_) {
        std::cout << "  " << actorType << " (" << meta.category << "): " 
                  << meta.creationCount << " created, "
                  << std::fixed << std::setprecision(2) << meta.avgCreationTime << " ms avg"
                  << (meta.isValid ? "" : " [INVALID]") << std::endl;
    }
    std::cout << std::endl;
}

std::string ActorFactorySystem::GetFactoryHealthReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "=== Actor Factory Health Report ===\n";
    ss << "Total Factories: " << factories_.size() << "\n";
    ss << "Total Creations: " << metrics_.totalCreations << "\n";
    ss << "Avg Creation Time: " << std::fixed << std::setprecision(3) 
       << metrics_.avgTimeMs << " ms\n";
    ss << "Min/Max Time: " << metrics_.minTimeMs << " / " << metrics_.maxTimeMs << " ms\n";
    
    size_t validCount = 0;
    for (const auto& [type, meta] : metadata_) {
        if (meta.isValid) validCount++;
    }
    ss << "Valid Factories: " << validCount << "/" << metadata_.size() << "\n";
    
    if (validCount < metadata_.size()) {
        ss << "\nInvalid Factories:\n";
        for (const auto& [type, meta] : metadata_) {
            if (!meta.isValid) {
                ss << "  - " << type << ": " << meta.validationErrors << "\n";
            }
        }
    }
    
    return ss.str();
}

std::string ActorFactorySystem::GenerateDocumentation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "# Actor Factory System Documentation\n\n";
    ss << "Auto-generated documentation for all registered actor factories.\n\n";
    ss << "## Registered Actor Types\n\n";
    
    // Group by category
    std::unordered_map<std::string, std::vector<std::string>> byCategory;
    for (const auto& [type, meta] : metadata_) {
        byCategory[meta.category].push_back(type);
    }
    
    for (const auto& [category, types] : byCategory) {
        ss << "### Category: " << category << "\n\n";
        for (const auto& type : types) {
            const auto& meta = metadata_.at(type);
            ss << "#### " << type << "\n\n";
            ss << "- **Status**: " << (meta.isValid ? "Valid" : "Invalid") << "\n";
            ss << "- **Creation Count**: " << meta.creationCount << "\n";
            ss << "- **Avg Time**: " << std::fixed << std::setprecision(3) 
               << meta.avgCreationTime << " ms\n";
            
            if (!meta.dependencies.empty()) {
                ss << "- **Dependencies**: ";
                for (size_t i = 0; i < meta.dependencies.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << meta.dependencies[i];
                }
                ss << "\n";
            }
            
            if (!meta.validationErrors.empty()) {
                ss << "- **Validation Errors**: " << meta.validationErrors << "\n";
            }
            
            ss << "\n";
        }
    }
    
    // Templates section
    if (!templates_.empty()) {
        ss << "## Actor Templates\n\n";
        for (const auto& [name, templ] : templates_) {
            ss << "### " << name << "\n\n";
            ss << "- **Base Type**: " << templ.baseType << "\n";
            ss << "- **Usage Count**: " << templ.usageCount << "\n";
            ss << "- **Parameters**: " << templ.parameters.size() << "\n";
            ss << "\n";
        }
    }
    
    return ss.str();
}

void ActorFactorySystem::ExportDocumentation(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << GenerateDocumentation();
        file.close();
        std::cout << "[ActorFactory] Documentation exported to: " << filepath << std::endl;
    } else {
        std::cerr << "[ActorFactory] Failed to export documentation to: " << filepath << std::endl;
    }
}

bool ActorFactorySystem::TestFactory(const std::string& actorType, std::string& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Testing factory: " << actorType << "\n";
    
    auto factIt = factories_.find(actorType);
    if (factIt == factories_.end()) {
        ss << "  FAIL: Factory not registered\n";
        result = ss.str();
        return false;
    }
    
    try {
        auto actor = factIt->second();
        if (!actor) {
            ss << "  FAIL: Factory returned null\n";
            result = ss.str();
            return false;
        }
        
        ss << "  PASS: Actor created successfully\n";
        ss << "  Type: " << actor->GetName() << "\n";
        result = ss.str();
        return true;
        
    } catch (const std::exception& e) {
        ss << "  FAIL: Exception thrown: " << e.what() << "\n";
        result = ss.str();
        return false;
    }
}

std::vector<std::string> ActorFactorySystem::TestAllFactories() {
    std::vector<std::string> results;
    
    for (const auto& [actorType, factory] : factories_) {
        std::string result;
        TestFactory(actorType, result);
        results.push_back(result);
    }
    
    return results;
}

std::vector<std::string> ActorFactorySystem::GetRegisteredTypes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> types;
    for (const auto& [type, factory] : factories_) {
        types.push_back(type);
    }
    
    std::sort(types.begin(), types.end());
    return types;
}

std::vector<std::string> ActorFactorySystem::GetFactoriesByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> types;
    for (const auto& [type, meta] : metadata_) {
        if (meta.category == category) {
            types.push_back(type);
        }
    }
    
    return types;
}

bool ActorFactorySystem::HasFactory(const std::string& actorType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.find(actorType) != factories_.end();
}

void ActorFactorySystem::UpdateMetrics(const std::string& actorType, double creationTimeMs) {
    metrics_.totalCreations++;
    metrics_.totalTimeMs += creationTimeMs;
    metrics_.avgTimeMs = metrics_.totalTimeMs / metrics_.totalCreations;
    metrics_.minTimeMs = std::min(metrics_.minTimeMs, creationTimeMs);
    metrics_.maxTimeMs = std::max(metrics_.maxTimeMs, creationTimeMs);
    metrics_.creationsByType[actorType]++;
    
    auto& meta = metadata_[actorType];
    meta.creationCount++;
    meta.totalCreationTime += creationTimeMs;
    meta.avgCreationTime = meta.totalCreationTime / meta.creationCount;
    meta.lastUsed = std::chrono::steady_clock::now();
}

bool ActorFactorySystem::CheckDependencies(const std::string& actorType, std::string& errorMsg) {
    auto metaIt = metadata_.find(actorType);
    if (metaIt == metadata_.end()) {
        return true; // No dependencies to check
    }
    
    for (const auto& dep : metaIt->second.dependencies) {
        if (factories_.find(dep) == factories_.end()) {
            errorMsg = "Missing dependency: " + dep;
            return false;
        }
    }
    
    return true;
}

void ActorFactorySystem::LogCreation(const std::string& actorType, bool success, double timeMs) {
    if (debugMode_) {
        std::cout << "[ActorFactory] " << (success ? "SUCCESS" : "FAILED") 
                  << " creating " << actorType 
                  << " (" << std::fixed << std::setprecision(3) << timeMs << " ms)" 
                  << std::endl;
    }
}
