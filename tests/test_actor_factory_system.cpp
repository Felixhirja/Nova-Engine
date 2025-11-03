#include "engine/ActorFactorySystem.h"
#include "engine/ecs/EntityManager.h"
#include "engine/IActor.h"
#include <iostream>
#include <cassert>

// Test actor classes
class TestActor : public IActor {
public:
    void Initialize() override {
        std::cout << "  TestActor initialized" << std::endl;
    }
    
    std::string GetName() const override {
        return "TestActor";
    }
};

class TestActorWithDeps : public IActor {
public:
    void Initialize() override {
        std::cout << "  TestActorWithDeps initialized" << std::endl;
    }
    
    std::string GetName() const override {
        return "TestActorWithDeps";
    }
};

class FailingActor : public IActor {
public:
    void Initialize() override {
        throw std::runtime_error("Intentional failure for testing");
    }
    
    std::string GetName() const override {
        return "FailingActor";
    }
};

void TestBasicRegistration() {
    std::cout << "\n=== Test: Basic Registration ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Register a simple factory
    factory.RegisterFactory("TestActor", []() -> std::unique_ptr<IActor> {
        return std::make_unique<TestActor>();
    });
    
    // Verify registration
    assert(factory.HasFactory("TestActor"));
    std::cout << "PASS: Basic registration" << std::endl;
}

void TestFactoryValidation() {
    std::cout << "\n=== Test: Factory Validation ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Register factory with dependencies
    factory.RegisterFactory("TestActorWithDeps", 
        []() -> std::unique_ptr<IActor> {
            return std::make_unique<TestActorWithDeps>();
        },
        "test",
        {"TestActor"}
    );
    
    // Validate factory
    std::string errorMsg;
    bool valid = factory.ValidateFactory("TestActorWithDeps", errorMsg);
    
    assert(valid);
    std::cout << "PASS: Factory validation with dependencies" << std::endl;
    
    // Test validation of missing factory
    valid = factory.ValidateFactory("NonExistent", errorMsg);
    assert(!valid);
    std::cout << "PASS: Invalid factory detection" << std::endl;
}

void TestActorCreation() {
    std::cout << "\n=== Test: Actor Creation ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    EntityManager entityManager;
    
    Entity entity = entityManager.CreateEntity();
    
    // Create actor
    auto result = factory.CreateActor("TestActor", entityManager, entity);
    
    assert(result.success);
    assert(result.actor != nullptr);
    assert(result.creationTimeMs >= 0.0);
    
    std::cout << "PASS: Actor creation (time: " << result.creationTimeMs << " ms)" << std::endl;
}

void TestFailedCreation() {
    std::cout << "\n=== Test: Failed Creation ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Register failing factory
    factory.RegisterFactory("FailingActor", []() -> std::unique_ptr<IActor> {
        return std::make_unique<FailingActor>();
    });
    
    EntityManager entityManager;
    Entity entity = entityManager.CreateEntity();
    
    // Attempt creation (should fail during Initialize)
    auto result = factory.CreateActor("FailingActor", entityManager, entity);
    
    assert(!result.success);
    assert(!result.errorMessage.empty());
    
    std::cout << "PASS: Failed creation handled correctly" << std::endl;
    std::cout << "  Error: " << result.errorMessage << std::endl;
}

void TestTemplateSystem() {
    std::cout << "\n=== Test: Template System ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Register template
    factory.RegisterTemplate("FastTestActor", "TestActor", {
        {"speed", "fast"},
        {"color", "red"}
    });
    
    EntityManager entityManager;
    Entity entity = entityManager.CreateEntity();
    
    // Create from template
    auto result = factory.CreateFromTemplate("FastTestActor", entityManager, entity);
    
    assert(result.success);
    std::cout << "PASS: Template creation" << std::endl;
}

void TestPerformanceMetrics() {
    std::cout << "\n=== Test: Performance Metrics ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    EntityManager entityManager;
    
    // Create multiple actors
    for (int i = 0; i < 10; ++i) {
        Entity entity = entityManager.CreateEntity();
        factory.CreateActor("TestActor", entityManager, entity);
    }
    
    // Get metrics
    auto metrics = factory.GetPerformanceMetrics();
    
    assert(metrics.totalCreations >= 10);
    assert(metrics.avgTimeMs >= 0.0);
    assert(metrics.minTimeMs <= metrics.maxTimeMs);
    
    std::cout << "PASS: Performance metrics collected" << std::endl;
    std::cout << "  Total creations: " << metrics.totalCreations << std::endl;
    std::cout << "  Avg time: " << metrics.avgTimeMs << " ms" << std::endl;
    std::cout << "  Min/Max: " << metrics.minTimeMs << "/" << metrics.maxTimeMs << " ms" << std::endl;
}

void TestAnalytics() {
    std::cout << "\n=== Test: Analytics ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Get most used types
    auto mostUsed = factory.GetMostUsedActorTypes(5);
    
    assert(!mostUsed.empty());
    std::cout << "PASS: Analytics - most used types:" << std::endl;
    for (const auto& type : mostUsed) {
        const auto& meta = factory.GetFactoryMetadata(type);
        std::cout << "  " << type << ": " << meta.creationCount << " creations" << std::endl;
    }
}

void TestDebugMode() {
    std::cout << "\n=== Test: Debug Mode ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Enable debug mode
    factory.EnableDebugMode(true);
    
    // Create actor with debug output
    EntityManager entityManager;
    Entity entity = entityManager.CreateEntity();
    factory.CreateActor("TestActor", entityManager, entity);
    
    // Disable debug mode
    factory.EnableDebugMode(false);
    
    std::cout << "PASS: Debug mode" << std::endl;
}

void TestFactoryTesting() {
    std::cout << "\n=== Test: Factory Testing ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Test individual factory
    std::string result;
    bool success = factory.TestFactory("TestActor", result);
    
    assert(success);
    std::cout << "PASS: Individual factory test" << std::endl;
    std::cout << result;
    
    // Test all factories
    auto results = factory.TestAllFactories();
    assert(!results.empty());
    
    std::cout << "PASS: All factories tested" << std::endl;
}

void TestDocumentation() {
    std::cout << "\n=== Test: Documentation Generation ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Generate documentation
    std::string docs = factory.GenerateDocumentation();
    
    assert(!docs.empty());
    assert(docs.find("# Actor Factory System Documentation") != std::string::npos);
    
    std::cout << "PASS: Documentation generated" << std::endl;
    
    // Export documentation
    factory.ExportDocumentation("actor_factory_docs.md");
    std::cout << "PASS: Documentation exported" << std::endl;
}

void TestHealthReport() {
    std::cout << "\n=== Test: Health Report ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    std::string report = factory.GetFactoryHealthReport();
    
    assert(!report.empty());
    std::cout << report << std::endl;
    std::cout << "PASS: Health report generated" << std::endl;
}

void TestCategoryQuerying() {
    std::cout << "\n=== Test: Category Querying ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Get all registered types
    auto allTypes = factory.GetRegisteredTypes();
    std::cout << "Total registered types: " << allTypes.size() << std::endl;
    
    // Get types by category
    auto testTypes = factory.GetFactoriesByCategory("test");
    std::cout << "Test category types: " << testTypes.size() << std::endl;
    
    std::cout << "PASS: Category querying" << std::endl;
}

void TestFactoryLogging() {
    std::cout << "\n=== Test: Factory Logging ===" << std::endl;
    
    auto& factory = ActorFactorySystem::GetInstance();
    
    // Log individual factory state
    factory.LogFactoryState("TestActor");
    
    // Log all factories
    factory.LogAllFactories();
    
    std::cout << "PASS: Factory logging" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "Actor Factory System Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        TestBasicRegistration();
        TestFactoryValidation();
        TestActorCreation();
        TestFailedCreation();
        TestTemplateSystem();
        TestPerformanceMetrics();
        TestAnalytics();
        TestDebugMode();
        TestFactoryTesting();
        TestDocumentation();
        TestHealthReport();
        TestCategoryQuerying();
        TestFactoryLogging();
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "ALL TESTS PASSED!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=====================================" << std::endl;
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        std::cerr << "=====================================" << std::endl;
        return 1;
    }
}
