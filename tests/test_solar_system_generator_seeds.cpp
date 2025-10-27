#include "../engine/SolarSystemGenerator.h"

#include <cassert>

int main() {
    SolarSystemGenerator generator;
    generator.SetSeed(1337u);

    const GenerationSeeds& seeds = generator.GetSeeds();
    assert(seeds.baseSeed == 1337u);
    assert(seeds.starSeed != 0u);
    assert(seeds.planetSeed != seeds.starSeed);
    assert(seeds.moonSeed != seeds.planetSeed);

    unsigned int firstPlanet = generator.GetSeed(SolarSystemGenerator::SeedType::Planet, 0);
    unsigned int secondPlanet = generator.GetSeed(SolarSystemGenerator::SeedType::Planet, 1);
    assert(firstPlanet != secondPlanet);

    unsigned int firstMoon = generator.GetSeed(SolarSystemGenerator::SeedType::Moon, 0);
    unsigned int secondMoon = generator.GetSeed(SolarSystemGenerator::SeedType::Moon, 1);
    assert(firstMoon != secondMoon);

    SolarSystemGenerator generatorCopy;
    generatorCopy.SetSeed(1337u);
    assert(generatorCopy.GetSeed(SolarSystemGenerator::SeedType::Planet, 0) == firstPlanet);
    assert(generatorCopy.GetSeed(SolarSystemGenerator::SeedType::Moon, 1) == secondMoon);

    std::mt19937 rngA = generator.CreateRng(SolarSystemGenerator::SeedType::Name);
    std::mt19937 rngB = generatorCopy.CreateRng(SolarSystemGenerator::SeedType::Name);
    assert(rngA() == rngB());

    return 0;
}
