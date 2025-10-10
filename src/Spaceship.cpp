#include "Spaceship.h"

#include <array>
#include <stdexcept>

namespace {

using Def = SpaceshipClassDefinition;

ComponentSlotSpec Slot(ComponentSlotCategory category, SlotSize size, int count, const std::string& notes) {
    return ComponentSlotSpec{category, size, count, notes};
}

HardpointSpec Hardpoint(HardpointCategory category, SlotSize size, int count, const std::string& notes) {
    return HardpointSpec{category, size, count, notes};
}

ProgressionTier Tier(int tier, const std::string& name, const std::string& description) {
    return ProgressionTier{tier, name, description};
}

FactionVariant Variant(const std::string& faction, const std::string& codename, const std::string& description) {
    return FactionVariant{faction, codename, description};
}

ConceptBrief Brief(const std::string& elevatorPitch, std::initializer_list<std::string> hooks) {
    return ConceptBrief{elevatorPitch, std::vector<std::string>(hooks)};
}

BaselineStats Baseline(float minMass, float maxMass, int minCrew, int maxCrew, float minPower, float maxPower) {
    return BaselineStats{minMass, maxMass, minCrew, maxCrew, minPower, maxPower};
}

const std::vector<SpaceshipClassDefinition>& BuildCatalog() {
    static const std::vector<SpaceshipClassDefinition> catalog = {
        Def{
            SpaceshipClassType::Fighter,
            "Fighter",
            Brief(
                "Agile interception craft built for rapid-response dogfighting.",
                {
                    "High thrust-to-weight ratio enabling extreme acceleration",
                    "Compact profile optimized for carrier deployment",
                    "Limited endurance balanced by modular avionics upgrades"
                }
            ),
            Baseline(25.0f, 35.0f, 1, 2, 8.0f, 12.0f),
            {
                Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Small, 2, "Fixed or gimbaled energy/ballistic cannons"),
                Hardpoint(HardpointCategory::Utility, SlotSize::XS, 1, "Countermeasure pod or sensor jammer"),
                Hardpoint(HardpointCategory::Module, SlotSize::Small, 1, "Avionics suite, stealth package, or auxiliary fuel tank")
            },
            {
                Slot(ComponentSlotCategory::PowerPlant, SlotSize::Small, 1, "Compact fusion core"),
                Slot(ComponentSlotCategory::MainThruster, SlotSize::Small, 1, "Main engine block with afterburner"),
                Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::XS, 4, "Vectored control thrusters"),
                Slot(ComponentSlotCategory::Shield, SlotSize::Small, 1, "Lightweight directional shield generator"),
                Slot(ComponentSlotCategory::Weapon, SlotSize::Small, 2, "Weapon cooling/targeting subsystems"),
                Slot(ComponentSlotCategory::Sensor, SlotSize::Small, 1, "Combat-grade targeting computer"),
                Slot(ComponentSlotCategory::Support, SlotSize::XS, 1, "Emergency life-support capsule")
            },
            {
                Tier(1, "Starter Interceptor", "Entry-level hull unlocked during tutorial arc."),
                Tier(2, "Specialist Interceptor", "Enhanced maneuvering thrusters and avionics."),
                Tier(3, "Elite Strike Fighter", "Modular wing pylons with stealth/strike packages.")
            },
            {
                Variant("Terran Navy", "Raptor", "Balanced stats with missile rack integration."),
                Variant("Outer Rim Syndicate", "Viper", "Sacrifices armor for boosted engines and smuggling compartment."),
                Variant("Zenith Collective", "Aurora", "Energy re-routing module for sustained beam weapons.")
            }
        },
        Def{
            SpaceshipClassType::Freighter,
            "Freighter",
            Brief(
                "Versatile cargo hauler that anchors trade routes and logistics chains.",
                {
                    "Modular container bays and detachable cargo pods",
                    "Reinforced frames for micro-jump stability",
                    "Defensive focus on countermeasures and drone escorts"
                }
            ),
            Baseline(90.0f, 120.0f, 2, 4, 18.0f, 26.0f),
            {
                Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 1, "Defensive turret covering dorsal arc"),
                Hardpoint(HardpointCategory::Utility, SlotSize::Small, 2, "Countermeasures, tractor beam, or repair drone"),
                Hardpoint(HardpointCategory::Module, SlotSize::Medium, 3, "Cargo bay extensions, shield capacitor, drone bay")
            },
            {
                Slot(ComponentSlotCategory::PowerPlant, SlotSize::Medium, 1, "High-endurance reactor core"),
                Slot(ComponentSlotCategory::MainThruster, SlotSize::Medium, 2, "Dual main engines with cargo-tuned exhaust"),
                Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Small, 6, "Station-keeping thruster clusters"),
                Slot(ComponentSlotCategory::Shield, SlotSize::Medium, 1, "Omni-directional cargo shield generator"),
                Slot(ComponentSlotCategory::Cargo, SlotSize::Large, 3, "Container racks or specialized payload modules"),
                Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Small, 1, "Extended crew habitation module"),
                Slot(ComponentSlotCategory::Sensor, SlotSize::Medium, 1, "Logistics-grade navigation array"),
                Slot(ComponentSlotCategory::Support, SlotSize::Medium, 1, "Docking collar or drone control bay")
            },
            {
                Tier(1, "Light Hauler", "Compact freighters for intra-system trade."),
                Tier(2, "Convoy Freighter", "Detachable cargo pods with improved security."),
                Tier(3, "Heavy Transport", "Jump-capable cargo frames with automated loaders.")
            },
            {
                Variant("Terran Commerce Guild", "Atlas", "Security seals and customs compliance modules."),
                Variant("Frontier Miners Union", "Prospector", "Swappable mining rigs and ore refining bay."),
                Variant("Free Traders League", "Nomad", "Expanded crew quarters and smuggling compartments.")
            }
        },
        Def{
            SpaceshipClassType::Explorer,
            "Explorer",
            Brief(
                "Long-range survey vessel outfitted for science expeditions and reconnaissance.",
                {
                    "Extended sensor suites and survey drones",
                    "Hybrid drives enabling atmospheric descent",
                    "Laboratory-grade module capacity"
                }
            ),
            Baseline(80.0f, 95.0f, 3, 5, 16.0f, 22.0f),
            {
                Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 1, "Defensive turret or rail repeater"),
                Hardpoint(HardpointCategory::Utility, SlotSize::Small, 3, "Sensor array, drone control, repair beam"),
                Hardpoint(HardpointCategory::Module, SlotSize::Medium, 3, "Labs, data core, stealth probe bay")
            },
            {
                Slot(ComponentSlotCategory::PowerPlant, SlotSize::Medium, 1, "Efficient long-range reactor"),
                Slot(ComponentSlotCategory::MainThruster, SlotSize::Medium, 1, "Hybrid atmospheric/space engine"),
                Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Small, 6, "Precision RCS clusters"),
                Slot(ComponentSlotCategory::Shield, SlotSize::Medium, 1, "Adaptive shield lattice"),
                Slot(ComponentSlotCategory::Sensor, SlotSize::Large, 2, "Long-range sensor and science array"),
                Slot(ComponentSlotCategory::Support, SlotSize::Medium, 2, "Survey drone racks, repair gantry"),
                Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Small, 1, "Science team habitation pod"),
                Slot(ComponentSlotCategory::Cargo, SlotSize::Medium, 1, "Sample containment hold")
            },
            {
                Tier(1, "Survey Corvette", "Planetary mapping contracts and exploration."),
                Tier(2, "Deep-space Scout", "Long-range jump matrix with cloaked probes."),
                Tier(3, "Expedition Cruiser", "Onboard fabrication and anomaly shielding.")
            },
            {
                Variant("Academy of Sciences", "Odyssey", "Enhanced lab capacity and science buffs."),
                Variant("Free Horizon Cartographers", "Pathfinder", "Jump range bonuses and terrain scanners."),
                Variant("Shadow Consortium", "Phantom", "Sensor-masking systems and covert data vaults.")
            }
        },
        Def{
            SpaceshipClassType::Industrial,
            "Industrial",
            Brief(
                "Heavy utility platform supporting mining, salvage, and construction operations.",
                {
                    "High-capacity power distribution for industrial tools",
                    "Expanded utility slots for drones and fabrication rigs",
                    "Armored hull optimized for hazardous environments"
                }
            ),
            Baseline(140.0f, 180.0f, 4, 6, 24.0f, 34.0f),
            {
                Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::Medium, 2, "Defensive cannons covering broad arcs"),
                Hardpoint(HardpointCategory::Utility, SlotSize::Medium, 2, "Tractor beams, repair projectors"),
                Hardpoint(HardpointCategory::Module, SlotSize::Large, 4, "Mining rigs, fabrication arrays, salvage bay, shield inducers")
            },
            {
                Slot(ComponentSlotCategory::PowerPlant, SlotSize::Large, 1, "Industrial-grade reactor core"),
                Slot(ComponentSlotCategory::MainThruster, SlotSize::Large, 2, "Heavy-duty propulsion blocks"),
                Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Medium, 8, "Directional thruster girdles"),
                Slot(ComponentSlotCategory::Shield, SlotSize::Large, 1, "Reinforced containment shields"),
                Slot(ComponentSlotCategory::Industrial, SlotSize::Large, 4, "Mining lasers, repair gantries, fabrication rigs"),
                Slot(ComponentSlotCategory::Cargo, SlotSize::Large, 2, "Bulk ore hoppers or construction material bins"),
                Slot(ComponentSlotCategory::Support, SlotSize::Medium, 2, "Drone hangars or crane assemblies"),
                Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Medium, 1, "Work crew habitation")
            },
            {
                Tier(1, "Utility Platform", "Salvage and repair missions in low-risk zones."),
                Tier(2, "Deep-core Miner", "Armored drill heads with ore refineries."),
                Tier(3, "Construction Platform", "Deploys outposts and orbital structures.")
            },
            {
                Variant("Union of Labor", "Forge", "Resilient hull with redundant systems."),
                Variant("Corporate Combine", "Constructor", "Advanced fabrication modules and supply bonuses."),
                Variant("Scavenger Clans", "Scrap Queen", "Expanded salvage bays and crane arms.")
            }
        },
        Def{
            SpaceshipClassType::Capital,
            "Capital",
            Brief(
                "Command-and-control flagships capable of projecting force and supporting fleets.",
                {
                    "Multiple subsystem redundancies and distributed crew stations",
                    "Acts as mobile base with hangar capacity",
                    "Hosts advanced command and logistics suites"
                }
            ),
            Baseline(600.0f, 950.0f, 8, 18, 60.0f, 120.0f),
            {
                Hardpoint(HardpointCategory::PrimaryWeapon, SlotSize::XL, 6, "Turrets or beam arrays spanning ship arcs"),
                Hardpoint(HardpointCategory::Utility, SlotSize::Large, 4, "Point-defense grids, sensor masts"),
                Hardpoint(HardpointCategory::Module, SlotSize::XL, 6, "Hangars, shield amplifiers, command modules, medical bays")
            },
            {
                Slot(ComponentSlotCategory::PowerPlant, SlotSize::XL, 2, "Redundant flagship cores"),
                Slot(ComponentSlotCategory::MainThruster, SlotSize::XL, 4, "Capital propulsion arrays"),
                Slot(ComponentSlotCategory::ManeuverThruster, SlotSize::Large, 12, "Distributed RCS banks"),
                Slot(ComponentSlotCategory::Shield, SlotSize::XL, 2, "Layered shield projectors"),
                Slot(ComponentSlotCategory::Hangar, SlotSize::XL, 2, "Strike craft or shuttle hangars"),
                Slot(ComponentSlotCategory::Support, SlotSize::Large, 4, "Command, medical, fabrication suites"),
                Slot(ComponentSlotCategory::Sensor, SlotSize::Large, 2, "Long-range tactical sensor masts"),
                Slot(ComponentSlotCategory::CrewQuarters, SlotSize::Large, 3, "Distributed crew habitats"),
                Slot(ComponentSlotCategory::Industrial, SlotSize::Large, 1, "Fleet support fabrication plant")
            },
            {
                Tier(2, "Escort Carrier", "Accessible via faction reputation milestones."),
                Tier(3, "Battlecruiser", "Command dreadnought unlocked in endgame campaigns."),
                Tier(4, "Legendary Flagship", "Narrative-locked capital hull with unique bonuses.")
            },
            {
                Variant("Terran Navy", "Resolute", "Balanced defenses with fighter bay bonuses."),
                Variant("Zenith Collective", "Echelon", "Superior energy projectors and psionic shielding."),
                Variant("Outer Rim Syndicate", "Leviathan", "Heavy armor plating and boarding pods.")
            }
        }
    };

    return catalog;
}

} // namespace

const SpaceshipClassDefinition& SpaceshipCatalog::GetDefinition(SpaceshipClassType type) {
    const auto& catalog = BuildCatalog();
    for (const auto& def : catalog) {
        if (def.type == type) {
            return def;
        }
    }
    throw std::out_of_range("SpaceshipCatalog::GetDefinition - unknown type");
}

const std::vector<SpaceshipClassDefinition>& SpaceshipCatalog::All() {
    return BuildCatalog();
}

std::string ToString(SpaceshipClassType type) {
    switch (type) {
        case SpaceshipClassType::Fighter: return "Fighter";
        case SpaceshipClassType::Freighter: return "Freighter";
        case SpaceshipClassType::Explorer: return "Explorer";
        case SpaceshipClassType::Industrial: return "Industrial";
        case SpaceshipClassType::Capital: return "Capital";
    }
    return "Unknown";
}

std::string ToString(HardpointCategory category) {
    switch (category) {
        case HardpointCategory::PrimaryWeapon: return "PrimaryWeapon";
        case HardpointCategory::Utility: return "Utility";
        case HardpointCategory::Module: return "Module";
    }
    return "Unknown";
}

std::string ToString(ComponentSlotCategory category) {
    switch (category) {
        case ComponentSlotCategory::PowerPlant: return "PowerPlant";
        case ComponentSlotCategory::MainThruster: return "MainThruster";
        case ComponentSlotCategory::ManeuverThruster: return "ManeuverThruster";
        case ComponentSlotCategory::Shield: return "Shield";
        case ComponentSlotCategory::Weapon: return "Weapon";
        case ComponentSlotCategory::Cargo: return "Cargo";
        case ComponentSlotCategory::CrewQuarters: return "CrewQuarters";
        case ComponentSlotCategory::Sensor: return "Sensor";
        case ComponentSlotCategory::Industrial: return "Industrial";
        case ComponentSlotCategory::Support: return "Support";
        case ComponentSlotCategory::Hangar: return "Hangar";
        case ComponentSlotCategory::Computer: return "Computer";
    }
    return "Unknown";
}

std::string ToString(SlotSize size) {
    switch (size) {
        case SlotSize::XS: return "XS";
        case SlotSize::Small: return "Small";
        case SlotSize::Medium: return "Medium";
        case SlotSize::Large: return "Large";
        case SlotSize::XL: return "XL";
        case SlotSize::XXL: return "XXL";
    }
    return "Unknown";
}
