#include "ActorRegistry.h"

#include <algorithm>
#include <iostream>
#include <regex>

ActorRegistry& ActorRegistry::Instance() {
    static ActorRegistry registry;
    return registry;
}

void ActorRegistry::Register(std::string name, Factory factory) {
    // Validate actor name
    if (!IsValidActorName(name)) {
        LogRegistration(name, false, GetActorNameValidationError(name));
        return;
    }

    // Check for null factory
    if (!factory) {
        LogRegistration(name, false, "factory function is null");
        return;
    }

    // Check for duplicate registration
    if (IsRegistered(name)) {
        LogRegistration(name, false, "actor name already registered");
        return;
    }

    // Register the actor
    factories_[std::move(name)] = std::move(factory);
    LogRegistration(name, true);
}

bool ActorRegistry::IsRegistered(std::string_view name) const {
    return factories_.find(std::string(name)) != factories_.end();
}

std::unique_ptr<IActor> ActorRegistry::Create(std::string_view name, const ActorContext& context) const {
    auto it = factories_.find(std::string(name));
    if (it == factories_.end()) {
        std::cerr << "[ActorRegistry] Failed to create actor '" << name
                  << "': actor not registered" << std::endl;
        return nullptr;
    }
    try {
        return it->second(context);
    } catch (const std::exception& e) {
        std::cerr << "[ActorRegistry] Failed to create actor '" << name
                  << "': " << e.what() << std::endl;
        return nullptr;
    }
}

std::vector<std::string> ActorRegistry::RegisteredActorNames() const {
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (const auto& [key, _] : factories_) {
        names.push_back(key);
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool ActorRegistry::IsValidActorName(std::string_view name) {
    // Actor names must:
    // - Not be empty
    // - Start with a capital letter
    // - Contain only letters, numbers, and underscores
    // - Not start or end with underscore
    // - Not contain consecutive underscores

    if (name.empty()) {
        return false;
    }

    // Must start with capital letter
    if (!std::isupper(name[0])) {
        return false;
    }

    // Check each character
    bool lastWasUnderscore = false;
    for (size_t i = 0; i < name.length(); ++i) {
        char c = name[i];
        if (std::isalnum(c)) {
            lastWasUnderscore = false;
        } else if (c == '_') {
            // No consecutive underscores, no leading/trailing underscore
            if (lastWasUnderscore || i == 0 || i == name.length() - 1) {
                return false;
            }
            lastWasUnderscore = true;
        } else {
            // Invalid character
            return false;
        }
    }

    return true;
}

std::string ActorRegistry::GetActorNameValidationError(std::string_view name) {
    if (name.empty()) {
        return "actor name cannot be empty";
    }

    if (!std::isupper(name[0])) {
        return "actor name must start with a capital letter";
    }

    bool lastWasUnderscore = false;
    for (size_t i = 0; i < name.length(); ++i) {
        char c = name[i];
        if (std::isalnum(c)) {
            lastWasUnderscore = false;
        } else if (c == '_') {
            if (lastWasUnderscore) {
                return "actor name cannot contain consecutive underscores";
            }
            if (i == 0) {
                return "actor name cannot start with underscore";
            }
            if (i == name.length() - 1) {
                return "actor name cannot end with underscore";
            }
            lastWasUnderscore = true;
        } else {
            return "actor name can only contain letters, numbers, and underscores";
        }
    }

    return ""; // Valid
}

void ActorRegistry::LogRegistration(std::string_view name, bool success, const std::string& reason) {
    if (success) {
        std::cout << "[ActorRegistry] Successfully registered actor '" << name << "'" << std::endl;
    } else {
        std::cerr << "[ActorRegistry] Failed to register actor '" << name << "'";
        if (!reason.empty()) {
            std::cerr << ": " << reason;
        }
        std::cerr << std::endl;
    }
}

// Manual actor registration - add new actors here
void RegisterAllActors() {
    // This function is called once at startup to register all actors
    // Add new actor registrations here when you create new actor types

    // Note: This is a manual process, but it's simple and clear
    // Just add one line per new actor type
}
