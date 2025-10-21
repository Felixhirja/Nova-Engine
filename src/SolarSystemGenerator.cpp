#include "SolarSystemGenerator.h"

#include <array>

namespace {

constexpr std::uint32_t kStarSalt = 0x53544152;     // 'STAR'
constexpr std::uint32_t kPlanetSalt = 0x504c414e;   // 'PLAN'
constexpr std::uint32_t kMoonSalt = 0x4d4f4f4e;     // 'MOON'
constexpr std::uint32_t kAsteroidSalt = 0x41535452; // 'ASTR'
constexpr std::uint32_t kStationSalt = 0x53544154;  // 'STAT'
constexpr std::uint32_t kNameSalt = 0x4e414d45;     // 'NAME'

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
