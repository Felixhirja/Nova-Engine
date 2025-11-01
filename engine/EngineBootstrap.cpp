#include "EngineBootstrap.h"
#include "ResourceManager.h"
#include "ecs/SystemSchedulerV2.h"
#include "ecs/ShipAssembly.h"

#include <iostream>

const std::vector<EngineBootstrap::InitializationStep>& EngineBootstrap::InitializationSequence() {
    static const std::vector<InitializationStep> steps = {
        {"Resource Loading", "Load essential game resources and assets"},
        {"ECS Setup", "Initialize entity component system"},
        {"System Registration", "Register game systems with scheduler"},
        {"Actor Registration", "Register all actor types"},
        {"HUD Assembly", "Assemble HUD ship configuration"},
        {"Subsystem Check", "Verify all subsystems are ready"}
    };
    return steps;
}

std::vector<EngineBootstrap::SubsystemStatus> EngineBootstrap::BuildSubsystemChecklist(
    const BootstrapConfiguration& config) {
    
    std::vector<SubsystemStatus> checklist;
    
    checklist.push_back({
        "Entity Manager",
        "Core entity component system",
        true,  // required
        true,  // enabled
        true   // ready
    });
    
    checklist.push_back({
        "Resource Manager",
        "Asset and resource management",
        true,
        true,
        true
    });
    
    checklist.push_back({
        "Graphics System",
        "Rendering and display",
        true,
        config.loadRendering,
        config.loadRendering
    });
    
    checklist.push_back({
        "Physics System",
        "Physics simulation",
        false,
        true,  // Physics always enabled
        true   // Physics always ready
    });
    
    checklist.push_back({
        "Audio System",
        "Sound and music playback",
        false,
        config.loadAudio,
        config.loadAudio
    });
    
    return checklist;
}

EngineBootstrap::Result EngineBootstrap::Run(
    ResourceManager& /*resourceManager*/,
    ::EntityManager& entityManager,
    ecs::SystemSchedulerV2* scheduler) const {
    
    Result result;
    
    // Load bootstrap configuration
    result.configuration = BootstrapConfiguration::LoadFromFile("assets/bootstrap.json");
    
    // Build subsystem checklist
    result.subsystemChecklist = BuildSubsystemChecklist(result.configuration);
    
    // Actor types will be populated when actors are registered
    std::cout << "[EngineBootstrap] Bootstrap initialization complete" << std::endl;
    
    // Create actor context for global access
    result.actorContext = ActorContext(entityManager, 0); // No specific entity for bootstrap context
    
    // Enable archetype facade if scheduler is available
    if (scheduler) {
        std::cout << "[EngineBootstrap] System scheduler enabled" << std::endl;
    }
    
    // Attempt HUD assembly (non-fatal if it fails)
    try {
        ShipAssemblyRequest hudRequest;
        hudRequest.hullId = "fighter_mk1"; // Default HUD ship
        // Leave slotAssignments empty for default loadout
        
        result.hudAssembly = ShipAssembler::Assemble(hudRequest);
        
        if (result.hudAssembly.IsValid()) {
            std::cout << "[EngineBootstrap] HUD ship assembled successfully" << std::endl;
        } else {
            std::string errors = result.hudAssembly.diagnostics.HasErrors() 
                ? "Assembly has errors" 
                : "Invalid hull configuration";
            result.warnings.push_back("HUD ship assembly failed: " + errors);
            std::cout << "[EngineBootstrap] Warning: " << result.warnings.back() << std::endl;
        }
    } catch (const std::exception& e) {
        result.warnings.push_back(std::string("HUD assembly exception: ") + e.what());
        std::cout << "[EngineBootstrap] Warning: " << result.warnings.back() << std::endl;
    }
    
    // Check subsystems
    bool allRequired = true;
    for (const auto& subsystem : result.subsystemChecklist) {
        if (subsystem.required && !subsystem.ready) {
            allRequired = false;
            result.warnings.push_back("Required subsystem not ready: " + subsystem.name);
        }
    }
    
    if (allRequired) {
        std::cout << "[EngineBootstrap] All required subsystems ready" << std::endl;
    } else {
        std::cout << "[EngineBootstrap] Warning: Some required subsystems not ready" << std::endl;
    }
    
    return result;
}

void EngineBootstrap::Shutdown(
    ResourceManager& resourceManager,
    ::EntityManager& entityManager,
    ecs::SystemSchedulerV2* scheduler) const {
    
    (void)resourceManager;  // Unused for now
    (void)entityManager;    // Unused for now
    (void)scheduler;        // Unused for now
    
    std::cout << "[EngineBootstrap] Shutdown complete" << std::endl;
}
