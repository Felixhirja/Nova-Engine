#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "../ecs/EntityManager.h"
#include "../ecs/ShipAssembly.h"

// Hardpoint specification
struct HardpointSpec {
    HardpointCategory category;
    SlotSize size;
    int count;
    std::string notes;
};

// Hardpoint delta for variants
struct HardpointDelta {
    HardpointCategory category;
    std::optional<SlotSize> sizeDelta;
    int countDelta;
};

// Slot delta for variants
struct SlotDelta {
    ComponentSlotCategory category;
    std::optional<SlotSize> size;
    std::optional<SlotSize> sizeDelta;
    int countDelta;
};

// Passive buff specification
struct PassiveBuff {
    std::string type;
    double value;
    std::string statName;
    double multiplier;
};

// Spaceship concept summary
struct SpaceshipConceptSummary {
    std::string role;
    std::string description;
    std::string elevatorPitch;
    std::vector<std::string> gameplayHooks;
};

// Progression tier
struct ProgressionTier {
    int tier;
    std::string name;
    std::string description;
    std::vector<std::string> unlocks;
};

// Progression metadata
struct ProgressionMetadata {
    int minLevel;
    int blueprintCost;
    int factionReputation;
};

// Variant specification
struct VariantSpec {
    std::string codename;
    std::string name;
    std::string description;
    std::string faction;
    std::vector<HardpointDelta> hardpointDeltas;
    std::vector<SlotDelta> slotDeltas;
    std::vector<PassiveBuff> passiveBuffs;
};

// Default loadout
struct DefaultLoadout {
    std::string name;
    std::string description;
    std::vector<std::string> components;
};

// Resolved default loadout
struct ResolvedDefaultLoadout {
    std::string name;
    const DefaultLoadout* loadout;
    ShipAssemblyRequest assemblyRequest;
    std::vector<ShipAssemblyRequest> requests;
};

// Spaceship variant layout
struct SpaceshipVariantLayout {
    std::vector<HardpointSpec> hardpoints;
    std::vector<ComponentSlotSpec> slots;
    std::vector<ComponentSlotSpec> componentSlots;
    std::vector<PassiveBuff> passiveBuffs;
};

// Spaceship class catalog entry
struct SpaceshipClassCatalogEntry {
    std::string id;
    SpaceshipClassType type;
    std::string displayName;
    SpaceshipConceptSummary conceptSummary;
    SpaceshipBaselineSpec baseline;
    std::vector<HardpointSpec> hardpoints;
    std::vector<ComponentSlotSpec> componentSlots;
    std::vector<ProgressionTier> progression;
    ProgressionMetadata progressionMetadata;
    std::vector<VariantSpec> variants;
    std::vector<DefaultLoadout> defaultLoadouts;
};

// Spaceship spawn bundle
class SpaceshipSpawnBundle {
public:
    std::string classId;
    std::string displayName;
    SpaceshipClassDefinition definition;
    ShipAssemblyRequest assemblyRequest;
    std::vector<ShipAssemblyRequest> loadoutRequests;
    int loadoutIndex;
    bool playerControlled;
};

// Spaceship catalog management system
class SpaceshipCatalog {
public:
    // Get all available spaceship classes
    static const std::vector<SpaceshipClassCatalogEntry>& All();

    // Find a spaceship class by ID
    static const SpaceshipClassCatalogEntry* FindById(const std::string& id);

    // Force reload the catalog from disk
    static void Reload();

    // Enable/disable hot reloading of catalog files
    static void EnableHotReload(bool enabled);

    // Check for changes and reload if needed (call this periodically)
    static void TickHotReload();

    // Get validation errors from last load
    static const std::vector<std::string>& ValidationErrors();

    // Build class definition from catalog entry
    static SpaceshipClassDefinition BuildClassDefinition(const SpaceshipClassCatalogEntry& entry);

    // Build default loadout requests
    static std::vector<ShipAssemblyRequest> BuildDefaultLoadoutRequests(const SpaceshipClassCatalogEntry& entry);

    // Resolve a specific default loadout
    static ResolvedDefaultLoadout ResolveDefaultLoadout(const SpaceshipClassCatalogEntry& entry,
                                                       const DefaultLoadout& loadout);

    // Build assembly request for a specific loadout
    static std::optional<ShipAssemblyRequest> BuildDefaultLoadoutRequest(const std::string& classId,
                                                                        const std::string& loadoutName);

    // Resolve variant layout modifications
    static SpaceshipVariantLayout ResolveVariantLayout(const SpaceshipClassCatalogEntry& entry,
                                                      const VariantSpec& variant);

    // Build spawn bundle for a spaceship
    static SpaceshipSpawnBundle BuildSpawnBundle(const SpaceshipClassCatalogEntry& entry,
                                                const DefaultLoadout& loadout,
                                                int loadoutIndex,
                                                const std::string& hullSuffix = "");
};

// High-level spaceship spawning function
Entity SpawnSpaceship(EntityManager& entityManager, const SpaceshipSpawnBundle& bundle);