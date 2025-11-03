/**
 * Minimal Configuration Management Integration Test
 */
#include <iostream>
#include <string>

// Test ConfigEditor header includes correctly
#include "engine/config/ConfigEditor.h"

int main() {
    std::cout << "Configuration Management System - Integration Test\n";
    std::cout << "===================================================\n\n";
    
    int passed = 0;
    int total = 0;
    
    // Test 1: ConfigEditor instantiation
    std::cout << "Test 1: ConfigEditor instantiation... ";
    try {
        NovaEngine::Config::ConfigEditor editor;
        std::cout << "✓ PASS\n";
        passed++;
    } catch (...) {
        std::cout << "✗ FAIL\n";
    }
    total++;
    
    // Test 2: RealTimeValidator instantiation
    std::cout << "Test 2: RealTimeValidator instantiation... ";
    try {
        NovaEngine::Config::RealTimeValidator validator;
        std::cout << "✓ PASS\n";
        passed++;
    } catch (...) {
        std::cout << "✗ FAIL\n";
    }
    total++;
    
    // Test 3: ConfigTemplateManager singleton
    std::cout << "Test 3: ConfigTemplateManager singleton... ";
    try {
        auto& mgr = NovaEngine::Config::ConfigTemplateManager::GetInstance();
        std::cout << "✓ PASS\n";
        passed++;
    } catch (...) {
        std::cout << "✗ FAIL\n";
    }
    total++;
    
    // Test 4: ConfigTestSuite
    std::cout << "Test 4: ConfigTestSuite instantiation... ";
    try {
        NovaEngine::Config::ConfigTestSuite suite("Test");
        std::cout << "✓ PASS\n";
        passed++;
    } catch (...) {
        std::cout << "✗ FAIL\n";
    }
    total++;
    
    // Test 5: ConfigDeployment singleton
    std::cout << "Test 5: ConfigDeployment singleton... ";
    try {
        auto& deploy = NovaEngine::Config::ConfigDeployment::GetInstance();
        std::cout << "✓ PASS\n";
        passed++;
    } catch (...) {
        std::cout << "✗ FAIL\n";
    }
    total++;
    
    // Summary
    std::cout << "\n===================================================\n";
    std::cout << "Results: " << passed << "/" << total << " tests passed\n";
    std::cout << "Success Rate: " << (passed * 100.0 / total) << "%\n";
    std::cout << "===================================================\n";
    
    return (passed == total) ? 0 : 1;
}
