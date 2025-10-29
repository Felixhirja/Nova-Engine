#include "EngineBootstrap.h"

#include "ActorRegistry.h"
#include "Actors.h"  // Include all actor headers for automatic registration
#include "AudioSystem.h"
#include "BootstrapConfiguration.h"
#include "Input.h"
#include "ResourceManager.h"
#include "Spaceship.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include "ecs/SystemSchedulerV2.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>

namespace {

// Initialization roadmap for bootstrap.  This mirrors the logical sequencing in
// EngineBootstrap::Run and is exposed through InitializationSequence() so that
// external launchers can coordinate subsystem bring-up before invoking Run().
const std::array<EngineBootstrap::InitializationStep, 7> kInitializationSequence = {{
    {"Configure ECS façade", "Enable archetype façade so ActorContext uses the modern entity storage."},
    {"Bind actor context", "Populate ActorContext with entity manager, scheduler, and debug name."},
    {"Discover actor registry", "Query ActorRegistry for all statically registered actors."},
    {"Inspect spaceship catalog", "Ensure spaceship class definitions are loaded before HUD assembly."},
    {"Build default HUD assembly", "Assemble a baseline ship loadout used by the HUD and sample scenes."},
    {"Prepare sprite assets", "Create sprite directories, emit demo textures, and register sprite metadata."},
    {"Spawn demo entities", "Instantiate sample entities to validate ECS wiring and rendering hooks."}
}};

bool RenderingBackendAvailable() {
#if defined(USE_GLFW) || defined(USE_SDL)
    return true;
#else
    return false;
#endif
}

ShipAssemblyResult BuildHudAssembly() {
    ShipAssemblyResult result;

    const ShipHullBlueprint* fighterHull = ShipHullCatalog::Find("fighter_mk1");
    if (!fighterHull) {
        std::cerr << "HUD assembly: failed to locate default fighter hull blueprint" << std::endl;
        return result;
    }

    ShipAssemblyRequest request;
    request.hullId = fighterHull->id;

    auto assignComponent = [](const HullSlot& slot) -> std::string {
        switch (slot.category) {
            case ComponentSlotCategory::PowerPlant:
                return "fusion_core_mk1";
            case ComponentSlotCategory::MainThruster:
                return "main_thruster_viper";
            case ComponentSlotCategory::ManeuverThruster:
                return "rcs_cluster_micro";
            case ComponentSlotCategory::Shield:
                return "shield_array_light";
            case ComponentSlotCategory::Weapon:
                return "weapon_cooling_cannon";
            case ComponentSlotCategory::Sensor:
                return "sensor_targeting_mk1";
            case ComponentSlotCategory::Support:
                return "support_life_pod";
            default:
                return std::string();
        }
    };

    for (const auto& slot : fighterHull->slots) {
        std::string componentId = assignComponent(slot);
        if (componentId.empty()) {
            std::cerr << "HUD assembly: missing default component mapping for slot "
                      << slot.slotId << " (" << ToString(slot.category) << ")" << std::endl;
            continue;
        }
        request.slotAssignments[slot.slotId] = componentId;
    }

    result = ShipAssembler::Assemble(request);

    std::cout << std::fixed << std::setprecision(2);
    if (!result.IsValid()) {
        std::cerr << "HUD assembly: assembly produced " << result.diagnostics.errors.size() << " error(s)" << std::endl;
        for (const auto& err : result.diagnostics.errors) {
            std::cerr << "  -> " << err << std::endl;
        }
    } else {
        // std::cout << "HUD assembly: "
        //           << (result.hull ? result.hull->displayName : std::string("Unknown Hull"))
        //           << " | Net Power " << result.NetPowerMW() << " MW"
        //           << " | Thrust/Mass " << result.ThrustToMassRatio()
        //           << std::endl;
        if (!result.diagnostics.warnings.empty()) {
            // std::cout << "  Warnings:" << std::endl;
            for (const auto& warn : result.diagnostics.warnings) {
                // std::cout << "    * " << warn << std::endl;
            }
        }
    }
    // std::cout.unsetf(std::ios_base::floatfield);
    return result;
}

void EnsureSpriteDirectory() {
    std::error_code ec;
    std::filesystem::create_directories("assets/sprites", ec);
    if (ec) {
        std::cerr << "Failed to create assets/sprites directory: " << ec.message() << std::endl;
    }
}

void WriteSpriteSheet(const std::filesystem::path& path,
                      unsigned char r1, unsigned char g1, unsigned char b1,
                      unsigned char r2, unsigned char g2, unsigned char b2) {
    const int frameWidth = 16;
    const int frameHeight = 16;
    const int width = frameWidth * 2;
    const int height = frameHeight;
    const int rowBytes = ((width * 3 + 3) / 4) * 4;
    int imgSize = rowBytes * height;
    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    int fileSize = 54 + imgSize;
    header[2] = fileSize & 0xFF; header[3] = (fileSize >> 8) & 0xFF;
    header[4] = (fileSize >> 16) & 0xFF; header[5] = (fileSize >> 24) & 0xFF;
    header[10] = 54;
    header[14] = 40;
    header[18] = width & 0xFF; header[19] = (width >> 8) & 0xFF;
    header[22] = height & 0xFF; header[23] = (height >> 8) & 0xFF;
    header[26] = 1; header[28] = 24;

    std::FILE* f = std::fopen(path.string().c_str(), "wb");
    if (!f) {
        std::cerr << "Failed to open " << path << " for writing" << std::endl;
        return;
    }

    std::fwrite(header, 1, 54, f);
    std::unique_ptr<unsigned char[]> row(new unsigned char[rowBytes]);
    for (int y = 0; y < height; ++y) {
        int idx = 0;
        for (int x = 0; x < width; ++x) {
            bool left = (x < frameWidth);
            if (left) {
                row[idx++] = b1; row[idx++] = g1; row[idx++] = r1;
            } else {
                row[idx++] = b2; row[idx++] = g2; row[idx++] = r2;
            }
        }
        while (idx < rowBytes) {
            row[idx++] = 0;
        }
        std::fwrite(row.get(), 1, rowBytes, f);
    }
    std::fclose(f);
}

void RegisterDemoEntity(EntityManager& entityManager, int textureHandle, double xPosition) {
    Entity e = entityManager.CreateEntity();
    auto position = std::make_shared<Position>();
    position->x = xPosition;
    position->y = 0.0;
    position->z = 0.0;
    entityManager.AddComponent<Position>(e, position);

    auto velocity = std::make_shared<Velocity>();
    velocity->vx = 0.0;
    velocity->vy = 0.0;
    velocity->vz = 0.0;
    entityManager.AddComponent<Velocity>(e, velocity);

    auto sprite = std::make_shared<Sprite>();
    sprite->textureHandle = textureHandle;
    sprite->frame = 0;
    entityManager.AddComponent<Sprite>(e, sprite);
}

} // namespace

EngineBootstrap::Result EngineBootstrap::Run(ResourceManager& resourceManager,
                                             EntityManager& entityManager,
                                             ecs::SystemSchedulerV2* scheduler) const {
    Result result;

    std::vector<std::string> warnings;
    const std::filesystem::path configPath{"assets/bootstrap.json"};
    result.configuration = BootstrapConfiguration::LoadFromFile(configPath, &warnings);
    result.subsystemChecklist = BuildSubsystemChecklist(result.configuration);

    for (const auto& status : result.subsystemChecklist) {
        if (status.required && status.enabled && !status.ready) {
            warnings.push_back("Subsystem '" + status.name + "' is required but not initialized when bootstrap runs.");
        }
    }

    // Actors are now automatically registered via static initialization
    entityManager.EnableArchetypeFacade();
    result.actorContext.entityManager = &entityManager.GetArchetypeManager();
    result.actorContext.scheduler = scheduler;
    result.actorContext.debugName = "bootstrap";

    result.actorTypes = ActorRegistry::Instance().RegisteredActorNames();

    const auto& shipClasses = SpaceshipCatalog::All();
    // std::cout << "SpaceshipCatalog: loaded " << shipClasses.size() << " class definitions" << std::endl;
    for (const auto& def : shipClasses) {
        // std::cout << "  -> " << def.displayName << ": " << def.conceptSummary.elevatorPitch << std::endl;
    }

    result.hudAssembly = BuildHudAssembly();

    EnsureSpriteDirectory();

    const std::filesystem::path demoPath1{"assets/sprites/demo1.bmp"};
    const std::filesystem::path demoPath2{"assets/sprites/demo2.bmp"};

    WriteSpriteSheet(demoPath1, 0x4a, 0x90, 0xe2, 0xff, 0xff, 0x00);
    WriteSpriteSheet(demoPath2, 0xe9, 0x4a, 0x4a, 0x4a, 0xff, 0x4a);

    int demoHandle1 = resourceManager.Load(demoPath1.string());
    int demoHandle2 = resourceManager.Load(demoPath2.string());

    ResourceManager::SpriteInfo spriteInfo;
    spriteInfo.frameW = 16;
    spriteInfo.frameH = 16;
    spriteInfo.frames = 2;
    spriteInfo.fps = 2;

    resourceManager.RegisterSprite(demoHandle1, spriteInfo);
    resourceManager.RegisterSprite(demoHandle2, spriteInfo);

    RegisterDemoEntity(entityManager, demoHandle1, -2.0);
    RegisterDemoEntity(entityManager, demoHandle2, 2.0);

    result.warnings = std::move(warnings);
    return result;
}

const std::vector<EngineBootstrap::InitializationStep>& EngineBootstrap::InitializationSequence() {
    static const std::vector<InitializationStep> steps(kInitializationSequence.begin(), kInitializationSequence.end());
    return steps;
}

std::vector<EngineBootstrap::SubsystemStatus> EngineBootstrap::BuildSubsystemChecklist(const BootstrapConfiguration& config) {
    std::vector<SubsystemStatus> checklist;
    checklist.push_back(SubsystemStatus{
        "input",
        "Keyboard/mouse device abstraction (Input::Init must run before bootstrap).",
        true,
        config.loadInput,
        config.loadInput ? Input::IsInitialized() : false});

    checklist.push_back(SubsystemStatus{
        "audio",
        "SDL_mixer driven audio playback via AudioSystem::Initialize().",
        true,
        config.loadAudio,
        config.loadAudio ? AudioSystem::IsInitialized() : false});

    checklist.push_back(SubsystemStatus{
        "rendering",
        "Windowing/renderer bootstrap (SDL or GLFW must create a renderer before Run).",
        true,
        config.loadRendering,
        config.loadRendering ? RenderingBackendAvailable() : false});

    for (const auto& framework : config.optionalFrameworks) {
        checklist.push_back(SubsystemStatus{
            framework,
            "Optional framework requested via bootstrap configuration.",
            false,
            true,
            false});
    }

    return checklist;
}

void EngineBootstrap::Shutdown(ResourceManager& resourceManager,
                               EntityManager& entityManager,
                               ecs::SystemSchedulerV2* scheduler) const {
    entityManager.Clear();
    if (scheduler) {
        scheduler->Clear();
    }

    resourceManager.Shutdown();

    if (AudioSystem::IsInitialized()) {
        AudioSystem::Shutdown();
    }

    if (Input::IsInitialized()) {
        Input::Shutdown();
    }
}
