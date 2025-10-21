#include "SolarSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifndef USE_GLFW
static constexpr GLenum GL_LINES = 0u;
#endif

#include "CelestialBody.h"
#include "Transform.h"
#include "ecs/Components.h"

namespace {
constexpr double kDegToRad = M_PI / 180.0;
constexpr double kTwoPi = 2.0 * M_PI;
constexpr double kAuToKm = 149597870.7;

constexpr const char* kBodyVertexShaderPath = "shaders/solar_system/body.vert";
constexpr const char* kPlanetFragmentShaderPath = "shaders/solar_system/body.frag";
constexpr const char* kStarFragmentShaderPath = "shaders/solar_system/star.frag";
constexpr const char* kOrbitVertexShaderPath = "shaders/solar_system/orbit.vert";
constexpr const char* kOrbitFragmentShaderPath = "shaders/solar_system/orbit.frag";

Vector3 ApplyOrbitalRotation(double xOrb, double yOrb, const OrbitalComponent& orbit) {
    double cosOmega = std::cos(orbit.longitudeOfAscendingNode * kDegToRad);
    double sinOmega = std::sin(orbit.longitudeOfAscendingNode * kDegToRad);
    double cosInclination = std::cos(orbit.inclination * kDegToRad);
    double sinInclination = std::sin(orbit.inclination * kDegToRad);
    double cosArgPeriapsis = std::cos(orbit.argumentOfPeriapsis * kDegToRad);
    double sinArgPeriapsis = std::sin(orbit.argumentOfPeriapsis * kDegToRad);

    double m11 = cosOmega * cosArgPeriapsis - sinOmega * sinArgPeriapsis * cosInclination;
    double m12 = -cosOmega * sinArgPeriapsis - sinOmega * cosArgPeriapsis * cosInclination;
    double m21 = sinOmega * cosArgPeriapsis + cosOmega * sinArgPeriapsis * cosInclination;
    double m22 = -sinOmega * sinArgPeriapsis + cosOmega * cosArgPeriapsis * cosInclination;
    double m31 = sinArgPeriapsis * sinInclination;
    double m32 = cosArgPeriapsis * sinInclination;

    double x = m11 * xOrb + m12 * yOrb;
    double y = m21 * xOrb + m22 * yOrb;
    double z = m31 * xOrb + m32 * yOrb;
    return Vector3(x, y, z);
}

} // namespace

SolarSystem::SolarSystem()
    : entityManager_(nullptr)
    , systemName_()
    , starEntity_(0)
    , planets_()
    , asteroidBelts_()
    , spaceStations_()
    , planetMoons_()
    , simulationTime_(0.0)
    , orbitalVisualizationEnabled_(false)
    , orbitalVisualizationMesh_()
    , orbitalVisualizationDirty_(false)
    , updateCounter_(0)
    , orbitVertexShaderPath_(kOrbitVertexShaderPath)
    , orbitFragmentShaderPath_(kOrbitFragmentShaderPath) {
}

SolarSystem::~SolarSystem() {
    Clear();
}

void SolarSystem::Init(EntityManager* entityManager, const std::string& systemName) {
    entityManager_ = entityManager;
    systemName_ = systemName;
    planets_.clear();
    asteroidBelts_.clear();
    spaceStations_.clear();
    planetMoons_.clear();
    simulationTime_ = 0.0;
    orbitalVisualizationEnabled_ = false;
    orbitalVisualizationMesh_.Clear();
    orbitalVisualizationDirty_ = false;
    updateCounter_ = 0;
    starEntity_ = 0;
    orbitVertexShaderPath_ = kOrbitVertexShaderPath;
    orbitFragmentShaderPath_ = kOrbitFragmentShaderPath;
}

void SolarSystem::SetStarEntity(Entity starEntity) {
    starEntity_ = starEntity;
    orbitalVisualizationDirty_ = true;
    AssignDefaultShaders(starEntity_);
}

void SolarSystem::Update(double dt, double timeAcceleration) {
    if (!entityManager_) {
        return;
    }

    double scaledDt = dt * timeAcceleration;
    if (scaledDt < 0.0) {
        scaledDt = 0.0;
    }
    simulationTime_ += scaledDt;

    if (UPDATE_FREQUENCY > 1) {
        updateCounter_ = (updateCounter_ + 1) % UPDATE_FREQUENCY;
        if (updateCounter_ != 0) {
            return;
        }
    }

    Vector3 starPosition = GetEntityPosition(starEntity_);

    for (Entity planet : planets_) {
        UpdateBodyHierarchy(planet, starPosition);
    }

    for (Entity belt : asteroidBelts_) {
        UpdateBodyHierarchy(belt, starPosition);
    }

    for (Entity station : spaceStations_) {
        Vector3 parentPosition = starPosition;
        if (auto* orbit = entityManager_->GetComponent<OrbitalComponent>(station)) {
            Entity parent = static_cast<Entity>(orbit->parentEntity);
            if (parent != 0 && parent != starEntity_) {
                parentPosition = GetEntityPosition(parent);
            }
        }
        UpdateBodyHierarchy(station, parentPosition);
    }

    if (orbitalVisualizationEnabled_ && orbitalVisualizationDirty_) {
        BuildOrbitalVisualizationMesh();
    }
}

void SolarSystem::AddPlanet(Entity planetEntity) {
    if (!entityManager_ || !entityManager_->IsAlive(planetEntity)) {
        return;
    }
    if (std::find(planets_.begin(), planets_.end(), planetEntity) == planets_.end()) {
        planets_.push_back(planetEntity);
        planetMoons_.emplace_back(planetEntity, std::vector<Entity>());
        orbitalVisualizationDirty_ = true;
        AssignDefaultShaders(planetEntity);
    }
}

void SolarSystem::AddMoon(Entity planetEntity, Entity moonEntity) {
    if (!entityManager_ || !entityManager_->IsAlive(moonEntity)) {
        return;
    }
    auto it = std::find_if(planetMoons_.begin(), planetMoons_.end(),
        [planetEntity](const auto& entry) { return entry.first == planetEntity; });
    if (it == planetMoons_.end()) {
        planetMoons_.emplace_back(planetEntity, std::vector<Entity>{moonEntity});
        orbitalVisualizationDirty_ = true;
        AssignDefaultShaders(moonEntity);
        return;
    }
    auto& moonList = it->second;
    if (std::find(moonList.begin(), moonList.end(), moonEntity) == moonList.end()) {
        moonList.push_back(moonEntity);
        orbitalVisualizationDirty_ = true;
        AssignDefaultShaders(moonEntity);
    }
}

std::vector<Entity> SolarSystem::GetMoons(Entity planetEntity) const {
    auto it = std::find_if(planetMoons_.begin(), planetMoons_.end(),
        [planetEntity](const auto& entry) { return entry.first == planetEntity; });
    if (it == planetMoons_.end()) {
        return {};
    }
    return it->second;
}

void SolarSystem::AddAsteroidBelt(Entity beltEntity) {
    if (!entityManager_ || !entityManager_->IsAlive(beltEntity)) {
        return;
    }
    if (std::find(asteroidBelts_.begin(), asteroidBelts_.end(), beltEntity) == asteroidBelts_.end()) {
        asteroidBelts_.push_back(beltEntity);
        orbitalVisualizationDirty_ = true;
        AssignDefaultShaders(beltEntity);
    }
}

void SolarSystem::AddSpaceStation(Entity stationEntity) {
    if (!entityManager_ || !entityManager_->IsAlive(stationEntity)) {
        return;
    }
    if (std::find(spaceStations_.begin(), spaceStations_.end(), stationEntity) == spaceStations_.end()) {
        spaceStations_.push_back(stationEntity);
        orbitalVisualizationDirty_ = true;
        AssignDefaultShaders(stationEntity);
    }
}

Entity SolarSystem::FindNearestBody(const Vector3& position, double maxDistance) const {
    if (!entityManager_) {
        return 0;
    }

    double maxDistanceSquared = (maxDistance > 0.0) ? maxDistance * maxDistance : std::numeric_limits<double>::infinity();
    double bestDistanceSquared = std::numeric_limits<double>::infinity();
    Entity bestEntity = 0;

    auto considerEntity = [&](Entity entity) {
        if (!entity || !entityManager_->IsAlive(entity)) {
            return;
        }
        Vector3 entityPosition = GetEntityPosition(entity);
        Vector3 diff = entityPosition - position;
        double distanceSquared = diff.Dot(diff);
        if (distanceSquared <= maxDistanceSquared && distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestEntity = entity;
        }
    };

    considerEntity(starEntity_);
    for (Entity planet : planets_) {
        considerEntity(planet);
        auto it = std::find_if(planetMoons_.begin(), planetMoons_.end(),
            [planet](const auto& entry) { return entry.first == planet; });
        if (it != planetMoons_.end()) {
            for (Entity moon : it->second) {
                considerEntity(moon);
            }
        }
    }
    for (Entity belt : asteroidBelts_) {
        considerEntity(belt);
    }
    for (Entity station : spaceStations_) {
        considerEntity(station);
    }

    return bestEntity;
}

std::vector<Entity> SolarSystem::FindBodiesInRadius(const Vector3& position, double radius) const {
    std::vector<Entity> result;
    if (!entityManager_) {
        return result;
    }

    double radiusSquared = (radius >= 0.0) ? radius * radius : std::numeric_limits<double>::infinity();

    auto tryAdd = [&](Entity entity) {
        if (!entity || !entityManager_->IsAlive(entity)) {
            return;
        }
        Vector3 entityPosition = GetEntityPosition(entity);
        Vector3 diff = entityPosition - position;
        double distanceSquared = diff.Dot(diff);
        if (distanceSquared <= radiusSquared) {
            if (std::find(result.begin(), result.end(), entity) == result.end()) {
                result.push_back(entity);
            }
        }
    };

    tryAdd(starEntity_);
    for (Entity planet : planets_) {
        tryAdd(planet);
        auto it = std::find_if(planetMoons_.begin(), planetMoons_.end(),
            [planet](const auto& entry) { return entry.first == planet; });
        if (it != planetMoons_.end()) {
            for (Entity moon : it->second) {
                tryAdd(moon);
            }
        }
    }
    for (Entity belt : asteroidBelts_) {
        tryAdd(belt);
    }
    for (Entity station : spaceStations_) {
        tryAdd(station);
    }

    return result;
}

void SolarSystem::Clear() {
    planets_.clear();
    asteroidBelts_.clear();
    spaceStations_.clear();
    planetMoons_.clear();
    systemName_.clear();
    starEntity_ = 0;
    simulationTime_ = 0.0;
    orbitalVisualizationEnabled_ = false;
    orbitalVisualizationMesh_.Clear();
    orbitalVisualizationDirty_ = false;
    updateCounter_ = 0;
}

void SolarSystem::SetOrbitalVisualizationEnabled(bool enabled) {
    if (orbitalVisualizationEnabled_ == enabled) {
        return;
    }
    orbitalVisualizationEnabled_ = enabled;
    if (enabled) {
        orbitalVisualizationDirty_ = true;
    } else {
        orbitalVisualizationMesh_.Clear();
        orbitalVisualizationDirty_ = false;
    }
}

OrbitalPosition SolarSystem::CalculateOrbitalPosition(const OrbitalComponent& orbit, const Vector3& parentPosition) const {
    OrbitalPosition result;
    result.isValid = false;

    double periodSeconds = orbit.orbitalPeriod * 86400.0;
    if (periodSeconds <= 0.0) {
        return result;
    }

    double semiMajorAxisKm = orbit.semiMajorAxis;
    Entity parent = static_cast<Entity>(orbit.parentEntity);
    if (parent == 0 || parent == starEntity_) {
        semiMajorAxisKm *= kAuToKm;
    }
    if (semiMajorAxisKm <= 0.0) {
        return result;
    }

    double meanAnomaly = orbit.currentMeanAnomaly * kDegToRad;
    meanAnomaly = std::fmod(meanAnomaly, kTwoPi);
    if (meanAnomaly < 0.0) {
        meanAnomaly += kTwoPi;
    }

    double eccentricAnomaly = SolveKeplersEquation(meanAnomaly, orbit.eccentricity);
    double sinE = std::sin(eccentricAnomaly);
    double cosE = std::cos(eccentricAnomaly);

    double trueAnomaly = std::atan2(std::sqrt(1.0 - orbit.eccentricity * orbit.eccentricity) * sinE,
                                    cosE - orbit.eccentricity);

    double distance = semiMajorAxisKm * (1.0 - orbit.eccentricity * cosE);

    Vector3 localPosition = OrbitalToCartesian(trueAnomaly, distance, orbit);
    Vector3 worldPosition = parentPosition + localPosition;

    double mu = 4.0 * M_PI * M_PI * semiMajorAxisKm * semiMajorAxisKm * semiMajorAxisKm /
                (periodSeconds * periodSeconds);
    double p = semiMajorAxisKm * (1.0 - orbit.eccentricity * orbit.eccentricity);
    double h = (p > 0.0 && mu > 0.0) ? std::sqrt(mu * p) : 0.0;
    double r = distance;
    Vector3 localVelocity;
    if (h > 0.0 && r > 0.0) {
        double vxOrb = -h / r * std::sin(trueAnomaly);
        double vyOrb = h / r * (orbit.eccentricity + std::cos(trueAnomaly));
        localVelocity = ApplyOrbitalRotation(vxOrb, vyOrb, orbit);
    }

    result.position = worldPosition;
    result.velocity = localVelocity;
    result.trueAnomaly = trueAnomaly;
    result.distance = distance;
    result.isValid = true;
    return result;
}

double SolarSystem::SolveKeplersEquation(double meanAnomaly, double eccentricity, double tolerance, int maxIterations) {
    double M = std::fmod(meanAnomaly, kTwoPi);
    if (M < 0.0) {
        M += kTwoPi;
    }

    double E = (eccentricity < 0.8) ? M : (M_PI);
    for (int i = 0; i < maxIterations; ++i) {
        double f = E - eccentricity * std::sin(E) - M;
        double fp = 1.0 - eccentricity * std::cos(E);
        if (std::abs(fp) < 1e-12) {
            break;
        }
        double delta = f / fp;
        E -= delta;
        if (std::abs(delta) < tolerance) {
            break;
        }
    }

    return E;
}

Vector3 SolarSystem::OrbitalToCartesian(double trueAnomaly, double distance, const OrbitalComponent& orbit) {
    double xOrb = distance * std::cos(trueAnomaly);
    double yOrb = distance * std::sin(trueAnomaly);
    return ApplyOrbitalRotation(xOrb, yOrb, orbit);
}

void SolarSystem::UpdateOrbit(Entity entity, OrbitalComponent* orbit, const Vector3& parentPosition) {
    if (!entityManager_ || !orbit) {
        return;
    }

    double elapsed = simulationTime_ - orbit->lastUpdateTime;
    if (elapsed < 0.0) {
        elapsed = 0.0;
    }

    double periodSeconds = orbit->orbitalPeriod * 86400.0;
    double meanMotion = (periodSeconds > 0.0) ? (kTwoPi / periodSeconds) : 0.0;

    double currentMeanAnomaly = orbit->currentMeanAnomaly * kDegToRad;
    currentMeanAnomaly += meanMotion * elapsed;
    currentMeanAnomaly = std::fmod(currentMeanAnomaly, kTwoPi);
    if (currentMeanAnomaly < 0.0) {
        currentMeanAnomaly += kTwoPi;
    }
    orbit->currentMeanAnomaly = currentMeanAnomaly / kDegToRad;
    orbit->lastUpdateTime = simulationTime_;

    OrbitalPosition pos = CalculateOrbitalPosition(*orbit, parentPosition);
    if (!pos.isValid) {
        return;
    }

    Vector3 previousPosition = orbit->cachedPosition;
    orbit->cachedPosition = pos.position;
    if (elapsed > 0.0) {
        orbit->cachedVelocity = (pos.position - previousPosition) * (1.0 / elapsed);
    } else {
        orbit->cachedVelocity = pos.velocity;
    }

    if (auto* position = entityManager_->GetComponent<Position>(entity)) {
        position->x = orbit->cachedPosition.x;
        position->y = orbit->cachedPosition.y;
        position->z = orbit->cachedPosition.z;
    } else if (auto* transform = entityManager_->GetComponent<Transform2D>(entity)) {
        transform->x = orbit->cachedPosition.x;
        transform->y = orbit->cachedPosition.y;
    }
}

void SolarSystem::UpdateBodyHierarchy(Entity entity, const Vector3& parentPosition) {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return;
    }

    Vector3 currentPosition = parentPosition;
    if (auto* orbit = entityManager_->GetComponent<OrbitalComponent>(entity)) {
        UpdateOrbit(entity, orbit, parentPosition);
        currentPosition = orbit->cachedPosition;
    } else {
        currentPosition = GetEntityPosition(entity);
    }

    auto it = std::find_if(planetMoons_.begin(), planetMoons_.end(),
        [entity](const auto& entry) { return entry.first == entity; });
    if (it != planetMoons_.end()) {
        for (Entity moon : it->second) {
            UpdateBodyHierarchy(moon, currentPosition);
        }
    }

    if (auto* satellites = entityManager_->GetComponent<SatelliteSystemComponent>(entity)) {
        for (Entity satellite : satellites->satelliteEntities) {
            UpdateBodyHierarchy(satellite, currentPosition);
        }
    }
}

Vector3 SolarSystem::GetEntityPosition(Entity entity) const {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return Vector3();
    }
    if (auto* position = entityManager_->GetComponent<Position>(entity)) {
        return Vector3(position->x, position->y, position->z);
    }
    if (auto* orbit = entityManager_->GetComponent<OrbitalComponent>(entity)) {
        return orbit->cachedPosition;
    }
    return Vector3();
}

void SolarSystem::BuildOrbitalVisualizationMesh() {
    orbitalVisualizationMesh_.Clear();
    orbitalVisualizationDirty_ = false;

    if (!entityManager_ || !orbitalVisualizationEnabled_) {
        return;
    }

    MeshBuilder builder(GL_LINES);

    Vector3 starPosition = GetEntityPosition(starEntity_);
    std::vector<Entity> visited;
    visited.reserve(planets_.size() + asteroidBelts_.size() + spaceStations_.size());

    for (Entity planet : planets_) {
        BuildOrbitMeshRecursive(planet, starPosition, builder, visited);
    }

    for (Entity belt : asteroidBelts_) {
        BuildOrbitMeshRecursive(belt, starPosition, builder, visited);
    }

    for (Entity station : spaceStations_) {
        Vector3 parentPosition = starPosition;
        if (auto* orbit = entityManager_->GetComponent<OrbitalComponent>(station)) {
            Entity parent = static_cast<Entity>(orbit->parentEntity);
            if (parent != 0) {
                parentPosition = GetEntityPosition(parent);
            }
        }
        BuildOrbitMeshRecursive(station, parentPosition, builder, visited);
    }

    orbitalVisualizationMesh_ = builder.Build(true);
    orbitalVisualizationMesh_.SetAttributes(MeshAttribute_Position | MeshAttribute_Color);
}

void SolarSystem::BuildOrbitMeshRecursive(Entity entity, const Vector3& parentPosition,
    MeshBuilder& builder, std::vector<Entity>& visitedEntities) {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return;
    }
    if (std::find(visitedEntities.begin(), visitedEntities.end(), entity) != visitedEntities.end()) {
        return;
    }
    visitedEntities.push_back(entity);

    Vector3 currentParent = parentPosition;
    if (auto* orbit = entityManager_->GetComponent<OrbitalComponent>(entity)) {
        BuildOrbitPathMesh(entity, *orbit, parentPosition, builder);
        currentParent = orbit->cachedPosition;
    } else {
        currentParent = GetEntityPosition(entity);
    }

    auto moonIt = std::find_if(planetMoons_.begin(), planetMoons_.end(),
        [entity](const auto& entry) { return entry.first == entity; });
    if (moonIt != planetMoons_.end()) {
        for (Entity moon : moonIt->second) {
            BuildOrbitMeshRecursive(moon, currentParent, builder, visitedEntities);
        }
    }

    if (auto* satellites = entityManager_->GetComponent<SatelliteSystemComponent>(entity)) {
        for (unsigned int childId : satellites->satelliteEntities) {
            BuildOrbitMeshRecursive(static_cast<Entity>(childId), currentParent, builder, visitedEntities);
        }
    }
}

void SolarSystem::BuildOrbitPathMesh(Entity entity, const OrbitalComponent& orbit,
    const Vector3& parentPosition, MeshBuilder& builder) {
    if (orbit.orbitalPeriod <= 0.0) {
        return;
    }

    float colorR = 0.6f;
    float colorG = 0.7f;
    float colorB = 1.0f;
    const float colorA = 0.65f;

    if (auto* body = entityManager_->GetComponent<CelestialBodyComponent>(entity)) {
        switch (body->type) {
            case CelestialBodyComponent::BodyType::RockyPlanet:
                colorR = 0.82f; colorG = 0.58f; colorB = 0.36f;
                break;
            case CelestialBodyComponent::BodyType::GasGiant:
                colorR = 0.45f; colorG = 0.70f; colorB = 0.98f;
                break;
            case CelestialBodyComponent::BodyType::IceGiant:
                colorR = 0.55f; colorG = 0.78f; colorB = 1.0f;
                break;
            case CelestialBodyComponent::BodyType::Moon:
                colorR = 0.78f; colorG = 0.78f; colorB = 0.78f;
                break;
            case CelestialBodyComponent::BodyType::Asteroid:
                colorR = 0.62f; colorG = 0.62f; colorB = 0.62f;
                break;
            case CelestialBodyComponent::BodyType::SpaceStation:
                colorR = 1.0f; colorG = 0.95f; colorB = 0.40f;
                break;
            case CelestialBodyComponent::BodyType::AsteroidBelt:
                colorR = 0.90f; colorG = 0.82f; colorB = 0.55f;
                break;
            default:
                break;
        }
    }

    const int segmentCount = 128;
    GLuint baseIndex = static_cast<GLuint>(builder.Vertices().size());
    int validVertices = 0;

    for (int i = 0; i < segmentCount; ++i) {
        double fraction = static_cast<double>(i) / static_cast<double>(segmentCount);
        OrbitalComponent sample = orbit;
        double meanAnomalyDeg = fraction * 360.0;
        sample.currentMeanAnomaly = meanAnomalyDeg;
        sample.meanAnomalyAtEpoch = meanAnomalyDeg;
        OrbitalPosition position = CalculateOrbitalPosition(sample, parentPosition);
        if (!position.isValid) {
            continue;
        }

        builder.AddVertex(static_cast<GLfloat>(position.position.x),
                          static_cast<GLfloat>(position.position.y),
                          static_cast<GLfloat>(position.position.z),
                          colorR, colorG, colorB, colorA);
        ++validVertices;
    }

    GLuint endIndex = static_cast<GLuint>(builder.Vertices().size());
    if (validVertices < 2) {
        builder.Vertices().resize(baseIndex);
        return;
    }

    for (GLuint i = baseIndex; i + 1 < endIndex; ++i) {
        builder.AddLine(i, i + 1);
    }
    builder.AddLine(endIndex - 1, baseIndex);
}

void SolarSystem::AssignDefaultShaders(Entity entity) {
    if (!entityManager_ || !entityManager_->IsAlive(entity)) {
        return;
    }

    auto* visual = entityManager_->GetComponent<VisualCelestialComponent>(entity);
    if (!visual) {
        return;
    }

    auto assignSurfaceShaders = [&](const char* vertexPath, const char* fragmentPath) {
        if (visual->surfaceVertexShader.empty()) {
            visual->surfaceVertexShader = vertexPath;
        }
        if (visual->surfaceFragmentShader.empty()) {
            visual->surfaceFragmentShader = fragmentPath;
        }
    };

    if (auto* body = entityManager_->GetComponent<CelestialBodyComponent>(entity)) {
        if (body->type == CelestialBodyComponent::BodyType::Star) {
            assignSurfaceShaders(kBodyVertexShaderPath, kStarFragmentShaderPath);
        } else {
            assignSurfaceShaders(kBodyVertexShaderPath, kPlanetFragmentShaderPath);
        }
    } else {
        assignSurfaceShaders(kBodyVertexShaderPath, kPlanetFragmentShaderPath);
    }

    if (visual->orbitVertexShader.empty()) {
        visual->orbitVertexShader = orbitVertexShaderPath_;
    }
    if (visual->orbitFragmentShader.empty()) {
        visual->orbitFragmentShader = orbitFragmentShaderPath_;
    }
}
