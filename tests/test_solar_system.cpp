#include "../engine/SolarSystem.h"
#include "../engine/CelestialBody.h"
#include "../engine/ecs/EntityManager.h"
#include "../engine/ecs/Components.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

double SolveByBisection(double meanAnomaly, double eccentricity) {
    double a = 0.0;
    double b = 2.0 * M_PI;
    double fa = a - eccentricity * std::sin(a) - meanAnomaly;
    for (int i = 0; i < 60; ++i) {
        double mid = 0.5 * (a + b);
        double fm = mid - eccentricity * std::sin(mid) - meanAnomaly;
        if (fa * fm < 0.0) {
            b = mid;
        } else {
            a = mid;
            fa = fm;
        }
    }
    return 0.5 * (a + b);
}

} // namespace

int main() {
    {
        double meanAnomaly = 1.2; // radians
        double eccentricity = 0.45;
        double expected = SolveByBisection(meanAnomaly, eccentricity);
        double result = SolarSystem::SolveKeplersEquation(meanAnomaly, eccentricity, 1e-10, 64);
        assert(std::abs(result - expected) < 1e-6);
    }

    EntityManager entityManager;
    SolarSystem solarSystem;
    solarSystem.Init(&entityManager, "Test System");

    Entity star = entityManager.CreateEntity();
    entityManager.EmplaceComponent<CelestialBodyComponent>(star);
    entityManager.EmplaceComponent<StarComponent>(star);
    auto& starPosition = entityManager.EmplaceComponent<Position>(star);
    starPosition.x = 0.0;
    starPosition.y = 0.0;
    starPosition.z = 0.0;
    solarSystem.SetStarEntity(star);

    Entity planet = entityManager.CreateEntity();
    auto& orbit = entityManager.EmplaceComponent<OrbitalComponent>(planet);
    orbit.parentEntity = star;
    orbit.semiMajorAxis = 1.0; // AU
    orbit.eccentricity = 0.05;
    orbit.inclination = 2.0;
    orbit.longitudeOfAscendingNode = 30.0;
    orbit.argumentOfPeriapsis = 45.0;
    orbit.orbitalPeriod = 365.25;
    orbit.meanAnomalyAtEpoch = 0.0;
    orbit.currentMeanAnomaly = 0.0;
    entityManager.EmplaceComponent<CelestialBodyComponent>(planet);
    entityManager.EmplaceComponent<Position>(planet);
    solarSystem.AddPlanet(planet);

    Entity moon = entityManager.CreateEntity();
    auto& moonOrbit = entityManager.EmplaceComponent<OrbitalComponent>(moon);
    moonOrbit.parentEntity = planet;
    moonOrbit.semiMajorAxis = 384400.0; // km
    moonOrbit.eccentricity = 0.02;
    moonOrbit.orbitalPeriod = 27.3;
    moonOrbit.currentMeanAnomaly = 180.0;
    entityManager.EmplaceComponent<CelestialBodyComponent>(moon);
    entityManager.EmplaceComponent<Position>(moon);
    solarSystem.AddMoon(planet, moon);

    solarSystem.Update(3600.0, 1.0); // advance one hour

    auto* updatedOrbit = entityManager.GetComponent<OrbitalComponent>(planet);
    assert(updatedOrbit);
    assert(updatedOrbit->cachedPosition.Length() > 1e6); // roughly planetary scale in km

    auto moons = solarSystem.GetMoons(planet);
    assert(moons.size() == 1 && moons.front() == moon);

    Vector3 farPoint(2.0 * 149597870.7, 0.0, 0.0);
    Entity nearest = solarSystem.FindNearestBody(farPoint, -1.0);
    assert(nearest == planet || nearest == star);

    Vector3 planetPosition = solarSystem.GetPlanets().empty() ? Vector3() : updatedOrbit->cachedPosition;
    auto nearby = solarSystem.FindBodiesInRadius(planetPosition, 500000.0);
    assert(!nearby.empty());

    std::cout << "Solar system tests passed" << std::endl;
    return 0;
}
