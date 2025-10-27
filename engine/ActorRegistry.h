#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <utility>
#include <regex>
#include <iostream>

#include "Actor.h"

class ActorRegistry {
public:
    using Factory = std::function<std::unique_ptr<IActor>(const ActorContext&)>;

    static ActorRegistry& Instance();

    void Register(std::string name, Factory factory);
    bool IsRegistered(std::string_view name) const;
    std::unique_ptr<IActor> Create(std::string_view name, const ActorContext& context) const;
    std::vector<std::string> RegisteredActorNames() const;

    // Validation methods
    static bool IsValidActorName(std::string_view name);
    static std::string GetActorNameValidationError(std::string_view name);

private:
    std::unordered_map<std::string, Factory> factories_;

    void LogRegistration(std::string_view name, bool success, const std::string& reason = "");
};

// Simple automatic actor registration macro
#define REGISTER_ACTOR(Type, Name) \
    namespace { \
        struct Type##Registrar { \
            Type##Registrar() { \
                ActorRegistry::Instance().Register(Name, [](const ActorContext& ctx) -> std::unique_ptr<IActor> { \
                    auto actor = std::make_unique<Type>(); \
                    actor->AttachContext(ctx); \
                    return actor; \
                }); \
            } \
        } Type##RegistrarInstance; \
    }
