#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SystemSchedulerV2.h"
#include "SystemTypes.h"

class EntityManager;
namespace ecs {
class SystemSchedulerV2;
class SystemV2;
}

class System {
public:
    virtual ~System() = default;
    virtual void Update(EntityManager& entityManager, double dt) = 0;

    virtual ecs::UpdatePhase GetUpdatePhase() const { return ecs::UpdatePhase::Simulation; }

    virtual std::vector<ecs::ComponentDependency> GetComponentDependencies() const { return {}; }

    virtual std::vector<ecs::SystemDependency> GetSystemDependencies() const { return {}; }

    virtual const char* GetName() const { return typeid(*this).name(); }
};

class SystemManager {
public:
    SystemManager() = default;
    ~SystemManager() = default;

    template<typename T, typename... Args>
    T& RegisterSystem(Args&&... args) {
        static_assert(std::is_base_of<System, T>::value, "System must derive from System base class");
        auto registration = std::make_unique<RegisteredSystem>();
        registration->instance = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *static_cast<T*>(registration->instance.get());
        registration->legacyType = std::type_index(typeid(T));
        registration->wrapperType = std::type_index(typeid(LegacySystemWrapper<T>));
        registration->factory = [](SystemManager& manager, RegisteredSystem& registrationRef) {
            return std::make_unique<LegacySystemWrapper<T>>(manager, registrationRef);
        };
        RefreshRegistrationMetadata(*registration);
        systems_.emplace_back(std::move(registration));
        scheduleDirty_ = true;
        metadataDirty_ = true;
        return ref;
    }

    void Clear();
    void UpdateAll(EntityManager& entityManager, double dt);

    void SetDocumentationOutputPath(std::string path);

    struct SystemMetadata {
        std::string name;
        std::string legacyTypeName;
        ecs::UpdatePhase phase;
        std::vector<ecs::ComponentDependency> componentDependencies;
        std::vector<ecs::SystemDependency> systemDependencies;
    };

    const std::vector<SystemMetadata>& GetRegisteredSystemMetadata() const;

private:
    struct RegisteredSystem {
        std::unique_ptr<System> instance;
        std::type_index legacyType{typeid(void)};
        std::type_index wrapperType{typeid(void)};
        std::string name;
        ecs::UpdatePhase phase{ecs::UpdatePhase::Simulation};
        std::vector<ecs::ComponentDependency> componentDependencies;
        std::vector<ecs::SystemDependency> systemDependencies;
        std::function<std::unique_ptr<ecs::SystemV2>(SystemManager&, RegisteredSystem&)> factory;
    };

    template<typename LegacySystemType>
    class LegacySystemWrapper;

    void BuildSchedule();
    void RefreshRegistrationMetadata(RegisteredSystem& registration);
    void InvokeLegacyUpdate(RegisteredSystem& registration, double dt);
    void EmitComponentConflicts() const;
    void ExportDocumentation() const;
    std::type_index ResolveWrapperType(const std::type_index& legacyType) const;
    bool HasComponentConflict(const std::vector<ecs::ComponentDependency>& a,
                              const std::vector<ecs::ComponentDependency>& b) const;

    std::vector<std::unique_ptr<RegisteredSystem>> systems_;
    mutable std::vector<SystemMetadata> metadataCache_;
    mutable bool metadataDirty_ = true;
    mutable std::unordered_map<std::type_index, std::type_index> wrapperTypeLUT_;
    ecs::SystemSchedulerV2 scheduler_{};
    EntityManager* currentEntityManager_ = nullptr;
    bool scheduleDirty_ = true;
    std::string documentationOutputPath_;
};

template<typename LegacySystemType>
class SystemManager::LegacySystemWrapper : public ecs::SystemV2 {
public:
    LegacySystemWrapper(SystemManager& owner, RegisteredSystem& registration)
        : owner_(owner), registration_(registration) {}

    void Update(ecs::EntityManagerV2& entityManager, double dt) override {
        (void)entityManager;
        owner_.InvokeLegacyUpdate(registration_, dt);
    }

    std::vector<ecs::ComponentDependency> GetDependencies() const override {
        return registration_.componentDependencies;
    }

    std::vector<ecs::SystemDependency> GetSystemDependencies() const override {
        std::vector<ecs::SystemDependency> resolved;
        resolved.reserve(registration_.systemDependencies.size());
        for (const auto& dependency : registration_.systemDependencies) {
            resolved.emplace_back(owner_.ResolveWrapperType(dependency.type));
        }
        return resolved;
    }

    ecs::UpdatePhase GetUpdatePhase() const override {
        return registration_.phase;
    }

    const char* GetName() const override { return registration_.name.c_str(); }

    bool SupportsDuplicateRegistration() const override { return true; }

private:
    SystemManager& owner_;
    RegisteredSystem& registration_;
};
