#include "ContentCompositor.h"
#include <algorithm>

namespace NovaEngine {

// ContentCompositor implementation
void ContentCompositor::RegisterRule(const std::string& contentType, 
                                     const FieldCompositionRule& rule) {
    rules_[contentType].push_back(rule);
}

void ContentCompositor::ClearRules(const std::string& contentType) {
    rules_.erase(contentType);
}

std::unique_ptr<ContentDefinition> ContentCompositor::Compose(
    const std::vector<const ContentDefinition*>& bases,
    const std::string& newId) const {
    
    if (bases.empty()) {
        return nullptr;
    }
    
    // All bases must be same type
    const std::string& type = bases[0]->GetType();
    for (size_t i = 1; i < bases.size(); ++i) {
        if (bases[i]->GetType() != type) {
            return nullptr;
        }
    }
    
    // Convert to JSON
    std::vector<simplejson::JsonValue> jsons;
    for (const auto* base : bases) {
        jsons.push_back(base->ToJson());
    }
    
    // Compose
    simplejson::JsonValue composed = ComposeJson(jsons, type);
    composed.Set("id", newId);
    
    // Create new content from composed JSON
    auto result = ContentFactory::Instance().Create(type, newId);
    if (result && result->FromJson(composed)) {
        return result;
    }
    
    return nullptr;
}

std::unique_ptr<ContentDefinition> ContentCompositor::ComposeWithRules(
    const std::vector<const ContentDefinition*>& bases,
    const std::string& newId,
    const std::vector<FieldCompositionRule>& rules) const {
    
    if (bases.empty()) {
        return nullptr;
    }
    
    // Temporarily override rules
    const std::string& type = bases[0]->GetType();
    
    // Store old rules temporarily
    auto it = rules_.find(type);
    std::vector<FieldCompositionRule> oldRules;
    if (it != rules_.end()) {
        oldRules = it->second;
    }
    
    // Set custom rules (need to cast away const)
    const_cast<ContentCompositor*>(this)->rules_[type] = rules;
    
    auto result = Compose(bases, newId);
    
    // Restore old rules
    if (oldRules.empty()) {
        const_cast<ContentCompositor*>(this)->rules_.erase(type);
    } else {
        const_cast<ContentCompositor*>(this)->rules_[type] = oldRules;
    }
    
    return result;
}

std::unique_ptr<ContentDefinition> ContentCompositor::CreateVariant(
    const ContentDefinition& base,
    const std::string& newId,
    const simplejson::JsonValue& overrides) const {
    
    simplejson::JsonValue baseJson = base.ToJson();
    
    // Apply overrides
    // Simple implementation - deep merge would be better
    auto keys = overrides.GetKeys();
    for (const auto& key : keys) {
        auto value = overrides.Get(key);
        if (value) {
            baseJson.Set(key, *value);
        }
    }
    
    baseJson.Set("id", newId);
    
    auto result = ContentFactory::Instance().Create(base.GetType(), newId);
    if (result && result->FromJson(baseJson)) {
        return result;
    }
    
    return nullptr;
}

simplejson::JsonValue ContentCompositor::ComposeJson(const std::vector<simplejson::JsonValue>& jsons,
                                         const std::string& contentType) const {
    if (jsons.empty()) {
        return simplejson::JsonValue();
    }
    
    simplejson::JsonValue result = jsons[0];
    
    // Get composition rules for this type
    auto rulesIt = rules_.find(contentType);
    const std::vector<FieldCompositionRule>* typeRules = nullptr;
    if (rulesIt != rules_.end()) {
        typeRules = &rulesIt->second;
    }
    
    // Apply composition
    for (size_t i = 1; i < jsons.size(); ++i) {
        const simplejson::JsonValue& current = jsons[i];
        
        // Get all keys from current
        auto keys = current.GetKeys();
        
        for (const auto& key : keys) {
            auto value = current.Get(key);
            if (!value) continue;
            
            // Check if there's a specific rule for this field
            FieldCompositionRule* rule = nullptr;
            if (typeRules) {
                for (auto& r : *typeRules) {
                    if (r.fieldPath == key) {
                        rule = &r;
                        break;
                    }
                }
            }
            
            if (rule) {
                // Apply rule
                std::vector<simplejson::JsonValue> values;
                auto existingValue = result.Get(key);
                if (existingValue) {
                    values.push_back(*existingValue);
                }
                values.push_back(*value);
                
                simplejson::JsonValue composed = ApplyCompositionRule(values, *rule);
                result.Set(key, composed);
            } else {
                // Default: override
                result.Set(key, *value);
            }
        }
    }
    
    return result;
}

simplejson::JsonValue ContentCompositor::ApplyCompositionRule(
    const std::vector<simplejson::JsonValue>& values,
    const FieldCompositionRule& rule) const {
    
    if (values.empty()) {
        return simplejson::JsonValue();
    }
    
    switch (rule.strategy) {
        case CompositionStrategy::Override:
            return values.back();
            
        case CompositionStrategy::Add:
            if (values[0].IsNumber()) {
                double sum = 0.0;
                for (const auto& v : values) {
                    if (v.IsNumber()) {
                        sum += v.AsNumber();
                    }
                }
                simplejson::JsonValue result;
                result.Set("value", sum);
                return result;
            }
            break;
            
        case CompositionStrategy::Multiply:
            if (values[0].IsNumber()) {
                double product = 1.0;
                for (const auto& v : values) {
                    if (v.IsNumber()) {
                        product *= v.AsNumber();
                    }
                }
                simplejson::JsonValue result;
                result.Set("value", product);
                return result;
            }
            break;
            
        case CompositionStrategy::Min:
            if (values[0].IsNumber()) {
                double minVal = values[0].AsNumber();
                for (size_t i = 1; i < values.size(); ++i) {
                    if (values[i].IsNumber()) {
                        minVal = std::min(minVal, values[i].AsNumber());
                    }
                }
                simplejson::JsonValue result;
                result.Set("value", minVal);
                return result;
            }
            break;
            
        case CompositionStrategy::Max:
            if (values[0].IsNumber()) {
                double maxVal = values[0].AsNumber();
                for (size_t i = 1; i < values.size(); ++i) {
                    if (values[i].IsNumber()) {
                        maxVal = std::max(maxVal, values[i].AsNumber());
                    }
                }
                simplejson::JsonValue result;
                result.Set("value", maxVal);
                return result;
            }
            break;
            
        case CompositionStrategy::Concatenate:
            if (values[0].IsString()) {
                std::string concat;
                for (const auto& v : values) {
                    if (v.IsString()) {
                        concat += v.AsString();
                    }
                }
                simplejson::JsonValue result;
                result.Set("value", concat);
                return result;
            }
            break;
            
        case CompositionStrategy::Custom:
            if (rule.customFunc) {
                return rule.customFunc(values);
            }
            break;
            
        default:
            break;
    }
    
    return values.back();  // Default to override
}

// ContentInheritance implementation
void ContentInheritance::SetBaseContent(const std::string& derivedId, 
                                       const std::string& baseId) {
    baseMap_[derivedId] = baseId;
    derivedMap_[baseId].push_back(derivedId);
}

void ContentInheritance::RemoveInheritance(const std::string& derivedId) {
    auto it = baseMap_.find(derivedId);
    if (it != baseMap_.end()) {
        const std::string& baseId = it->second;
        
        auto& derived = derivedMap_[baseId];
        derived.erase(std::remove(derived.begin(), derived.end(), derivedId), 
                     derived.end());
        
        baseMap_.erase(it);
    }
}

std::string ContentInheritance::GetBaseContent(const std::string& contentId) const {
    auto it = baseMap_.find(contentId);
    return it != baseMap_.end() ? it->second : "";
}

std::vector<std::string> ContentInheritance::GetDerivedContent(
    const std::string& contentId) const {
    auto it = derivedMap_.find(contentId);
    return it != derivedMap_.end() ? it->second : std::vector<std::string>();
}

std::vector<std::string> ContentInheritance::GetInheritanceChain(
    const std::string& contentId) const {
    std::vector<std::string> chain;
    std::string current = contentId;
    
    while (!current.empty()) {
        chain.push_back(current);
        current = GetBaseContent(current);
    }
    
    return chain;
}

std::unique_ptr<ContentDefinition> ContentInheritance::ResolveInheritance(
    const std::string& contentId) const {
    
    auto chain = GetInheritanceChain(contentId);
    if (chain.empty()) {
        return nullptr;
    }
    
    // Collect all content in chain
    std::vector<const ContentDefinition*> bases;
    auto& registry = ContentRegistry::Instance();
    
    // Start from base (reverse order)
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        auto* content = registry.GetContent(*it);
        if (content) {
            bases.push_back(content);
        }
    }
    
    if (bases.empty()) {
        return nullptr;
    }
    
    // Compose from base to derived
    return ContentCompositor::Instance().Compose(bases, contentId);
}

bool ContentInheritance::IsValidInheritance(const std::string& derivedId, 
                                           const std::string& baseId) const {
    // Check for circular inheritance
    auto chain = GetInheritanceChain(baseId);
    return std::find(chain.begin(), chain.end(), derivedId) == chain.end();
}

// ContentTemplate implementation
std::unique_ptr<ContentDefinition> ContentTemplate::Instantiate(
    const std::string& newId,
    const std::unordered_map<std::string, simplejson::JsonValue>& params) const {
    
    simplejson::JsonValue resolved = ResolveTemplate(template_, params);
    resolved.Set("id", newId);
    resolved.Set("type", type_);
    
    auto result = ContentFactory::Instance().Create(type_, newId);
    if (result && result->FromJson(resolved)) {
        return result;
    }
    
    return nullptr;
}

simplejson::JsonValue ContentTemplate::ResolveTemplate(
    const simplejson::JsonValue& json,
    const std::unordered_map<std::string, simplejson::JsonValue>& params) const {
    
    // Simple implementation - would need proper template resolution
    // This is a placeholder for template variable substitution
    return json;
}

// ContentTemplateRegistry implementation
void ContentTemplateRegistry::RegisterTemplate(std::shared_ptr<ContentTemplate> tmpl) {
    if (tmpl) {
        templates_[tmpl->GetId()] = tmpl;
    }
}

std::shared_ptr<ContentTemplate> ContentTemplateRegistry::GetTemplate(
    const std::string& id) const {
    auto it = templates_.find(id);
    return it != templates_.end() ? it->second : nullptr;
}

std::vector<std::string> ContentTemplateRegistry::GetAllTemplates() const {
    std::vector<std::string> result;
    for (const auto& pair : templates_) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<std::string> ContentTemplateRegistry::GetTemplatesByType(
    const std::string& type) const {
    std::vector<std::string> result;
    for (const auto& pair : templates_) {
        if (pair.second->GetType() == type) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::unique_ptr<ContentDefinition> ContentTemplateRegistry::InstantiateTemplate(
    const std::string& templateId,
    const std::string& newId,
    const std::unordered_map<std::string, simplejson::JsonValue>& params) const {
    
    auto tmpl = GetTemplate(templateId);
    if (!tmpl) {
        return nullptr;
    }
    
    return tmpl->Instantiate(newId, params);
}

// CompositionBuilder implementation
std::unique_ptr<ContentDefinition> CompositionBuilder::Build() const {
    std::vector<const ContentDefinition*> bases;
    auto& registry = ContentRegistry::Instance();
    
    for (const auto& id : baseIds_) {
        auto* content = registry.GetContent(id);
        if (content) {
            bases.push_back(content);
        }
    }
    
    if (bases.empty()) {
        return nullptr;
    }
    
    return ContentCompositor::Instance().ComposeWithRules(bases, newId_, rules_);
}

} // namespace NovaEngine
