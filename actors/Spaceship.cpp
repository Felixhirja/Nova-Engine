#include "Spaceship.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace {

ComponentSlotSpec Slot(ComponentSlotCategory category, SlotSize size, int count, std::string notes) {
    ComponentSlotSpec spec;
    spec.category = category;
    spec.size = size;
    spec.count = count;
    spec.notes = std::move(notes);
    return spec;
}

HardpointSpec Hardpoint(HardpointCategory category, SlotSize size, int count, std::string notes) {
    HardpointSpec spec;
    spec.category = category;
    spec.size = size;
    spec.count = count;
    spec.notes = std::move(notes);
    return spec;
}

HardpointDelta HardpointChange(HardpointCategory category, int countDelta, std::optional<SlotSize> sizeDelta = std::nullopt) {
    HardpointDelta delta;
    delta.category = category;
    delta.countDelta = countDelta;
    delta.sizeDelta = sizeDelta;
    return delta;
}

SlotDelta SlotChange(ComponentSlotCategory category, int countDelta, std::optional<SlotSize> size = std::nullopt) {
    SlotDelta delta;
    delta.category = category;
    delta.countDelta = countDelta;
    delta.size = size;
    return delta;
}

SpaceshipClassCatalogEntry BuildFighter() {
    SpaceshipClassCatalogEntry entry;
    entry.id = "fighter";
    entry.type = SpaceshipClassType::Fighter;
    entry.displayName = "Fighter";
    entry.conceptSummary = {
        "Agile interception craft built for rapid-response dogfighting.",
        {
            "High thrust-to-weight ratio enabling extreme acceleration",
            "Compact profile optimized for carrier deployment",
            "Limited endurance balanced by modular avionics upgrades"
        }
    };
    entry.baseline = {25.0, 35.0, 1, 2, 8.0, 12.0};
    entry.hardpoints = {
        Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Small, 2, "Fixed or gimbaled energy/ballistic cannons"),
        Hardpoint(HardpointCategory::Utility, SlotSize::XS, 1, "Countermeasure pod or sensor jammer"),
        Hardpoint(HardpointCategory::Module, SlotSize::Small, 1, "Avionics suite, stealth package, or auxiliary fuel tank")
    };
    entry.componentSlots = {
        Slot(ComponentSlotCategory::PowerPlant, SlotSize::Small, 1, "Compact fusion core"),
        Slot(ComponentSlotCategory::MainThruster, SlotSize::Small, 1, "Main engine block with afterburner"),
        Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::XS, 4, "Vectored control thrusters"),
        Slot(ComponentSlotCategory::Shield, SlotSize::Small, 1, "Lightweight directional shield generator"),
        Slot(ComponentSlotCategory::Weapon, SlotSize::Small, 2, "Weapon cooling/targeting subsystems"),
        Slot(ComponentSlotCategory::Sensor, SlotSize::Small, 1, "Combat-grade targeting computer"),
        Slot(ComponentSlotCategory::Support, SlotSize::XS, 1, "Emergency life-support capsule")
    };
    entry.progression = {
        {1, "Starter Interceptor", "Entry-level hull unlocked during tutorial arc."},
        {2, "Specialist Interceptor", "Enhanced maneuvering thrusters and avionics."},
        {3, "Elite Strike Fighter", "Modular wing pylons with stealth/strike packages."}
    };
    entry.variants = {
        {
            "Terran Navy",
            "Raptor",
            "Balanced stats with missile rack integration.",
            {HardpointChange(HardpointCategory::Utility, 1, SlotSize::Small)},
            {},
            {PassiveBuff{"missile_lock_time", -0.15}}
        },
        {
            "Outer Rim Syndicate",
            "Viper",
            "Sacrifices armor for boosted engines and smuggling compartment.",
            {},
            {SlotChange(ComponentSlotCategory::Cargo, 1, SlotSize::XS)},
            {
                PassiveBuff{"thrust_multiplier", 0.1},
                PassiveBuff{"shield_capacity", -0.2}
            }
        },
        {
            "Zenith Collective",
            "Aurora",
            "Energy re-routing module for sustained beam weapons.",
            {},
            {},
            {
                PassiveBuff{"beam_weapon_efficiency", 0.25},
                PassiveBuff{"energy_weapon_heat", -0.1}
            }
        }
    };
    entry.progressionMetadata = {1, 0, 1000};
    entry.defaultLoadouts = {
        {
            "Starter Fighter",
            "Basic fighter configuration for new pilots",
            {"fusion_core_mk1", "main_thruster_viper", "rcs_cluster_micro", "shield_array_light",
             "weapon_twin_cannon", "sensor_targeting_mk1", "support_life_pod"}
        }
    };
    return entry;
}

SpaceshipClassCatalogEntry BuildFreighter() {
    SpaceshipClassCatalogEntry entry;
    entry.id = "freighter";
    entry.type = SpaceshipClassType::Freighter;
    entry.displayName = "Freighter";
    entry.conceptSummary = {
        "Versatile cargo hauler that anchors trade routes and logistics chains.",
        {
            "Modular container bays and detachable cargo pods",
            "Reinforced frames for micro-jump stability",
            "Defensive focus on countermeasures and drone escorts"
        }
    };
    entry.baseline = {90.0, 120.0, 2, 4, 18.0, 26.0};
    entry.hardpoints = {
        Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 1, "Defensive turret covering dorsal arc"),
        Hardpoint(HardpointCategory::Utility, SlotSize::Small, 2, "Countermeasures, tractor beam, or repair drone"),
        Hardpoint(HardpointCategory::Module, SlotSize::Medium, 3, "Cargo bay extensions, shield capacitor, drone bay")
    };
    entry.componentSlots = {
        Slot(ComponentSlotCategory::PowerPlant, SlotSize::Medium, 1, "High-endurance reactor core"),
        Slot(ComponentSlotCategory::MainThruster, SlotSize::Medium, 2, "Dual main engines with cargo-tuned exhaust"),
        Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Small, 6, "Station-keeping thruster clusters"),
        Slot(ComponentSlotCategory::Shield, SlotSize::Medium, 1, "Omni-directional cargo shield generator"),
        Slot(ComponentSlotCategory::Cargo, SlotSize::Large, 3, "Container racks or specialized payload modules"),
        Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Small, 1, "Extended crew habitation module"),
        Slot(ComponentSlotCategory::Sensor, SlotSize::Medium, 1, "Logistics-grade navigation array"),
        Slot(ComponentSlotCategory::Support, SlotSize::Medium, 1, "Docking collar or drone control bay")
    };
    entry.progression = {
        {1, "Light Hauler", "Compact freighters for intra-system trade."},
        {2, "Convoy Freighter", "Detachable cargo pods with improved security."},
        {3, "Heavy Transport", "Jump-capable cargo frames with automated loaders."}
    };
    entry.variants = {
        {
            "Terran Commerce Guild",
            "Atlas",
            "Security seals and customs compliance modules.",
            {},
            {SlotChange(ComponentSlotCategory::Support, 1, SlotSize::Small)},
            {
                PassiveBuff{"cargo_security", 1.0},
                PassiveBuff{"trade_efficiency", 0.15}
            }
        },
        {
            "Frontier Miners Union",
            "Prospector",
            "Swappable mining rigs and ore refining bay.",
            {HardpointChange(HardpointCategory::Module, 1, SlotSize::Large)},
            {SlotChange(ComponentSlotCategory::Industrial, 2, SlotSize::Medium)},
            {
                PassiveBuff{"mining_yield", 0.25},
                PassiveBuff{"ore_processing", 1.0}
            }
        },
        {
            "Free Traders League",
            "Nomad",
            "Expanded crew quarters and smuggling compartments.",
            {},
            {
                SlotChange(ComponentSlotCategory::CrewQuarters, 1, SlotSize::Medium),
                SlotChange(ComponentSlotCategory::Cargo, 1, SlotSize::Medium)
            },
            {
                PassiveBuff{"crew_morale", 0.2},
                PassiveBuff{"smuggling_capacity", 1.0}
            }
        }
    };
    entry.progressionMetadata = {3, 10, 2500};
    entry.defaultLoadouts = {
        {
            "Standard Cargo Hauler",
            "Reliable configuration for general freight operations",
            {"fusion_core_mk2", "main_thruster_freighter", "rcs_cluster_micro", "shield_array_medium",
             "cargo_rack_standard", "sensor_targeting_mk1", "support_life_pod"}
        }
    };
    return entry;
}

SpaceshipClassCatalogEntry BuildExplorer() {
    SpaceshipClassCatalogEntry entry;
    entry.id = "explorer";
    entry.type = SpaceshipClassType::Explorer;
    entry.displayName = "Explorer";
    entry.conceptSummary = {
        "Long-range survey vessel outfitted for science expeditions and reconnaissance.",
        {
            "Extended sensor suites and survey drones",
            "Hybrid drives enabling atmospheric descent",
            "Laboratory-grade module capacity"
        }
    };
    entry.baseline = {80.0, 95.0, 3, 5, 16.0, 22.0};
    entry.hardpoints = {
        Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 1, "Defensive turret or rail repeater"),
        Hardpoint(HardpointCategory::Utility, SlotSize::Small, 3, "Sensor array, drone control, repair beam"),
        Hardpoint(HardpointCategory::Module, SlotSize::Medium, 3, "Labs, data core, stealth probe bay")
    };
    entry.componentSlots = {
        Slot(ComponentSlotCategory::PowerPlant, SlotSize::Medium, 1, "Efficient long-range reactor"),
        Slot(ComponentSlotCategory::MainThruster, SlotSize::Medium, 1, "Hybrid atmospheric/space engine"),
        Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Small, 6, "Precision RCS clusters"),
        Slot(ComponentSlotCategory::Shield, SlotSize::Medium, 1, "Adaptive shield lattice"),
        Slot(ComponentSlotCategory::Sensor, SlotSize::Large, 2, "Long-range sensor and science array"),
        Slot(ComponentSlotCategory::Support, SlotSize::Medium, 2, "Survey drone racks, repair gantry"),
        Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Small, 1, "Science team habitation pod"),
        Slot(ComponentSlotCategory::Cargo, SlotSize::Medium, 1, "Sample containment hold")
    };
    entry.progression = {
        {1, "Survey Corvette", "Planetary mapping contracts and exploration."},
        {2, "Deep-space Scout", "Long-range jump matrix with cloaked probes."},
        {3, "Expedition Cruiser", "Onboard fabrication and anomaly shielding."}
    };
    entry.variants = {
        {
            "Academy of Sciences",
            "Odyssey",
            "Enhanced lab capacity and science buffs.",
            {HardpointChange(HardpointCategory::Module, 1, SlotSize::Large)},
            {SlotChange(ComponentSlotCategory::Sensor, 1, SlotSize::XL)},
            {
                PassiveBuff{"science_scan_rate", 0.3},
                PassiveBuff{"research_output", 0.25}
            }
        },
        {
            "Free Horizon Cartographers",
            "Pathfinder",
            "Jump range bonuses and terrain scanners.",
            {},
            {},
            {
                PassiveBuff{"jump_range", 0.2},
                PassiveBuff{"terrain_scan_quality", 0.4}
            }
        },
        {
            "Shadow Consortium",
            "Phantom",
            "Sensor-masking systems and covert data vaults.",
            {},
            {SlotChange(ComponentSlotCategory::Cargo, 1, SlotSize::Small)},
            {
                PassiveBuff{"stealth_rating", 0.35},
                PassiveBuff{"sensor_masking", 1.0}
            }
        }
    };
    entry.progressionMetadata = {5, 20, 3000};
    entry.defaultLoadouts = {
        {
            "Science Surveyor",
            "Equipped for planetary exploration and data collection",
            {"fusion_core_mk2", "main_thruster_freighter", "rcs_cluster_micro", "shield_array_medium",
             "sensor_targeting_mk1", "cargo_rack_standard", "support_life_pod"}
        }
    };
    return entry;
}

SpaceshipClassCatalogEntry BuildIndustrial() {
    SpaceshipClassCatalogEntry entry;
    entry.id = "industrial";
    entry.type = SpaceshipClassType::Industrial;
    entry.displayName = "Industrial";
    entry.conceptSummary = {
        "Heavy utility platform supporting mining, salvage, and construction operations.",
        {
            "High-capacity power distribution for industrial tools",
            "Expanded utility slots for drones and fabrication rigs",
            "Armored hull optimized for hazardous environments"
        }
    };
    entry.baseline = {140.0, 180.0, 4, 6, 24.0, 34.0};
    entry.hardpoints = {
        Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 2, "Defensive cannons covering broad arcs"),
        Hardpoint(HardpointCategory::Utility, SlotSize::Medium, 2, "Tractor beams, repair projectors"),
        Hardpoint(HardpointCategory::Module, SlotSize::Large, 4, "Mining rigs, fabrication arrays, salvage bay, shield inducers")
    };
    entry.componentSlots = {
        Slot(ComponentSlotCategory::PowerPlant, SlotSize::Large, 1, "Industrial-grade reactor core"),
        Slot(ComponentSlotCategory::MainThruster, SlotSize::Large, 2, "Heavy-duty propulsion blocks"),
        Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Medium, 8, "Directional thruster girdles"),
        Slot(ComponentSlotCategory::Shield, SlotSize::Large, 1, "Reinforced containment shields"),
        Slot(ComponentSlotCategory::Industrial, SlotSize::Large, 4, "Mining lasers, repair gantries, fabrication rigs"),
        Slot(ComponentSlotCategory::Cargo, SlotSize::Large, 2, "Bulk ore hoppers or construction material bins"),
        Slot(ComponentSlotCategory::Support, SlotSize::Medium, 2, "Drone hangars or crane assemblies"),
        Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Medium, 1, "Work crew habitation")
    };
    entry.progression = {
        {1, "Utility Platform", "Salvage and repair missions in low-risk zones."},
        {2, "Deep-core Miner", "Armored drill heads with ore refineries."},
        {3, "Construction Platform", "Deploys outposts and orbital structures."}
    };
    entry.variants = {
        {
            "Union of Labor",
            "Forge",
            "Resilient hull with redundant systems.",
            {},
            {},
            {
                PassiveBuff{"hull_integrity", 0.2},
                PassiveBuff{"system_redundancy", 0.3}
            }
        },
        {
            "Corporate Combine",
            "Constructor",
            "Advanced fabrication modules and supply bonuses.",
            {HardpointChange(HardpointCategory::Module, 1, SlotSize::XL)},
            {SlotChange(ComponentSlotCategory::Industrial, 1, SlotSize::XL)},
            {
                PassiveBuff{"fabrication_speed", 0.4},
                PassiveBuff{"supply_chain_efficiency", 0.25}
            }
        },
        {
            "Scavenger Clans",
            "Scrap Queen",
            "Expanded salvage bays and crane arms.",
            {HardpointChange(HardpointCategory::Utility, 1, SlotSize::Large)},
            {SlotChange(ComponentSlotCategory::Cargo, 1, SlotSize::XL)},
            {
                PassiveBuff{"salvage_yield", 0.35},
                PassiveBuff{"wreck_processing", 1.0}
            }
        }
    };
    entry.progressionMetadata = {7, 30, 4000};
    entry.defaultLoadouts = {
        {
            "Mining Platform",
            "Configured for asteroid mining and resource extraction",
            {"fusion_core_mk2", "main_thruster_freighter", "rcs_cluster_micro", "shield_array_heavy",
             "cargo_rack_standard", "sensor_targeting_mk1", "support_life_pod"}
        }
    };
    return entry;
}

SpaceshipClassCatalogEntry BuildCapital() {
    SpaceshipClassCatalogEntry entry;
    entry.id = "capital";
    entry.type = SpaceshipClassType::Capital;
    entry.displayName = "Capital";
    entry.conceptSummary = {
        "Command-and-control flagships capable of projecting force and supporting fleets.",
        {
            "Multiple subsystem redundancies and distributed crew stations",
            "Acts as mobile base with hangar capacity",
            "Hosts advanced command and logistics suites"
        }
    };
    entry.baseline = {600.0, 950.0, 8, 18, 60.0, 120.0};
    entry.hardpoints = {
        Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::XL, 6, "Turrets or beam arrays spanning ship arcs"),
        Hardpoint(HardpointCategory::Utility, SlotSize::Large, 4, "Point-defense grids, sensor masts"),
        Hardpoint(HardpointCategory::Module, SlotSize::XL, 6, "Hangars, shield amplifiers, command modules, medical bays")
    };
    entry.componentSlots = {
        Slot(ComponentSlotCategory::PowerPlant, SlotSize::XL, 2, "Redundant flagship cores"),
        Slot(ComponentSlotCategory::MainThruster, SlotSize::XL, 4, "Capital propulsion arrays"),
        Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Large, 12, "Distributed RCS banks"),
        Slot(ComponentSlotCategory::Shield, SlotSize::XL, 2, "Layered shield projectors"),
        Slot(ComponentSlotCategory::Hangar, SlotSize::XL, 2, "Strike craft or shuttle hangars"),
        Slot(ComponentSlotCategory::Support, SlotSize::Large, 4, "Command, medical, fabrication suites"),
        Slot(ComponentSlotCategory::Sensor, SlotSize::Large, 2, "Long-range tactical sensor masts"),
        Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Large, 3, "Distributed crew habitats"),
        Slot(ComponentSlotCategory::Industrial, SlotSize::Large, 1, "Fleet support fabrication plant")
    };
    entry.progression = {
        {2, "Escort Carrier", "Accessible via faction reputation milestones."},
        {3, "Battlecruiser", "Command dreadnought unlocked in endgame campaigns."},
        {4, "Legendary Flagship", "Narrative-locked capital hull with unique bonuses."}
    };
    entry.variants = {
        {
            "Terran Navy",
            "Resolute",
            "Balanced defenses with fighter bay bonuses.",
            {},
            {},
            {
                PassiveBuff{"fighter_bay_capacity", 2.0},
                PassiveBuff{"defensive_coordination", 0.2}
            }
        },
        {
            "Zenith Collective",
            "Echelon",
            "Superior energy projectors and psionic shielding nodes.",
            {HardpointChange(HardpointCategory::PrimaryWeapon, 2, SlotSize::XXL)},
            {},
            {
                PassiveBuff{"energy_weapon_damage", 0.25},
                PassiveBuff{"psionic_shielding", 1.0}
            }
        },
        {
            "Outer Rim Syndicate",
            "Leviathan",
            "Heavy armor plating and boarding pod launchers.",
            {HardpointChange(HardpointCategory::Utility, 2, SlotSize::XL)},
            {SlotChange(ComponentSlotCategory::Support, 1, SlotSize::XL)},
            {
                PassiveBuff{"armor_thickness", 0.4},
                PassiveBuff{"boarding_efficiency", 1.0}
            }
        }
    };
    entry.progressionMetadata = {15, 50, 10000};
    entry.defaultLoadouts = {
        {
            "Fleet Command Carrier",
            "Flagship configuration for fleet operations and command",
            {"fusion_core_mk2", "main_thruster_freighter", "rcs_cluster_micro", "shield_array_heavy",
             "cargo_rack_standard", "sensor_targeting_mk1", "support_life_pod"}
        }
    };
    return entry;
}

const std::vector<SpaceshipClassCatalogEntry>& BuildCatalog() {
    static const std::vector<SpaceshipClassCatalogEntry> catalog = {
        BuildFighter(),
        BuildFreighter(),
        BuildExplorer(),
        BuildIndustrial(),
        BuildCapital()
    };
    return catalog;
}

} // namespace

const std::vector<SpaceshipClassCatalogEntry>& SpaceshipCatalog::All() {
    return BuildCatalog();
}

const SpaceshipClassCatalogEntry* SpaceshipCatalog::FindById(const std::string& id) {
    const auto& catalog = All();
    auto it = std::find_if(catalog.begin(), catalog.end(), [&](const SpaceshipClassCatalogEntry& entry) {
        return entry.id == id;
    });
    if (it == catalog.end()) {
        return nullptr;
    }
    return &(*it);
}

