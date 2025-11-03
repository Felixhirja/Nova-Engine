#pragma once

#include "../engine/EntityCommon.h"

/**
 * CargoContainer: Auto-loading cargo container entity
 * 
 * This entity demonstrates the complete Nova Engine auto-loading pattern:
 * - Inherits from IActor interface
 * - Uses ActorContext for ECS integration
 * - Automatically loads configuration from JSON files
 * - Auto-registers with build system (no manual registration needed)
 * - Supports designer-friendly configuration
 * 
 * Configuration file: assets/actors/world/cargo_container.json
 * 
 * Features:
 * - Configurable cargo capacity and type restrictions
 * - Visual representation with customizable models
 * - Faction-based ownership and access control
 * - Automatic component setup for rendering and physics
 * 
 * TODO: Comprehensive Cargo Container System Roadmap
 * 
 * === CARGO MANAGEMENT ===
 * [ ] Inventory System: Advanced inventory management with item stacking
 * [ ] Cargo Types: Comprehensive cargo type system with properties
 * [ ] Container Variants: Different container sizes and specializations
 * [ ] Automated Loading: Automated cargo loading and unloading systems
 * [ ] Cargo Tracking: Track cargo movement and chain of custody
 * [ ] Quality Control: Cargo quality and condition monitoring
 * [ ] Cargo Manifests: Detailed cargo manifests and documentation
 * [ ] Container Networking: Networked container systems for large operations
 * [ ] Cargo Analytics: Analytics for cargo efficiency and optimization
 * [ ] Smart Containers: AI-enabled containers with self-management
 * 
 * === CONTAINER SECURITY ===
 * [ ] Access Control: Advanced access control and permission systems
 * [ ] Security Monitoring: Monitor container access and detect breaches
 * [ ] Encryption: Secure cargo data and container communications
 * [ ] Anti-Theft: Anti-theft systems and tracking for stolen containers
 * [ ] Biometric Access: Biometric authentication for high-security cargo
 * [ ] Tamper Detection: Detect tampering and unauthorized access
 * [ ] Emergency Lockdown: Emergency lockdown procedures for security threats
 * [ ] Audit Trails: Comprehensive audit trails for all container activities
 * [ ] Security Analytics: Analytics for security threats and vulnerabilities
 * [ ] Insurance Integration: Integration with cargo insurance systems
 * 
 * === CONTAINER LOGISTICS ===
 * [ ] Route Optimization: Optimize cargo container routing and scheduling
 * [ ] Fleet Management: Manage fleets of cargo containers and carriers
 * [ ] Supply Chain: Integration with supply chain management systems
 * [ ] Predictive Analytics: Predict cargo demand and optimize placement
 * [ ] Real-time Tracking: Real-time tracking of container location and status
 * [ ] Automated Dispatch: Automated dispatch and routing of containers
 * [ ] Load Balancing: Balance cargo loads across multiple containers
 * [ ] Emergency Routing: Emergency routing for critical cargo deliveries
 * [ ] Customs Integration: Integration with customs and regulatory systems
 * [ ] Documentation: Automated documentation and compliance reporting
 * 
 * === CONTAINER TECHNOLOGY ===
 * [ ] Smart Sensors: IoT sensors for environmental and cargo monitoring
 * [ ] Environmental Control: Climate control for sensitive cargo
 * [ ] Damage Prevention: Systems to prevent cargo damage during transport
 * [ ] Automated Sorting: Automated cargo sorting and organization
 * [ ] Robotic Loading: Robotic systems for cargo loading and unloading
 * [ ] AI Optimization: AI optimization of container space and weight
 * [ ] Predictive Maintenance: Predictive maintenance for container systems
 * [ ] Energy Management: Energy-efficient container systems and power
 * [ ] Communication: Advanced communication systems for container networks
 * [ ] Integration: Integration with ship and station cargo systems
 * 
 * === CONTAINER ECONOMICS ===
 * [ ] Dynamic Pricing: Dynamic pricing for container rentals and services
 * [ ] Market Analysis: Market analysis for cargo demand and pricing
 * [ ] Cost Optimization: Optimize container operations for cost efficiency
 * [ ] Revenue Tracking: Track revenue from container operations
 * [ ] Contract Management: Manage contracts for container services
 * [ ] Insurance: Comprehensive insurance for containers and cargo
 * [ ] Tax Compliance: Tax compliance and reporting for cargo operations
 * [ ] Financial Analytics: Financial analytics for container businesses
 * [ ] Investment Planning: Investment planning for container fleet expansion
 * [ ] Risk Management: Risk management for container operations
 * 
 * === CONTAINER VISUALIZATION ===
 * [ ] 3D Visualization: 3D visualization of container contents and status
 * [ ] AR Interface: Augmented reality interfaces for container management
 * [ ] Status Indicators: Visual status indicators for container health
 * [ ] Load Visualization: Visualize cargo load distribution and balance
 * [ ] Damage Visualization: Visual representation of container damage
 * [ ] Environmental Display: Display environmental conditions and alerts
 * [ ] Interactive Controls: Interactive controls for container management
 * [ ] Mobile Interface: Mobile interfaces for field container management
 * [ ] Dashboard: Comprehensive dashboard for container fleet management
 * [ ] Customization: Customizable interfaces for different user roles
 * 
 * === CONTAINER NETWORKING ===
 * [ ] Network Protocols: Efficient network protocols for container data
 * [ ] Mesh Networking: Mesh networking for container communication
 * [ ] Satellite Links: Satellite communication for remote containers
 * [ ] Edge Computing: Edge computing for local container intelligence
 * [ ] Data Synchronization: Synchronize container data across networks
 * [ ] Bandwidth Optimization: Optimize bandwidth usage for container data
 * [ ] Network Security: Secure container networks and communications
 * [ ] Failover Systems: Failover systems for network redundancy
 * [ ] Performance Monitoring: Monitor network performance for containers
 * [ ] Protocol Innovation: Develop new protocols for container networking
 * 
 * === CONTAINER SUSTAINABILITY ===
 * [ ] Eco-friendly Materials: Use sustainable materials for containers
 * [ ] Energy Efficiency: Maximize energy efficiency in container operations
 * [ ] Waste Reduction: Reduce waste in container manufacturing and operation
 * [ ] Recycling: Container recycling and material recovery programs
 * [ ] Carbon Footprint: Track and reduce carbon footprint of operations
 * [ ] Green Technology: Integrate green technology in container systems
 * [ ] Environmental Compliance: Ensure compliance with environmental regulations
 * [ ] Sustainability Reporting: Report on sustainability metrics and goals
 * [ ] Life Cycle Analysis: Analyze container life cycle environmental impact
 * [ ] Innovation: Drive innovation in sustainable container technology
 */
class CargoContainer : public IActor {
public:
    CargoContainer() = default;
    virtual ~CargoContainer() = default;

    // IActor interface
    void Initialize() override {
        // TODO: Advanced CargoContainer initialization
        // [ ] Multi-phase Init: Multi-phase initialization with dependency validation
        // [ ] Sensor Integration: Initialize IoT sensors and monitoring systems
        // [ ] Security Setup: Setup security systems and access controls
        // [ ] Network Setup: Initialize container networking and communication
        // [ ] Environmental Setup: Setup environmental control systems
        // [ ] AI Integration: Initialize AI systems for smart container management
        // [ ] Performance Setup: Setup performance monitoring and optimization
        // [ ] Event Registration: Register event handlers for container activities
        // [ ] Validation: Validate container configuration and parameters
        // [ ] Error Handling: Robust error handling during initialization
        
        // Auto-load configuration from JSON
        LoadConfiguration();
        
        // Set up ECS components for automatic rendering and physics
        SetupComponents();
        
        std::cout << "[CargoContainer] Auto-loaded: " << name_ 
                  << " (capacity: " << capacity_ << ", type: " << cargoType_ << ")" << std::endl;
    }
    
    void Update(double dt) override {
        // TODO: Comprehensive CargoContainer update system
        // [ ] Sensor Monitoring: Monitor IoT sensors and environmental conditions
        // [ ] Security Updates: Update security systems and threat monitoring
        // [ ] Network Updates: Update network communication and data sync
        // [ ] AI Processing: Process AI decisions for container optimization
        // [ ] Environmental Control: Update environmental control systems
        // [ ] Performance Monitoring: Monitor container performance and efficiency
        // [ ] Event Processing: Process container events and notifications
        // [ ] State Validation: Validate container state consistency
        // [ ] Analytics Update: Update analytics and reporting systems
        // [ ] Integration Updates: Update integration with external systems
        
        // Update cargo container logic
        UpdateCargoLogic(dt);
    }
    
    std::string GetName() const override { return name_; }

    // Public API for gameplay systems
    double GetCapacity() const { return capacity_; }
    double GetCurrentLoad() const { return currentLoad_; }
    const std::string& GetCargoType() const { return cargoType_; }
    const std::string& GetFaction() const { return faction_; }
    
    // TODO: Advanced CargoContainer interface methods
    // [ ] Container Commands: High-level command interface for container operations
    // [ ] Container Status: Comprehensive status reporting and monitoring
    // [ ] Container Events: Event system for container activities and notifications
    // [ ] Container Configuration: Runtime configuration of container parameters
    // [ ] Container Serialization: Save and load container state and cargo data
    // [ ] Container Metrics: Operational metrics and performance monitoring
    // [ ] Container Debugging: Debug interface for container system inspection
    // [ ] Container Integration: Integration with logistics and supply chain systems
    // [ ] Container Extensions: Extension points for modding and customization
    // [ ] Container Validation: Validation of container state and cargo consistency
    
    bool CanAcceptCargo(const std::string& cargoType, double amount) const {
        // Check type compatibility
        if (cargoType_ != "general" && cargoType_ != cargoType) {
            return false;
        }
        
        // Check capacity
        return (currentLoad_ + amount) <= capacity_;
    }
    
    bool AddCargo(const std::string& cargoType, double amount) {
        if (!CanAcceptCargo(cargoType, amount)) {
            return false;
        }
        
        currentLoad_ += amount;
        return true;
    }
    
    bool RemoveCargo(double amount) {
        if (amount > currentLoad_) {
            return false;
        }
        
        currentLoad_ -= amount;
        return true;
    }

private:
    // TODO: Enhanced CargoContainer data members
    // [ ] Sensor Array: IoT sensor array for environmental and cargo monitoring
    // [ ] Security Systems: Security system state and access control data
    // [ ] Network State: Network communication state and message queues
    // [ ] AI Controllers: AI controller instances for smart container management
    // [ ] Performance Counters: Performance monitoring and analytics data
    // [ ] Event Handlers: Event handling system for container activities
    // [ ] Environmental Controls: Environmental control system state
    // [ ] Integration State: Integration state with external logistics systems
    // [ ] Audit Data: Audit trail and compliance data structures
    // [ ] Optimization State: AI optimization state and decision history
    
    // Configuration properties (loaded from JSON)
    std::string name_ = "Cargo Container";
    double capacity_ = 1000.0;           // Maximum cargo units
    double currentLoad_ = 0.0;           // Current cargo units
    std::string cargoType_ = "general";  // Type restriction: "general", "ore", "fuel", "equipment"
    std::string model_ = "cargo_container_standard";
    std::string faction_ = "neutral";
    double health_ = 500.0;
    bool secured_ = false;               // Requires faction access
    
    // Runtime state
    bool isOpen_ = false;
    double lastAccessTime_ = 0.0;

    /**
     * Auto-load configuration from assets/actors/world/cargo_container.json
     * This demonstrates the Nova Engine auto-loading pattern
     */
    void LoadConfiguration() {
        try {
            // Use the ActorConfig system for type-safe JSON loading
            auto config = ActorConfig::LoadFromFile("assets/actors/world/cargo_container.json");
            
            if (config) {
                // Load properties with sensible defaults
                name_ = ActorConfig::GetString(*config, "name", "Cargo Container");
                capacity_ = ActorConfig::GetNumber(*config, "capacity", 1000.0);
                cargoType_ = ActorConfig::GetString(*config, "cargoType", "general");
                model_ = ActorConfig::GetString(*config, "model", "cargo_container_standard");
                faction_ = ActorConfig::GetString(*config, "faction", "neutral");
                health_ = ActorConfig::GetNumber(*config, "health", 500.0);
                secured_ = ActorConfig::GetBoolean(*config, "secured", false);
                
                // Initialize runtime state based on config
                currentLoad_ = ActorConfig::GetNumber(*config, "currentLoad", 0.0);
                
                std::cout << "[CargoContainer] Configuration loaded successfully from JSON" << std::endl;
            } else {
                std::cout << "[CargoContainer] Warning: Could not load config, using defaults" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[CargoContainer] Error loading config: " << e.what() << ", using defaults" << std::endl;
        }
    }

    /**
     * Set up ECS components for automatic rendering and physics integration
     */
    void SetupComponents() {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Add ViewportID component for automatic rendering
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(entity, vp);
            
            // Add Position component
            auto pos = std::make_shared<Position>();
            pos->x = 0.0; pos->y = 0.0; pos->z = 0.0;
            em->AddComponent<Position>(entity, pos);
            
            // Add DrawComponent for visual representation
            auto draw = std::make_shared<DrawComponent>();
            draw->mode = DrawComponent::RenderMode::Mesh3D;
            draw->visible = true;
            draw->renderLayer = 2; // Background objects
            draw->meshHandle = 0;  // Will use fallback colored cube
            
            // Set color based on cargo type
            if (cargoType_ == "ore") {
                draw->SetTint(0.7f, 0.5f, 0.3f); // Brown for ore
            } else if (cargoType_ == "fuel") {
                draw->SetTint(0.2f, 0.6f, 1.0f); // Blue for fuel
            } else if (cargoType_ == "equipment") {
                draw->SetTint(0.8f, 0.8f, 0.2f); // Yellow for equipment
            } else {
                draw->SetTint(0.6f, 0.6f, 0.6f); // Gray for general cargo
            }
            
            em->AddComponent<DrawComponent>(entity, draw);
            
            // Add PhysicsBody component for collision detection
            auto physics = std::make_shared<PhysicsBody>();
            physics->mass = 100.0 + currentLoad_; // Mass varies with cargo
            em->AddComponent<PhysicsBody>(entity, physics);
            
            std::cout << "[CargoContainer] ECS components configured automatically" << std::endl;
        }
    }

    /**
     * Update cargo container gameplay logic
     */
    void UpdateCargoLogic(double dt) {
        // Update based on current load (could affect physics, visuals, etc.)
        if (auto* em = context_.GetEntityManager()) {
            if (auto* physics = em->GetComponent<PhysicsBody>(context_.GetEntity())) {
                // Dynamically update mass based on current cargo load
                physics->mass = 100.0 + currentLoad_;
            }
            
            // Update visual scale based on load percentage
            if (auto* draw = em->GetComponent<DrawComponent>(context_.GetEntity())) {
                double loadPercentage = currentLoad_ / capacity_;
                float intensity = 0.5f + 0.5f * static_cast<float>(loadPercentage);
                
                // Adjust brightness based on load (darker = less cargo, brighter = more cargo)
                if (cargoType_ == "ore") {
                    draw->SetTint(0.7f * intensity, 0.5f * intensity, 0.3f * intensity); // Brown for ore
                } else if (cargoType_ == "fuel") {
                    draw->SetTint(0.2f * intensity, 0.6f * intensity, 1.0f * intensity); // Blue for fuel
                } else if (cargoType_ == "equipment") {
                    draw->SetTint(0.8f * intensity, 0.8f * intensity, 0.2f * intensity); // Yellow for equipment
                } else {
                    draw->SetTint(0.6f * intensity, 0.6f * intensity, 0.6f * intensity); // Gray for general cargo
                }
            }
        }
        
        // Track access time for gameplay systems
        lastAccessTime_ += dt;
    }
};

// Note: This entity will be automatically registered via the build system
// The Makefile scans entities/*.h and includes them in engine/Entities.h
// No manual registration is required - just add the header file!