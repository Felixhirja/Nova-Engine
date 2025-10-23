#include "SolarSystemGenerator.h"

// Define M_PI if not available (Windows)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <numeric>
#include <sstream>
#include <tuple>

#include "ecs/Components.h"

namespace {

constexpr double kSolarMassKg = 1.98847e30;
constexpr double kSolarRadiusKm = 695700.0;
constexpr double kEarthMassKg = 5.972e24;
constexpr double kEarthRadiusKm = 6371.0;
constexpr double kAstronomicalUnitKm = 149597870.7;
constexpr double kGravitationalConstantKm = 6.67430e-20; // km^3 kg^-1 s^-2

constexpr std::array<const char*, 8> kSystemPrefixes = {
    "Kepler", "Nova", "Aurora", "Vega", "Orion", "Helios", "Atlas", "Seren"
};

const std::array<Vector3, 7> kStarColors = {
    Vector3(0.6, 0.7, 1.0),
    Vector3(0.7, 0.8, 1.0),
    Vector3(0.8, 0.85, 1.0),
    Vector3(1.0, 0.95, 0.85),
    Vector3(1.0, 0.9, 0.7),
    Vector3(1.0, 0.75, 0.5),
    Vector3(1.0, 0.55, 0.4)
};

std::string ToRomanNumeral(int value) {
    if (value <= 0) {
        return std::to_string(value);
    }

    struct Numeral { int value; const char* symbol; };
    static constexpr std::array<Numeral, 13> numerals = {{
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
        {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}
    }};

    std::string result;
    for (const auto& numeral : numerals) {
        while (value >= numeral.value) {
            result += numeral.symbol;
            value -= numeral.value;
        }
    }
    return result;
}

constexpr std::uint32_t kStarSalt = 0x53544152;     // 'STAR'
constexpr std::uint32_t kPlanetSalt = 0x504c414e;   // 'PLAN'
constexpr std::uint32_t kMoonSalt = 0x4d4f4f4e;     // 'MOON'
constexpr std::uint32_t kAsteroidSalt = 0x41535452; // 'ASTR'
constexpr std::uint32_t kStationSalt = 0x53544154;  // 'STAT'
constexpr std::uint32_t kNameSalt = 0x4e414d45;     // 'NAME'

constexpr double kDefaultSystemInnerOrbit = 0.35; // AU
constexpr double kDefaultSystemSpacing = 1.65;
constexpr double kSnowLineCoefficient = 2.7; // AU * sqrt(luminosity)

} // namespace

SolarSystemGenerator::SolarSystemGenerator()
    : currentSystemName_(), currentSeed_(0), seeds_() {}

SolarSystemGenerator::~SolarSystemGenerator() = default;

void SolarSystemGenerator::SetSeed(unsigned int seed) {
    currentSeed_ = seed;
    InitializeSeedState(seed);
}

unsigned int SolarSystemGenerator::GetSeed(SeedType type, unsigned int index) const {
    unsigned int categorySeed = 0;
    switch (type) {
        case SeedType::Star:
            categorySeed = seeds_.starSeed;
            break;
        case SeedType::Planet:
            categorySeed = seeds_.planetSeed;
            break;
        case SeedType::Moon:
            categorySeed = seeds_.moonSeed;
            break;
        case SeedType::Asteroid:
            categorySeed = seeds_.asteroidSeed;
            break;
        case SeedType::Station:
            categorySeed = seeds_.stationSeed;
            break;
        case SeedType::Name:
            categorySeed = seeds_.namingSeed;
            break;
        default:
            categorySeed = seeds_.baseSeed;
            break;
    }

    if (categorySeed == 0) {
        categorySeed = CombineSeed(currentSeed_, static_cast<std::uint32_t>(type));
    }

    if (index == 0) {
        return categorySeed;
    }

    return CombineSeedWithIndex(categorySeed, index);
}

std::mt19937 SolarSystemGenerator::CreateRng(SeedType type, unsigned int index) const {
    return std::mt19937(GetSeed(type, index));
}

void SolarSystemGenerator::InitializeSeedState(unsigned int seed) {
    seeds_.baseSeed = seed;
    seeds_.starSeed = CombineSeed(seed, kStarSalt);
    seeds_.planetSeed = CombineSeed(seed, kPlanetSalt);
    seeds_.moonSeed = CombineSeed(seed, kMoonSalt);
    seeds_.asteroidSeed = CombineSeed(seed, kAsteroidSalt);
    seeds_.stationSeed = CombineSeed(seed, kStationSalt);
    seeds_.namingSeed = CombineSeed(seed, kNameSalt);
}

unsigned int SolarSystemGenerator::CombineSeed(unsigned int seed, std::uint32_t salt) {
    std::uint64_t value = (static_cast<std::uint64_t>(seed) << 32) ^ salt;
    value += 0x9e3779b97f4a7c15ull;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
    value ^= (value >> 31);
    return static_cast<unsigned int>(value & 0xffffffffu);
}

unsigned int SolarSystemGenerator::CombineSeedWithIndex(unsigned int seed, unsigned int index) {
    std::uint32_t salt = static_cast<std::uint32_t>(index + 1) * 0x9e3779b9u;
    return CombineSeed(seed, salt);
}

Entity SolarSystemGenerator::GenerateSystem(
    EntityManager* entityManager,
    unsigned int seed,
    const GenerationParameters& params) {
    if (!entityManager) {
        return 0;
    }

    SetSeed(seed);
    currentSeed_ = seed;

    std::mt19937 nameRng = CreateRng(SeedType::Name);
    std::uniform_int_distribution<std::size_t> prefixDist(0, kSystemPrefixes.size() - 1);
    std::uniform_int_distribution<int> numberDist(100, 999);
    std::ostringstream systemNameBuilder;
    systemNameBuilder << kSystemPrefixes[prefixDist(nameRng)] << "-" << numberDist(nameRng);
    currentSystemName_ = systemNameBuilder.str();

    std::mt19937 starRng = CreateRng(SeedType::Star);
    Entity starEntity = GenerateStar(entityManager, starRng);
    if (!starEntity) {
        return 0;
    }

    auto* starComponent = entityManager->GetComponent<StarComponent>(starEntity);
    if (!starComponent) {
        return starEntity;
    }

    std::mt19937 planetRng = CreateRng(SeedType::Planet);
    std::vector<Entity> planets = GeneratePlanets(
        entityManager,
        starEntity,
        *starComponent,
        planetRng,
        params);

    std::vector<double> planetOrbits;
    planetOrbits.reserve(planets.size());
    for (Entity planet : planets) {
        if (auto* orbit = entityManager->GetComponent<OrbitalComponent>(planet)) {
            planetOrbits.push_back(orbit->semiMajorAxis);
        }
    }

    std::mt19937 asteroidRng = CreateRng(SeedType::Asteroid);
    GenerateAsteroidBelts(entityManager, starEntity, planetOrbits, asteroidRng);

    std::mt19937 stationRng = CreateRng(SeedType::Station);
    GenerateSpaceStations(entityManager, starEntity, planets, stationRng, params);

    return starEntity;
}

Entity SolarSystemGenerator::GenerateStar(EntityManager* entityManager, std::mt19937& rng) {
    if (!entityManager) {
        return 0;
    }

    Entity starEntity = entityManager->CreateEntity();

    StarComponent::SpectralType spectralType = GenerateSpectralType(rng);
    std::uniform_int_distribution<int> subclassDist(0, 9);
    int subclass = subclassDist(rng);

    double massSolar = 1.0;
    double radiusKm = kSolarRadiusKm;
    double temperature = 5778.0;
    double luminosity = 1.0;
    CalculateStarProperties(spectralType, subclass, massSolar, radiusKm, temperature, luminosity);

    auto& body = entityManager->EmplaceComponent<CelestialBodyComponent>(starEntity);
    body.type = CelestialBodyComponent::BodyType::Star;
    body.mass = massSolar * kSolarMassKg;
    body.radius = radiusKm;
    body.temperature = temperature;
    body.rotationPeriod = RandomDouble(rng, 20.0, 45.0);
    body.axialTilt = RandomDouble(rng, 0.0, 7.0);
    body.hasAtmosphere = false;
    body.hasMagneticField = true;
    body.isLandable = false;
    body.isDockable = false;
    body.name = GenerateName(currentSystemName_, CelestialBodyComponent::BodyType::Star, 0);

    auto& star = entityManager->EmplaceComponent<StarComponent>(starEntity);
    star.spectralType = spectralType;
    star.spectralSubclass = subclass;
    star.luminosity = luminosity;
    star.surfaceTemperature = temperature;
    CalculateHabitableZone(luminosity, star.habitableZoneInner, star.habitableZoneOuter);
    star.coronaSize = static_cast<float>(RandomDouble(rng, 1.2, 2.0));
    star.hasFlares = RandomBool(rng, 0.6f);
    star.flareIntensity = RandomFloat(rng, 0.2f, 1.0f);

    auto& visual = entityManager->EmplaceComponent<VisualCelestialComponent>(starEntity);
    VisualCelestialComponent starVisual = GenerateVisualProperties(CelestialBodyComponent::BodyType::Star, rng);
    visual = starVisual;
    visual.emissive = 1.0f;
    visual.colorR = static_cast<float>(kStarColors[static_cast<int>(spectralType)].x);
    visual.colorG = static_cast<float>(kStarColors[static_cast<int>(spectralType)].y);
    visual.colorB = static_cast<float>(kStarColors[static_cast<int>(spectralType)].z);

    entityManager->EmplaceComponent<Position>(starEntity);

    auto& velocity = entityManager->EmplaceComponent<Velocity>(starEntity);
    velocity.vx = 0.0;
    velocity.vy = 0.0;
    velocity.vz = 0.0;

    auto& nameComponent = entityManager->EmplaceComponent<Name>(starEntity);
    nameComponent.value = body.name;

    return starEntity;
}

std::vector<Entity> SolarSystemGenerator::GeneratePlanets(
    EntityManager* entityManager,
    Entity starEntity,
    const StarComponent& starComponent,
    std::mt19937& rng,
    const GenerationParameters& params) {
    std::vector<Entity> planets;
    if (!entityManager) {
        return planets;
    }

    int minPlanets = std::max(0, params.minPlanets);
    int maxPlanets = std::max(minPlanets, params.maxPlanets);
    std::uniform_int_distribution<int> planetCountDist(minPlanets, maxPlanets);
    int planetCount = planetCountDist(rng);
    if (planetCount <= 0) {
        return planets;
    }

    CelestialBodyComponent* starBody = entityManager->GetComponent<CelestialBodyComponent>(starEntity);
    double starMassSolar = starBody ? starBody->mass / kSolarMassKg : 1.0;

    std::vector<double> orbitalDistances = GenerateOrbitalDistances(planetCount, starMassSolar, rng);
    double innerHabitable = 0.0;
    double outerHabitable = 0.0;
    CalculateHabitableZone(starComponent.luminosity, innerHabitable, outerHabitable);

    planets.reserve(static_cast<std::size_t>(planetCount));

    for (int i = 0; i < planetCount; ++i) {
        double distanceAU = orbitalDistances[static_cast<std::size_t>(i)];
        CelestialBodyComponent::BodyType type = DeterminePlanetType(distanceAU, starComponent.luminosity, rng);

        CelestialBodyComponent bodyProperties;
        switch (type) {
            case CelestialBodyComponent::BodyType::GasGiant:
                bodyProperties = GenerateGasGiantProperties(distanceAU, rng);
                break;
            case CelestialBodyComponent::BodyType::IceGiant:
                bodyProperties = GenerateIceGiantProperties(distanceAU, rng);
                break;
            default:
                bodyProperties = GenerateRockyPlanetProperties(distanceAU, starComponent.luminosity, rng);
                break;
        }
        bodyProperties.type = type;
        bodyProperties.name = GenerateName(currentSystemName_, type, i + 1);

        bool isHabitable = (type == CelestialBodyComponent::BodyType::RockyPlanet) &&
            IsInHabitableZone(distanceAU, innerHabitable, outerHabitable) &&
            bodyProperties.hasAtmosphere &&
            bodyProperties.temperature >= 250.0 && bodyProperties.temperature <= 320.0;
        bodyProperties.isHabitable = isHabitable;
        bodyProperties.isLandable = (type != CelestialBodyComponent::BodyType::GasGiant &&
                                     type != CelestialBodyComponent::BodyType::IceGiant);

        Entity planetEntity = entityManager->CreateEntity();
        auto& bodyComponent = entityManager->EmplaceComponent<CelestialBodyComponent>(planetEntity);
        bodyComponent = bodyProperties;

        auto& orbit = entityManager->EmplaceComponent<OrbitalComponent>(planetEntity);
        OrbitalComponent generatedOrbit = GenerateOrbitalParameters(
            distanceAU,
            std::max(starMassSolar, 0.1),
            distanceAU < kSnowLineCoefficient,
            rng);
        generatedOrbit.parentEntity = static_cast<unsigned int>(starEntity);
        orbit = generatedOrbit;

        auto& position = entityManager->EmplaceComponent<Position>(planetEntity);
        position.x = position.y = position.z = 0.0;

        auto& velocity = entityManager->EmplaceComponent<Velocity>(planetEntity);
        velocity.vx = 0.0;
        velocity.vy = 0.0;
        velocity.vz = 0.0;

        auto& planetComponent = entityManager->EmplaceComponent<PlanetComponent>(planetEntity);
        double gravity = (bodyComponent.radius > 0.0)
            ? (bodyComponent.mass / kEarthMassKg) * 9.81 *
                std::pow(kEarthRadiusKm / bodyComponent.radius, 2.0)
            : 9.81;
        planetComponent.gravity = static_cast<float>(gravity);
        planetComponent.hasOceans = bodyComponent.hasAtmosphere && bodyComponent.isHabitable;
        planetComponent.oceanCoverage = planetComponent.hasOceans ? RandomFloat(rng, 0.4f, 0.8f) : 0.0f;
        planetComponent.hasIceCaps = RandomBool(rng, bodyComponent.temperature < 240.0 ? 0.8f : 0.3f);
        planetComponent.iceCoverage = planetComponent.hasIceCaps ? RandomFloat(rng, 0.1f, 0.5f) : 0.0f;
        planetComponent.hasLife = bodyComponent.isHabitable && RandomBool(rng, 0.4f);
        planetComponent.hasIntelligentLife = planetComponent.hasLife && RandomBool(rng, 0.05f);
        planetComponent.biodiversityIndex = planetComponent.hasLife ? RandomFloat(rng, 0.3f, 0.9f) : 0.0f;
        planetComponent.mineralWealth = RandomFloat(rng, 0.2f, 0.9f);
        planetComponent.organicResources = planetComponent.hasLife ? RandomFloat(rng, 0.4f, 0.9f)
                                                                  : RandomFloat(rng, 0.0f, 0.3f);

        auto& visual = entityManager->EmplaceComponent<VisualCelestialComponent>(planetEntity);
        visual = GenerateVisualProperties(type, rng);
        if (bodyComponent.hasRings) {
            visual.ringInnerRadius = static_cast<float>(bodyComponent.radius * RandomDouble(rng, 1.7, 2.2));
            visual.ringOuterRadius = static_cast<float>(bodyComponent.radius * RandomDouble(rng, 2.3, 3.5));
            visual.ringOpacity = RandomFloat(rng, 0.4f, 0.9f);
        }

        auto& nameComponent = entityManager->EmplaceComponent<Name>(planetEntity);
        nameComponent.value = bodyComponent.name;

        if (params.generateMoons) {
            std::mt19937 moonRng = CreateRng(SeedType::Moon, static_cast<unsigned int>(i));
            GenerateMoons(entityManager, planetEntity, bodyComponent, moonRng);
        }

        planets.push_back(planetEntity);
    }

    return planets;
}

std::vector<Entity> SolarSystemGenerator::GenerateMoons(
    EntityManager* entityManager,
    Entity planetEntity,
    const CelestialBodyComponent& planetBody,
    std::mt19937& rng) {
    std::vector<Entity> moons;
    if (!entityManager) {
        return moons;
    }

    auto* planetOrbit = entityManager->GetComponent<OrbitalComponent>(planetEntity);
    if (!planetOrbit) {
        return moons;
    }

    Entity parentStar = static_cast<Entity>(planetOrbit->parentEntity);
    auto* starBody = entityManager->GetComponent<CelestialBodyComponent>(parentStar);
    double starMassKg = starBody ? starBody->mass : kSolarMassKg;

    int moonCount = DetermineMoonCount(planetBody.type, planetBody.mass, rng);
    if (moonCount <= 0) {
        return moons;
    }

    auto& satelliteSystem = entityManager->HasComponent<SatelliteSystemComponent>(planetEntity)
        ? *entityManager->GetComponent<SatelliteSystemComponent>(planetEntity)
        : entityManager->EmplaceComponent<SatelliteSystemComponent>(planetEntity);

    double hillSphereKm = CalculateHillSphere(planetBody.mass, planetOrbit->semiMajorAxis, starMassKg);
    double maxMoonOrbit = hillSphereKm * 0.5;
    double minMoonOrbit = std::max(planetBody.radius * 3.0, hillSphereKm * 0.05);
    if (maxMoonOrbit <= minMoonOrbit) {
        return moons;
    }

    for (int i = 0; i < moonCount; ++i) {
        Entity moonEntity = entityManager->CreateEntity();

        auto& moonBody = entityManager->EmplaceComponent<CelestialBodyComponent>(moonEntity);
        moonBody.type = CelestialBodyComponent::BodyType::Moon;
        moonBody.mass = RandomDouble(rng, 7.3e21, 1.0e23);
        moonBody.radius = RandomDouble(rng, 800.0, 3500.0);
        moonBody.rotationPeriod = RandomDouble(rng, 12.0, 80.0);
        moonBody.axialTilt = RandomDouble(rng, 0.0, 10.0);
        moonBody.temperature = planetBody.temperature * RandomDouble(rng, 0.6, 1.1);
        moonBody.hasAtmosphere = RandomBool(rng, 0.2f);
        moonBody.atmosphereDensity = moonBody.hasAtmosphere ? RandomDouble(rng, 0.01, 0.5) : 0.0;
        moonBody.isLandable = true;
        moonBody.isDockable = false;
        moonBody.name = GenerateName(currentSystemName_, CelestialBodyComponent::BodyType::Moon, i, planetBody.name);

        auto& orbit = entityManager->EmplaceComponent<OrbitalComponent>(moonEntity);
        orbit.parentEntity = static_cast<unsigned int>(planetEntity);
        orbit.semiMajorAxis = RandomDouble(rng, minMoonOrbit, maxMoonOrbit);
        orbit.eccentricity = RandomDouble(rng, 0.0, 0.05);
        orbit.inclination = RandomDouble(rng, 0.0, 7.0);
        orbit.longitudeOfAscendingNode = RandomDouble(rng, 0.0, 360.0);
        orbit.argumentOfPeriapsis = RandomDouble(rng, 0.0, 360.0);
        orbit.meanAnomalyAtEpoch = RandomDouble(rng, 0.0, 360.0);
        orbit.currentMeanAnomaly = orbit.meanAnomalyAtEpoch;
        double semiMajorAxisKm = orbit.semiMajorAxis;
        double mu = kGravitationalConstantKm * (planetBody.mass + moonBody.mass);
        double periodSeconds = (semiMajorAxisKm > 0.0 && mu > 0.0)
            ? (2.0 * M_PI * std::sqrt(std::pow(semiMajorAxisKm, 3.0) / mu))
            : 0.0;
        orbit.orbitalPeriod = (periodSeconds > 0.0) ? (periodSeconds / 86400.0) : RandomDouble(rng, 5.0, 40.0);

        auto& visual = entityManager->EmplaceComponent<VisualCelestialComponent>(moonEntity);
        visual = GenerateVisualProperties(CelestialBodyComponent::BodyType::Moon, rng);

        entityManager->EmplaceComponent<Position>(moonEntity);

        auto& velocity = entityManager->EmplaceComponent<Velocity>(moonEntity);
        velocity.vx = 0.0;
        velocity.vy = 0.0;
        velocity.vz = 0.0;

        auto& nameComponent = entityManager->EmplaceComponent<Name>(moonEntity);
        nameComponent.value = moonBody.name;

        satelliteSystem.satelliteEntities.push_back(static_cast<unsigned int>(moonEntity));
        satelliteSystem.moonCount = static_cast<int>(satelliteSystem.satelliteEntities.size());

        moons.push_back(moonEntity);
    }

    return moons;
}

std::vector<Entity> SolarSystemGenerator::GenerateAsteroidBelts(
    EntityManager* entityManager,
    Entity starEntity,
    const std::vector<double>& planetOrbits,
    std::mt19937& rng) {
    std::vector<Entity> belts;
    if (!entityManager) {
        return belts;
    }

    double innerRadius = 0.0;
    double outerRadius = 0.0;
    if (!FindAsteroidBeltLocation(planetOrbits, rng, innerRadius, outerRadius)) {
        return belts;
    }

    Entity beltEntity = entityManager->CreateEntity();
    auto& body = entityManager->EmplaceComponent<CelestialBodyComponent>(beltEntity);
    body.type = CelestialBodyComponent::BodyType::AsteroidBelt;
    body.mass = RandomDouble(rng, 5e20, 5e21);
    body.radius = 0.5 * (innerRadius + outerRadius) * kAstronomicalUnitKm;
    body.name = GenerateName(currentSystemName_, CelestialBodyComponent::BodyType::AsteroidBelt, 1);
    body.isLandable = false;
    body.isDockable = false;

    auto& belt = entityManager->EmplaceComponent<AsteroidBeltComponent>(beltEntity);
    belt.innerRadius = innerRadius;
    belt.outerRadius = outerRadius;
    belt.thickness = RandomDouble(rng, 0.2, 0.6);
    belt.density = static_cast<AsteroidBeltComponent::DensityLevel>(
        RandomInt(rng, 0, static_cast<int>(AsteroidBeltComponent::DensityLevel::VeryDense)));
    belt.composition = static_cast<AsteroidBeltComponent::CompositionType>(
        RandomInt(rng, 0, static_cast<int>(AsteroidBeltComponent::CompositionType::Mixed)));
    belt.asteroidCount = RandomInt(rng, 500, 5000);
    belt.resourceRichness = RandomFloat(rng, 0.2f, 0.9f);

    auto* starBody = entityManager->GetComponent<CelestialBodyComponent>(starEntity);
    double starMassSolar = starBody ? (starBody->mass / kSolarMassKg) : 1.0;

    auto& orbit = entityManager->EmplaceComponent<OrbitalComponent>(beltEntity);
    orbit.parentEntity = static_cast<unsigned int>(starEntity);
    orbit.semiMajorAxis = 0.5 * (innerRadius + outerRadius);
    orbit.eccentricity = RandomDouble(rng, 0.0, 0.05);
    orbit.inclination = RandomDouble(rng, 0.0, 2.0);
    orbit.longitudeOfAscendingNode = RandomDouble(rng, 0.0, 360.0);
    orbit.argumentOfPeriapsis = RandomDouble(rng, 0.0, 360.0);
    orbit.meanAnomalyAtEpoch = RandomDouble(rng, 0.0, 360.0);
    orbit.currentMeanAnomaly = orbit.meanAnomalyAtEpoch;
    double periodYears = std::sqrt(std::pow(orbit.semiMajorAxis, 3.0) / std::max(starMassSolar, 0.1));
    orbit.orbitalPeriod = periodYears * 365.25;

    auto& visual = entityManager->EmplaceComponent<VisualCelestialComponent>(beltEntity);
    visual = GenerateVisualProperties(CelestialBodyComponent::BodyType::AsteroidBelt, rng);

    auto& nameComponent = entityManager->EmplaceComponent<Name>(beltEntity);
    nameComponent.value = body.name;

    belts.push_back(beltEntity);
    return belts;
}

std::vector<Entity> SolarSystemGenerator::GenerateSpaceStations(
    EntityManager* entityManager,
    Entity starEntity,
    const std::vector<Entity>& planets,
    std::mt19937& rng,
    const GenerationParameters& params) {
    std::vector<Entity> stations;
    if (!entityManager) {
        return stations;
    }

    if (planets.empty()) {
        return stations;
    }

    (void)starEntity;

    int minStations = std::max(0, params.minStations);
    int maxStations = std::max(minStations, params.maxStations);
    std::uniform_int_distribution<int> stationCountDist(minStations, maxStations);
    int stationCount = stationCountDist(rng);
    if (stationCount <= 0) {
        return stations;
    }

    stations.reserve(static_cast<std::size_t>(stationCount));
    auto asteroidBelts = entityManager->GetAllWith<AsteroidBeltComponent>();

    for (int i = 0; i < stationCount; ++i) {
        Entity planetEntity = planets[static_cast<std::size_t>(RandomInt(rng, 0, static_cast<int>(planets.size()) - 1))];
        auto* planetBody = entityManager->GetComponent<CelestialBodyComponent>(planetEntity);
        auto* planetOrbit = entityManager->GetComponent<OrbitalComponent>(planetEntity);
        if (!planetBody || !planetOrbit) {
            continue;
        }

        Entity stationEntity = entityManager->CreateEntity();

        auto& body = entityManager->EmplaceComponent<CelestialBodyComponent>(stationEntity);
        body.type = CelestialBodyComponent::BodyType::SpaceStation;
        body.mass = RandomDouble(rng, 5e9, 5e10);
        body.radius = RandomDouble(rng, 2.0, 10.0);
        body.rotationPeriod = RandomDouble(rng, 12.0, 48.0);
        body.isDockable = true;
        body.isLandable = true;
        body.hasAtmosphere = false;
        body.name = GenerateName(currentSystemName_, CelestialBodyComponent::BodyType::SpaceStation, stations.size() + 1,
                                 planetBody->name);

        bool nearHabitable = planetBody->isHabitable;
        bool nearGasGiant = planetBody->type == CelestialBodyComponent::BodyType::GasGiant;
        bool nearAsteroidBelt = false;
        if (!asteroidBelts.empty()) {
            for (const auto& beltEntry : asteroidBelts) {
                const auto* beltComponent = beltEntry.second;
                if (!beltComponent) {
                    continue;
                }
                if (planetOrbit->semiMajorAxis >= beltComponent->innerRadius - 0.2 &&
                    planetOrbit->semiMajorAxis <= beltComponent->outerRadius + 0.2) {
                    nearAsteroidBelt = true;
                    break;
                }
            }
        }
        auto stationType = ChooseStationType(nearHabitable, nearAsteroidBelt, nearGasGiant, rng);

        auto& stationComponent = entityManager->EmplaceComponent<SpaceStationComponent>(stationEntity);
        stationComponent.stationType = stationType;
        stationComponent.population = RandomInt(rng, 500, 50000);
        stationComponent.maxPopulation = std::max(stationComponent.population, RandomInt(rng, 5000, 120000));
        stationComponent.hasShipyard = (stationType == SpaceStationComponent::StationType::Shipyard) || RandomBool(rng, 0.2f);
        stationComponent.hasRepairFacility = stationComponent.stationType != SpaceStationComponent::StationType::Research;
        stationComponent.hasRefuelStation = true;
        stationComponent.hasMarket = (stationType == SpaceStationComponent::StationType::Trading) || RandomBool(rng, 0.4f);
        stationComponent.wealthLevel = RandomInt(rng, 1, 5);
        stationComponent.dockingPorts = RandomInt(rng, 4, 20);

        auto& orbit = entityManager->EmplaceComponent<OrbitalComponent>(stationEntity);
        orbit.parentEntity = static_cast<unsigned int>(planetEntity);
        double stationAltitude = std::max(planetBody->radius * 2.0, RandomDouble(rng, 5000.0, 50000.0));
        orbit.semiMajorAxis = stationAltitude;
        orbit.eccentricity = RandomDouble(rng, 0.0, 0.02);
        orbit.inclination = RandomDouble(rng, 0.0, 5.0);
        orbit.longitudeOfAscendingNode = RandomDouble(rng, 0.0, 360.0);
        orbit.argumentOfPeriapsis = RandomDouble(rng, 0.0, 360.0);
        orbit.meanAnomalyAtEpoch = RandomDouble(rng, 0.0, 360.0);
        orbit.currentMeanAnomaly = orbit.meanAnomalyAtEpoch;
        double mu = kGravitationalConstantKm * planetBody->mass;
        double periodSeconds = (mu > 0.0)
            ? (2.0 * M_PI * std::sqrt(std::pow(stationAltitude, 3.0) / mu))
            : RandomDouble(rng, 6000.0, 20000.0);
        orbit.orbitalPeriod = periodSeconds / 86400.0;

        auto& visual = entityManager->EmplaceComponent<VisualCelestialComponent>(stationEntity);
        visual = GenerateVisualProperties(CelestialBodyComponent::BodyType::SpaceStation, rng);
        visual.metallic = 0.8f;
        visual.roughness = 0.3f;

        entityManager->EmplaceComponent<Position>(stationEntity);

        auto& velocity = entityManager->EmplaceComponent<Velocity>(stationEntity);
        velocity.vx = 0.0;
        velocity.vy = 0.0;
        velocity.vz = 0.0;

        auto& nameComponent = entityManager->EmplaceComponent<Name>(stationEntity);
        nameComponent.value = body.name;

        auto& satellites = entityManager->HasComponent<SatelliteSystemComponent>(planetEntity)
            ? *entityManager->GetComponent<SatelliteSystemComponent>(planetEntity)
            : entityManager->EmplaceComponent<SatelliteSystemComponent>(planetEntity);
        satellites.satelliteEntities.push_back(static_cast<unsigned int>(stationEntity));
        ++satellites.stationCount;

        stations.push_back(stationEntity);
    }

    return stations;
}

std::string SolarSystemGenerator::GenerateName(
    const std::string& systemName,
    CelestialBodyComponent::BodyType bodyType,
    int index,
    const std::string& parentName) {
    switch (bodyType) {
        case CelestialBodyComponent::BodyType::Star:
            return systemName.empty() ? "Unnamed Star" : systemName;
        case CelestialBodyComponent::BodyType::RockyPlanet:
        case CelestialBodyComponent::BodyType::GasGiant:
        case CelestialBodyComponent::BodyType::IceGiant:
            return systemName + " " + ToRomanNumeral(std::max(index, 1));
        case CelestialBodyComponent::BodyType::Moon: {
            char suffix = static_cast<char>('a' + (index % 26));
            std::string base = parentName.empty() ? systemName : parentName;
            return base + " " + std::string(1, suffix);
        }
        case CelestialBodyComponent::BodyType::AsteroidBelt:
            return systemName + " Belt" + (index > 1 ? " " + ToRomanNumeral(index) : "");
        case CelestialBodyComponent::BodyType::SpaceStation:
            return (parentName.empty() ? systemName : parentName) + " Station " + ToRomanNumeral(std::max(index, 1));
        case CelestialBodyComponent::BodyType::Asteroid:
        default:
            return systemName + " Object " + ToRomanNumeral(std::max(index, 1));
    }
}

void SolarSystemGenerator::CalculateHabitableZone(double luminosity, double& innerBound, double& outerBound) {
    double rootLuminosity = std::sqrt(std::max(luminosity, 0.0001));
    innerBound = 0.95 * rootLuminosity;
    outerBound = 1.37 * rootLuminosity;
}

bool SolarSystemGenerator::IsInHabitableZone(double semiMajorAxis, double innerBound, double outerBound) {
    return semiMajorAxis >= innerBound && semiMajorAxis <= outerBound;
}

StarComponent::SpectralType SolarSystemGenerator::GenerateSpectralType(std::mt19937& rng) {
    std::discrete_distribution<int> distribution({1, 3, 5, 10, 25, 30, 26});
    return static_cast<StarComponent::SpectralType>(distribution(rng));
}

void SolarSystemGenerator::CalculateStarProperties(
    StarComponent::SpectralType type,
    int subclass,
    double& mass,
    double& radius,
    double& temperature,
    double& luminosity) {
    struct StarRange {
        double massMin;
        double massMax;
        double radiusMin;
        double radiusMax;
        double temperatureMin;
        double temperatureMax;
        double luminosityMin;
        double luminosityMax;
    };

    static constexpr std::array<StarRange, 7> kRanges = {{
        {16.0, 50.0, 6.0, 10.0, 30000.0, 50000.0, 30000.0, 1000000.0}, // O
        {2.1, 16.0, 3.0, 6.0, 10000.0, 30000.0, 25.0, 30000.0},        // B
        {1.4, 2.1, 1.7, 3.0, 7500.0, 10000.0, 5.0, 25.0},              // A
        {1.04, 1.4, 1.2, 1.6, 6000.0, 7500.0, 1.5, 5.0},               // F
        {0.8, 1.04, 0.9, 1.2, 5200.0, 6000.0, 0.6, 1.5},               // G
        {0.45, 0.8, 0.7, 0.9, 3700.0, 5200.0, 0.1, 0.6},               // K
        {0.08, 0.45, 0.2, 0.7, 2400.0, 3700.0, 0.01, 0.1}              // M
    }};

    const StarRange& range = kRanges[static_cast<int>(type)];
    double t = std::clamp(subclass / 9.0, 0.0, 1.0);

    auto lerp = [](double a, double b, double factor) {
        return a + (b - a) * factor;
    };

    mass = lerp(range.massMax, range.massMin, t);
    radius = lerp(range.radiusMax, range.radiusMin, t) * kSolarRadiusKm;
    temperature = lerp(range.temperatureMax, range.temperatureMin, t);
    luminosity = lerp(range.luminosityMax, range.luminosityMin, t);
}

std::vector<double> SolarSystemGenerator::GenerateOrbitalDistances(int planetCount, double starMass, std::mt19937& rng) {
    std::vector<double> distances;
    if (planetCount <= 0) {
        return distances;
    }

    distances.reserve(static_cast<std::size_t>(planetCount));
    double baseDistance = RandomDouble(rng, kDefaultSystemInnerOrbit * 0.8, kDefaultSystemInnerOrbit * 1.2);
    double spacing = RandomDouble(rng, kDefaultSystemSpacing * 0.85, kDefaultSystemSpacing * 1.15);

    double currentDistance = baseDistance / std::cbrt(std::max(starMass, 0.1));
    for (int i = 0; i < planetCount; ++i) {
        double jitter = RandomDouble(rng, 0.9, 1.1);
        distances.push_back(currentDistance * jitter);
        currentDistance *= spacing * RandomDouble(rng, 0.95, 1.05);
    }

    std::sort(distances.begin(), distances.end());
    return distances;
}

CelestialBodyComponent::BodyType SolarSystemGenerator::DeterminePlanetType(
    double distanceAU,
    double starLuminosity,
    std::mt19937& rng) {
    double snowLine = kSnowLineCoefficient * std::sqrt(std::max(starLuminosity, 0.01));

    if (distanceAU < snowLine * 0.5) {
        return CelestialBodyComponent::BodyType::RockyPlanet;
    }

    if (distanceAU > snowLine * 1.5) {
        return RandomBool(rng, 0.5f)
            ? CelestialBodyComponent::BodyType::GasGiant
            : CelestialBodyComponent::BodyType::IceGiant;
    }

    if (RandomBool(rng, 0.6f)) {
        return CelestialBodyComponent::BodyType::GasGiant;
    }
    return CelestialBodyComponent::BodyType::RockyPlanet;
}

CelestialBodyComponent SolarSystemGenerator::GenerateRockyPlanetProperties(
    double distanceAU,
    double starLuminosity,
    std::mt19937& rng) {
    CelestialBodyComponent body;
    body.type = CelestialBodyComponent::BodyType::RockyPlanet;
    double massEarths = RandomDouble(rng, 0.2, 6.0);
    double radiusEarths = RandomDouble(rng, 0.5, 1.8);
    body.mass = massEarths * kEarthMassKg;
    body.radius = radiusEarths * kEarthRadiusKm;
    body.rotationPeriod = RandomDouble(rng, 12.0, 72.0);
    body.axialTilt = RandomDouble(rng, 0.0, 35.0);
    double equilibriumTemp = 278.0 * std::pow(std::max(starLuminosity, 0.01), 0.25) / std::sqrt(std::max(distanceAU, 0.1));
    body.temperature = equilibriumTemp;
    body.hasAtmosphere = RandomBool(rng, static_cast<float>(std::clamp(massEarths / 5.0, 0.1, 0.95)));
    body.atmosphereDensity = body.hasAtmosphere ? RandomDouble(rng, 0.3, 5.0) : 0.0;
    body.hasMagneticField = RandomBool(rng, 0.6f);
    body.hasRings = false;
    body.isLandable = true;
    body.isDockable = false;
    return body;
}

CelestialBodyComponent SolarSystemGenerator::GenerateGasGiantProperties(double distanceAU, std::mt19937& rng) {
    (void)distanceAU;
    CelestialBodyComponent body;
    body.type = CelestialBodyComponent::BodyType::GasGiant;
    double massJupiters = RandomDouble(rng, 0.5, 10.0);
    body.mass = massJupiters * 1.898e27;
    body.radius = RandomDouble(rng, 50000.0, 90000.0);
    body.rotationPeriod = RandomDouble(rng, 7.0, 20.0);
    body.axialTilt = RandomDouble(rng, 0.0, 30.0);
    body.temperature = RandomDouble(rng, 100.0, 500.0);
    body.hasAtmosphere = true;
    body.atmosphereDensity = RandomDouble(rng, 0.1, 2.0);
    body.hasRings = RandomBool(rng, 0.35f);
    body.hasMagneticField = true;
    body.isLandable = false;
    body.isDockable = false;
    return body;
}

CelestialBodyComponent SolarSystemGenerator::GenerateIceGiantProperties(double distanceAU, std::mt19937& rng) {
    (void)distanceAU;
    CelestialBodyComponent body;
    body.type = CelestialBodyComponent::BodyType::IceGiant;
    double massNeptunes = RandomDouble(rng, 0.5, 3.0);
    body.mass = massNeptunes * 1.024e26;
    body.radius = RandomDouble(rng, 20000.0, 35000.0);
    body.rotationPeriod = RandomDouble(rng, 12.0, 30.0);
    body.axialTilt = RandomDouble(rng, 10.0, 35.0);
    body.temperature = RandomDouble(rng, 40.0, 120.0);
    body.hasAtmosphere = true;
    body.atmosphereDensity = RandomDouble(rng, 0.05, 0.5);
    body.hasRings = RandomBool(rng, 0.25f);
    body.hasMagneticField = true;
    body.isLandable = false;
    body.isDockable = false;
    return body;
}

OrbitalComponent SolarSystemGenerator::GenerateOrbitalParameters(
    double semiMajorAxis,
    double parentMass,
    bool isInner,
    std::mt19937& rng) {
    OrbitalComponent orbit;
    orbit.semiMajorAxis = semiMajorAxis;
    orbit.eccentricity = RandomDouble(rng, 0.0, isInner ? 0.08 : 0.18);
    orbit.inclination = RandomDouble(rng, 0.0, isInner ? 5.0 : 12.0);
    orbit.longitudeOfAscendingNode = RandomDouble(rng, 0.0, 360.0);
    orbit.argumentOfPeriapsis = RandomDouble(rng, 0.0, 360.0);
    orbit.meanAnomalyAtEpoch = RandomDouble(rng, 0.0, 360.0);
    orbit.currentMeanAnomaly = orbit.meanAnomalyAtEpoch;
    double periodYears = std::sqrt(std::pow(semiMajorAxis, 3.0) / std::max(parentMass, 0.1));
    orbit.orbitalPeriod = periodYears * 365.25;
    orbit.lastUpdateTime = 0.0;
    return orbit;
}

VisualCelestialComponent SolarSystemGenerator::GenerateVisualProperties(
    CelestialBodyComponent::BodyType bodyType,
    std::mt19937& rng) {
    VisualCelestialComponent visual;
    switch (bodyType) {
        case CelestialBodyComponent::BodyType::Star: {
            visual.emissive = 1.0f;
            visual.specular = 0.0f;
            visual.roughness = 0.1f;
            visual.metallic = 0.0f;
            visual.colorR = RandomFloat(rng, 0.9f, 1.0f);
            visual.colorG = RandomFloat(rng, 0.7f, 1.0f);
            visual.colorB = RandomFloat(rng, 0.5f, 1.0f);
            break;
        }
        case CelestialBodyComponent::BodyType::RockyPlanet:
            GenerateRandomColor(rng, visual.colorR, visual.colorG, visual.colorB, 0.3f, 0.8f);
            visual.specular = RandomFloat(rng, 0.1f, 0.4f);
            visual.roughness = RandomFloat(rng, 0.4f, 0.8f);
            break;
        case CelestialBodyComponent::BodyType::GasGiant:
            GenerateRandomColor(rng, visual.colorR, visual.colorG, visual.colorB, 0.5f, 1.0f);
            visual.specular = RandomFloat(rng, 0.2f, 0.6f);
            visual.roughness = RandomFloat(rng, 0.2f, 0.5f);
            break;
        case CelestialBodyComponent::BodyType::IceGiant:
            visual.colorR = RandomFloat(rng, 0.5f, 0.7f);
            visual.colorG = RandomFloat(rng, 0.6f, 0.9f);
            visual.colorB = RandomFloat(rng, 0.8f, 1.0f);
            visual.specular = 0.4f;
            visual.roughness = 0.3f;
            break;
        case CelestialBodyComponent::BodyType::Moon:
            visual.colorR = RandomFloat(rng, 0.6f, 0.8f);
            visual.colorG = RandomFloat(rng, 0.6f, 0.8f);
            visual.colorB = RandomFloat(rng, 0.6f, 0.8f);
            visual.specular = 0.1f;
            visual.roughness = 0.9f;
            break;
        case CelestialBodyComponent::BodyType::Asteroid:
        case CelestialBodyComponent::BodyType::AsteroidBelt:
            visual.colorR = RandomFloat(rng, 0.5f, 0.7f);
            visual.colorG = RandomFloat(rng, 0.45f, 0.6f);
            visual.colorB = RandomFloat(rng, 0.35f, 0.5f);
            visual.specular = 0.05f;
            visual.roughness = 0.95f;
            break;
        case CelestialBodyComponent::BodyType::SpaceStation:
            visual.colorR = RandomFloat(rng, 0.7f, 0.9f);
            visual.colorG = RandomFloat(rng, 0.7f, 0.9f);
            visual.colorB = RandomFloat(rng, 0.7f, 0.9f);
            visual.specular = 0.6f;
            visual.roughness = 0.3f;
            visual.metallic = 0.7f;
            break;
    }
    return visual;
}

int SolarSystemGenerator::DetermineMoonCount(
    CelestialBodyComponent::BodyType planetType,
    double planetMass,
    std::mt19937& rng) {
    switch (planetType) {
        case CelestialBodyComponent::BodyType::GasGiant:
            return RandomInt(rng, 4, 12);
        case CelestialBodyComponent::BodyType::IceGiant:
            return RandomInt(rng, 2, 8);
        case CelestialBodyComponent::BodyType::RockyPlanet: {
            double massEarths = planetMass / kEarthMassKg;
            if (massEarths < 0.5) {
                return RandomBool(rng, 0.5f) ? 0 : 1;
            }
            return RandomInt(rng, 0, 2);
        }
        default:
            return 0;
    }
}

double SolarSystemGenerator::CalculateHillSphere(double planetMass, double semiMajorAxis, double starMass) {
    double semiMajorAxisKm = semiMajorAxis * kAstronomicalUnitKm;
    if (semiMajorAxisKm <= 0.0 || starMass <= 0.0) {
        return 0.0;
    }
    return semiMajorAxisKm * std::cbrt(planetMass / (3.0 * starMass));
}

bool SolarSystemGenerator::FindAsteroidBeltLocation(
    const std::vector<double>& planetOrbits,
    std::mt19937& rng,
    double& innerRadius,
    double& outerRadius) {
    if (planetOrbits.size() < 2) {
        double base = RandomDouble(rng, 2.0, 4.0);
        innerRadius = base * 0.9;
        outerRadius = base * 1.1;
        return true;
    }

    double bestGap = 0.0;
    double bestInner = 0.0;
    double bestOuter = 0.0;
    for (std::size_t i = 0; i + 1 < planetOrbits.size(); ++i) {
        double inner = planetOrbits[i];
        double outer = planetOrbits[i + 1];
        double gap = outer - inner;
        if (gap > bestGap) {
            bestGap = gap;
            double padding = gap * 0.3;
            bestInner = inner + padding;
            bestOuter = outer - padding;
        }
    }

    if (bestGap < 0.3 || bestOuter <= bestInner) {
        return false;
    }

    double jitter = RandomDouble(rng, 0.9, 1.1);
    innerRadius = bestInner * jitter;
    outerRadius = bestOuter * jitter;
    if (outerRadius <= innerRadius) {
        std::swap(innerRadius, outerRadius);
    }
    return true;
}

SpaceStationComponent::StationType SolarSystemGenerator::ChooseStationType(
    bool nearHabitableZone,
    bool nearAsteroidBelt,
    bool nearGasGiant,
    std::mt19937& rng) {
    if (nearAsteroidBelt && RandomBool(rng, 0.6f)) {
        return SpaceStationComponent::StationType::Mining;
    }
    if (nearGasGiant && RandomBool(rng, 0.4f)) {
        return SpaceStationComponent::StationType::Research;
    }
    if (nearHabitableZone && RandomBool(rng, 0.5f)) {
        return SpaceStationComponent::StationType::Residential;
    }

    std::uniform_int_distribution<int> dist(0, 5);
    return static_cast<SpaceStationComponent::StationType>(dist(rng));
}

float SolarSystemGenerator::RandomFloat(std::mt19937& rng, float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

double SolarSystemGenerator::RandomDouble(std::mt19937& rng, double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

int SolarSystemGenerator::RandomInt(std::mt19937& rng, int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

bool SolarSystemGenerator::RandomBool(std::mt19937& rng, float probability) {
    std::bernoulli_distribution dist(std::clamp(probability, 0.0f, 1.0f));
    return dist(rng);
}

void SolarSystemGenerator::GenerateRandomColor(std::mt19937& rng, float& r, float& g, float& b,
                                              float minBrightness, float maxBrightness) {
    std::uniform_real_distribution<float> dist(minBrightness, maxBrightness);
    r = dist(rng);
    g = dist(rng);
    b = dist(rng);

    float maxComponent = std::max({r, g, b, 1e-3f});
    float scale = std::clamp(maxBrightness / maxComponent, 0.5f, 1.0f);
    r *= scale;
    g *= scale;
    b *= scale;
}
