#include "System.h"
#include "EntityManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string PhaseToString(ecs::UpdatePhase phase) {
    switch (phase) {
        case ecs::UpdatePhase::Input:
            return "Input";
        case ecs::UpdatePhase::Simulation:
            return "Simulation";
        case ecs::UpdatePhase::RenderPrep:
            return "Render Prep";
    }
    return "Unknown";
}

const char* AccessToString(ecs::ComponentAccess access) {
    switch (access) {
        case ecs::ComponentAccess::Read:
            return "Read";
        case ecs::ComponentAccess::Write:
            return "Write";
        case ecs::ComponentAccess::ReadWrite:
            return "Read/Write";
    }
    return "Unknown";
}

std::string FormatComponentList(const std::vector<ecs::ComponentDependency>& dependencies) {
    if (dependencies.empty()) {
        return "None";
    }

    std::ostringstream stream;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        const auto& dep = dependencies[i];
        stream << dep.type.name() << " (" << AccessToString(dep.access) << ")";
        if (i + 1 < dependencies.size()) {
            stream << "<br/>";
        }
    }
    return stream.str();
}

std::string FormatSystemDependencyList(const std::vector<ecs::SystemDependency>& dependencies) {
    if (dependencies.empty()) {
        return "None";
    }

    std::ostringstream stream;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        stream << dependencies[i].type.name();
        if (i + 1 < dependencies.size()) {
            stream << "<br/>";
        }
    }
    return stream.str();
}

} // namespace

void SystemManager::Clear() {
    scheduler_.Clear();
    systems_.clear();
    metadataCache_.clear();
    wrapperTypeLUT_.clear();
    metadataDirty_ = true;
    scheduleDirty_ = true;
    currentEntityManager_ = nullptr;
}

void SystemManager::UpdateAll(EntityManager& entityManager, double dt) {
    entityManager.EnableArchetypeFacade();
    BuildSchedule();

    struct EntityManagerScope {
        SystemManager& manager;
        ~EntityManagerScope() { manager.currentEntityManager_ = nullptr; }
    } scope{*this};

    currentEntityManager_ = &entityManager;
    scheduler_.UpdateAll(entityManager.GetArchetypeManager(), dt);
}

void SystemManager::SetDocumentationOutputPath(std::string path) {
    documentationOutputPath_ = std::move(path);
    ExportDocumentation();
}

const std::vector<SystemManager::SystemMetadata>& SystemManager::GetRegisteredSystemMetadata() const {
    if (!metadataDirty_) {
        return metadataCache_;
    }

    metadataCache_.clear();
    metadataCache_.reserve(systems_.size());

    for (const auto& registration : systems_) {
        if (!registration || !registration->instance) {
            continue;
        }

        metadataCache_.push_back(SystemMetadata{
            registration->name,
            registration->legacyType.name(),
            registration->phase,
            registration->componentDependencies,
            registration->systemDependencies});
    }

    metadataDirty_ = false;
    return metadataCache_;
}

void SystemManager::BuildSchedule() {
    if (!scheduleDirty_) {
        return;
    }

    wrapperTypeLUT_.clear();
    scheduler_.Clear();

    for (auto& registration : systems_) {
        if (!registration) {
            continue;
        }
        RefreshRegistrationMetadata(*registration);
        wrapperTypeLUT_.insert_or_assign(registration->legacyType, registration->wrapperType);
    }

    for (auto& registration : systems_) {
        if (!registration || !registration->factory) {
            continue;
        }
        scheduler_.RegisterSystemInstance(registration->factory(*this, *registration));
    }

    scheduleDirty_ = false;
    EmitComponentConflicts();
    ExportDocumentation();
}

void SystemManager::RefreshRegistrationMetadata(RegisteredSystem& registration) {
    if (!registration.instance) {
        registration.name = "<null system>";
        registration.phase = ecs::UpdatePhase::Simulation;
        registration.componentDependencies.clear();
        registration.systemDependencies.clear();
        metadataDirty_ = true;
        return;
    }

    registration.phase = registration.instance->GetUpdatePhase();
    registration.componentDependencies = registration.instance->GetComponentDependencies();
    registration.systemDependencies = registration.instance->GetSystemDependencies();

    const char* name = registration.instance->GetName();
    if (name && *name) {
        registration.name = name;
    } else {
        registration.name = registration.legacyType.name();
    }

    metadataDirty_ = true;
}

void SystemManager::InvokeLegacyUpdate(RegisteredSystem& registration, double dt) {
    if (!currentEntityManager_) {
        throw std::runtime_error("SystemManager attempted to update a system without an active EntityManager");
    }

    registration.instance->Update(*currentEntityManager_, dt);
}

void SystemManager::EmitComponentConflicts() const {
    if (systems_.empty()) {
        return;
    }

    std::map<ecs::UpdatePhase, std::vector<const RegisteredSystem*>> grouped;
    for (const auto& registration : systems_) {
        if (!registration) {
            continue;
        }
        grouped[registration->phase].push_back(registration.get());
    }

    for (const auto& [phase, systemsInPhase] : grouped) {
        for (std::size_t i = 0; i < systemsInPhase.size(); ++i) {
            for (std::size_t j = i + 1; j < systemsInPhase.size(); ++j) {
                const auto* a = systemsInPhase[i];
                const auto* b = systemsInPhase[j];
                if (!a || !b) {
                    continue;
                }

                if (!HasComponentConflict(a->componentDependencies, b->componentDependencies)) {
                    continue;
                }

                std::cerr << "[SystemManager] Warning: component access conflict detected in phase "
                          << PhaseToString(phase) << " between systems '" << a->name << "' and '"
                          << b->name << "'." << std::endl;
            }
        }
    }
}

void SystemManager::ExportDocumentation() const {
    if (documentationOutputPath_.empty()) {
        return;
    }

    const auto& metadata = GetRegisteredSystemMetadata();

    std::filesystem::path outputPath(documentationOutputPath_);
    if (!outputPath.empty() && outputPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(outputPath.parent_path(), ec);
        if (ec) {
            std::cerr << "[SystemManager] Failed to create documentation directory '"
                      << outputPath.parent_path().string() << "': " << ec.message() << std::endl;
        }
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        std::cerr << "[SystemManager] Failed to write dependency documentation to "
                  << outputPath.string() << std::endl;
        return;
    }

    output << "# System Dependency Map\n\n";

    if (metadata.empty()) {
        output << "_No systems registered._\n";
        return;
    }

    std::map<ecs::UpdatePhase, std::vector<const SystemMetadata*>> grouped;
    for (const auto& entry : metadata) {
        grouped[entry.phase].push_back(&entry);
    }

    for (const auto& [phase, entries] : grouped) {
        output << "## Phase: " << PhaseToString(phase) << "\n\n";
        output << "| System | Legacy Type | Component Access | System Dependencies |\n";
        output << "| --- | --- | --- | --- |\n";
        for (const auto* entry : entries) {
            output << "| " << entry->name << " | `" << entry->legacyTypeName << "` | "
                   << FormatComponentList(entry->componentDependencies) << " | "
                   << FormatSystemDependencyList(entry->systemDependencies) << " |\n";
        }
        output << "\n";
    }
}

std::type_index SystemManager::ResolveWrapperType(const std::type_index& legacyType) const {
    auto it = wrapperTypeLUT_.find(legacyType);
    if (it != wrapperTypeLUT_.end()) {
        return it->second;
    }

    for (const auto& pair : wrapperTypeLUT_) {
        if (pair.second == legacyType) {
            return legacyType;
        }
    }

    throw std::runtime_error(std::string("SystemManager dependency for type ") + legacyType.name() +
                             " is not registered.");
}

bool SystemManager::HasComponentConflict(const std::vector<ecs::ComponentDependency>& a,
                                         const std::vector<ecs::ComponentDependency>& b) const {
    for (const auto& depA : a) {
        for (const auto& depB : b) {
            if (depA.type != depB.type) {
                continue;
            }

            const bool aWrites = depA.access == ecs::ComponentAccess::Write ||
                                 depA.access == ecs::ComponentAccess::ReadWrite;
            const bool bWrites = depB.access == ecs::ComponentAccess::Write ||
                                 depB.access == ecs::ComponentAccess::ReadWrite;

            if (aWrites || bWrites) {
                return true;
            }
        }
    }

    return false;
}
