#include "../engine/ecs/System.h"
#include "../engine/ecs/EntityManager.h"
#include <iostream>
#include <memory>

// Mock system for testing
class MockSystem : public System {
public:
    MockSystem() : updateCount(0) {}
    void Update(EntityManager& entityManager, double dt) override {
        updateCount++;
        lastDt = dt;
    }
    const char* GetName() const override { return "MockSystem"; }
    int updateCount;
    double lastDt;
};

bool TestSystemRegistration() {
    SystemManager manager;
    MockSystem& sys1 = manager.RegisterSystem<MockSystem>();
    MockSystem& sys2 = manager.RegisterSystem<MockSystem>();

    if (&sys1 == &sys2) {
        std::cerr << "RegisterSystem should return different instances" << std::endl;
        return false;
    }

    const auto& metadata = manager.GetRegisteredSystemMetadata();
    if (metadata.size() != 2) {
        std::cerr << "System metadata should contain two entries" << std::endl;
        return false;
    }

    EntityManager em;
    manager.UpdateAll(em, 1.0);

    if (sys1.updateCount != 1 || sys2.updateCount != 1) {
        std::cerr << "Systems should be updated" << std::endl;
        return false;
    }

    if (sys1.lastDt != 1.0 || sys2.lastDt != 1.0) {
        std::cerr << "Systems should receive correct dt" << std::endl;
        return false;
    }

    return true;
}

bool TestSystemClearing() {
    SystemManager manager;
    MockSystem& sys = manager.RegisterSystem<MockSystem>();

    EntityManager em;
    manager.UpdateAll(em, 1.0);
    if (sys.updateCount != 1) {
        std::cerr << "System should be updated before clear" << std::endl;
        return false;
    }

    manager.Clear();

    const auto& metadata = manager.GetRegisteredSystemMetadata();
    if (!metadata.empty()) {
        std::cerr << "Metadata cache should be empty after clear" << std::endl;
        return false;
    }

    manager.UpdateAll(em, 1.0); // Should be a no-op with no systems registered

    return true;
}

int main() {
    if (!TestSystemRegistration()) {
        return 1;
    }

    if (!TestSystemClearing()) {
        return 2;
    }

    std::cout << "SystemManager unit tests passed." << std::endl;
    return 0;
}