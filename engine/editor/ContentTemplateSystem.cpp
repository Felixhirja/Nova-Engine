#include "ContentTemplateSystem.h"
#include <iostream>

namespace ContentManagement {

ContentTemplateSystem::ContentTemplateSystem() {
}

ContentTemplateSystem::~ContentTemplateSystem() {
}

std::unique_ptr<simplejson::JsonObject> ContentTemplateSystem::InstantiateTemplate(
    const std::string& templateId,
    const std::unordered_map<std::string, std::string>& variables) {
    
    auto result = std::make_unique<simplejson::JsonObject>();
    
    // Basic template instantiation
    (*result)["template_id"] = simplejson::JsonValue(templateId);
    for (const auto& [key, value] : variables) {
        (*result)[key] = simplejson::JsonValue(value);
    }
    
    return result;
}

void ContentTemplateSystem::RegisterTemplate(const ContentTemplate& templ) {
    ContentTemplate copy;
    copy.id = templ.id;
    copy.name = templ.name;
    copy.description = templ.description;
    copy.category = templ.category;
    copy.variables = templ.variables;
    if (templ.baseContent) {
        copy.baseContent = std::make_unique<simplejson::JsonObject>(*templ.baseContent);
    }
    copy.parentTemplateId = templ.parentTemplateId;
    copy.tags = templ.tags;
    copy.version = templ.version;
    copy.author = templ.author;
    copy.createdDate = templ.createdDate;
    
    templates_[templ.id] = std::move(copy);
}

bool ContentTemplateSystem::LoadTemplates(const std::string& directory) {
    std::cout << "Loading templates from: " << directory << std::endl;
    return true;
}

ContentTemplateSystem::ContentTemplate ContentTemplateSystem::CreateTemplateFromContent(
    const simplejson::JsonObject& content,
    const std::string& templateName,
    const std::string& category) {
    
    ContentTemplate templ;
    templ.id = templateName;
    templ.name = templateName;
    templ.category = category;
    templ.version = 1;
    templ.baseContent = std::make_unique<simplejson::JsonObject>(content);
    
    return templ;
}

bool ContentTemplateSystem::AddVariable(const std::string& templateId, const TemplateVariable& variable) {
    auto it = templates_.find(templateId);
    if (it != templates_.end()) {
        it->second.variables.push_back(variable);
        return true;
    }
    return false;
}

const ContentTemplateSystem::ContentTemplate* ContentTemplateSystem::GetTemplate(const std::string& templateId) const {
    auto it = templates_.find(templateId);
    return (it != templates_.end()) ? &it->second : nullptr;
}

std::vector<std::string> ContentTemplateSystem::GetAllTemplateIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, templ] : templates_) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace ContentManagement
