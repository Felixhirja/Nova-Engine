#include "../engine/LifecycleAnalytics.h"
#include "../engine/LifecycleActor.h"
#include "../engine/IActor.h"
#include "../engine/ecs/EntityManager.h"
#include <thread>
#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

using namespace lifecycle;

class SimpleActor : public IActor {
public:
    SimpleActor(const std::string& name) { name_ = name; }
    void Initialize() override {}
    void Update(double) override {}
    std::string GetName() const override { return name_; }
private:
    std::string name_;
};

int main() {
    std::cout << "=== Lifecycle Analytics Test ===" << std::endl;

    // Initialize lifecycle system (this will also initialize analytics via LifecycleActor)
    lifecycle_utils::InitializeLifecycleSystem();

    EntityManager em;
    std::vector<std::unique_ptr<SimpleActor>> actors;

    // Create multiple actors
    for (int i = 0; i < 10; ++i) {
        auto a = std::make_unique<SimpleActor>("A_" + std::to_string(i));
        Entity e = em.CreateEntity();
        ActorContext ctx{em, e};
        a->AttachContext(ctx);
        ActorLifecycleManager::Instance().RegisterActor(a.get(), &ctx);
        actors.push_back(std::move(a));
    }

    for (int i = 0; i < 5; ++i) {
        auto b = std::make_unique<SimpleActor>("B_" + std::to_string(i));
        Entity e = em.CreateEntity();
        ActorContext ctx{em, e};
        b->AttachContext(ctx);
        ActorLifecycleManager::Instance().RegisterActor(b.get(), &ctx);
        actors.push_back(std::move(b));
    }

    // Transition actors to initialized and active
    for (auto& a : actors) {
        ActorLifecycleManager::Instance().TransitionTo(a.get(), ActorState::Initialized);
        ActorLifecycleManager::Instance().TransitionTo(a.get(), ActorState::Active);
    }

    // Let actors run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Destroy half of them
    for (size_t i = 0; i < actors.size(); ++i) {
        if (i % 2 == 0) {
            ActorLifecycleManager::Instance().UnregisterActor(actors[i].get());
        }
    }

    // Print analytics report
    LifecycleAnalytics::Instance().PrintReport();
    std::cout << LifecycleAnalytics::Instance().ExportJson() << std::endl;

    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
