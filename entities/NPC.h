#pragma once

#include "../engine/EntityCommon.h"

/**
 * NPC: Base class for non-player character spaceships
 * Provides AI behavior framework and spaceship functionality
 * 
 * TODO: Comprehensive NPC System Roadmap
 * 
 * === NPC AI FRAMEWORK ===
 * [ ] Behavior Trees: Advanced behavior tree system for complex AI behaviors
 * [ ] State Machines: Hierarchical state machines for AI decision making
 * [ ] Goal-Oriented AI: Goal-oriented action planning (GOAP) for smart decisions
 * [ ] Machine Learning: ML-based AI that learns from player and NPC interactions
 * [ ] Personality System: NPC personalities that affect behavior and decisions
 * [ ] Social AI: AI social interactions and relationship management
 * [ ] Dynamic Difficulty: AI difficulty scaling based on player skill
 * [ ] Emergent Behavior: Emergent AI behaviors from simple rule interactions
 * [ ] AI Communication: AI-to-AI communication and coordination protocols
 * [ ] AI Analytics: Analytics and metrics for AI behavior effectiveness
 * 
 * === NPC BEHAVIOR SYSTEMS ===
 * [ ] Combat AI: Advanced combat AI with tactics and formation flying
 * [ ] Navigation AI: Intelligent navigation with obstacle avoidance
 * [ ] Social Behavior: Social interactions between NPCs and with players
 * [ ] Economic Behavior: Economic decision-making and trading AI
 * [ ] Exploration AI: AI exploration patterns and curiosity-driven behavior
 * [ ] Survival AI: Self-preservation and emergency response behaviors
 * [ ] Cooperative AI: Cooperative behaviors and teamwork mechanics
 * [ ] Territorial AI: Territory control and patrol behaviors
 * [ ] Adaptive AI: AI that adapts to changing game conditions
 * [ ] Faction AI: Faction-specific behaviors and loyalty systems
 * 
 * === NPC SPECIALIZATIONS ===
 * [ ] Trader Specialists: Specialized trader types with unique behaviors
 * [ ] Combat Specialists: Different combat roles and specializations
 * [ ] Explorer NPCs: Exploration-focused NPCs with discovery behaviors
 * [ ] Diplomat NPCs: Diplomatic NPCs for faction interactions
 * [ ] Scientist NPCs: Research-focused NPCs with scientific behaviors
 * [ ] Criminal NPCs: Criminal organizations and illegal activities
 * [ ] Mercenary NPCs: Hire-able NPCs with contract-based behaviors
 * [ ] Civilian NPCs: Civilian population with everyday activities
 * [ ] VIP NPCs: Important NPCs with special protection requirements
 * [ ] Elite NPCs: Elite-level NPCs with superior capabilities
 * 
 * === NPC COMMUNICATION ===
 * [ ] Voice Acting: Voice-acted dialogue and communication
 * [ ] Dynamic Dialogue: Context-sensitive dialogue generation
 * [ ] Language System: Multi-language support and localization
 * [ ] Emotion System: Emotional states affecting communication
 * [ ] Reputation System: Reputation-based dialogue and interaction options
 * [ ] Information Trading: NPCs trading information and intelligence
 * [ ] Rumor System: Dynamic rumor generation and propagation
 * [ ] Negotiation System: Complex negotiation mechanics with NPCs
 * [ ] Cultural Adaptation: NPCs adapting communication to player culture
 * [ ] Translation System: Real-time translation for alien NPCs
 * 
 * === NPC ECONOMY ===
 * [ ] Economic Simulation: NPCs participating in dynamic economy
 * [ ] Supply Chains: NPCs managing complex supply chain operations
 * [ ] Market Analysis: NPCs analyzing market conditions for decisions
 * [ ] Investment Behavior: NPCs making investment and business decisions
 * [ ] Resource Competition: NPCs competing for limited resources
 * [ ] Economic Warfare: NPCs engaging in economic competition
 * [ ] Trade Networks: NPCs establishing and maintaining trade networks
 * [ ] Price Negotiation: Dynamic price negotiation with NPCs
 * [ ] Economic Intelligence: NPCs gathering economic intelligence
 * [ ] Market Manipulation: NPCs attempting to manipulate markets
 * 
 * === NPC PROGRESSION ===
 * [ ] Skill Development: NPCs developing skills over time
 * [ ] Equipment Upgrades: NPCs upgrading ships and equipment
 * [ ] Career Progression: NPCs advancing in their careers and roles
 * [ ] Relationship Development: NPCs developing relationships with players
 * [ ] Reputation Building: NPCs building reputation within factions
 * [ ] Experience System: NPCs gaining experience from activities
 * [ ] Achievement System: NPCs working toward personal goals
 * [ ] Legacy System: NPCs leaving lasting impact on game world
 * [ ] Generational NPCs: Multi-generational NPC families and lineages
 * [ ] Evolution System: NPCs evolving and adapting over time
 * 
 * === NPC NETWORKING ===
 * [ ] Network Synchronization: Efficient NPC state synchronization
 * [ ] AI Authority: Distributed AI processing and authority management
 * [ ] Bandwidth Optimization: Optimize network traffic for NPC data
 * [ ] Lag Compensation: Compensate for network lag in NPC interactions
 * [ ] Predictive AI: Client-side AI prediction for smooth behavior
 * [ ] Cloud AI: Cloud-based AI processing for complex behaviors
 * [ ] AI Load Balancing: Balance AI processing across servers
 * [ ] Network Analytics: Monitor network performance for NPC systems
 * [ ] Cheat Prevention: Prevent cheating related to NPC interactions
 * [ ] AI Security: Secure AI behavior data and prevent exploitation
 * 
 * === NPC DEBUGGING ===
 * [ ] AI Visualization: Visual debugging tools for AI behavior
 * [ ] Behavior Recording: Record and replay NPC behavior sessions
 * [ ] AI Profiling: Profile AI performance and decision-making
 * [ ] Debug Console: In-game debug console for NPC manipulation
 * [ ] State Inspection: Real-time inspection of NPC AI state
 * [ ] Behavior Analytics: Analyze NPC behavior patterns and effectiveness
 * [ ] Error Reporting: Automated error reporting for AI system issues
 * [ ] Performance Monitoring: Monitor NPC system performance
 * [ ] AI Testing: Automated testing framework for NPC behaviors
 * [ ] Diagnostic Tools: Tools for diagnosing AI system problems
 */
class NPC : public IActor {
public:
    enum class NPCType {
        Trader,
        Pirate,
        Patrol
    };

    NPC(NPCType type = NPCType::Trader) : npcType_(type) {}
    virtual ~NPC() = default;

    // IActor interface
    void Initialize() override;
    std::string GetName() const override;
    void Update(double dt) override;

    // NPC-specific methods
    NPCType GetNPCType() const { return npcType_; }

    void SetFaction(const std::string& faction) { faction_ = faction; }
    const std::string& GetFaction() const { return faction_; }

    void SetAIEnabled(bool enabled) { aiEnabled_ = enabled; }
    bool IsAIEnabled() const { return aiEnabled_; }

    // Spaceship properties
    void SetSpaceshipClass(SpaceshipClassType shipClass) { spaceshipClass_ = shipClass; }
    SpaceshipClassType GetSpaceshipClass() const { return spaceshipClass_; }
    
    // TODO: Advanced NPC interface methods
    // [ ] AI Commands: High-level command interface for AI behavior control
    // [ ] NPC Status: Comprehensive status reporting for NPC state and goals
    // [ ] NPC Events: Event system for NPC activities and state changes
    // [ ] NPC Configuration: Runtime configuration of AI parameters and behavior
    // [ ] NPC Serialization: Save and load NPC state and AI memory
    // [ ] NPC Metrics: Performance metrics and behavior analytics
    // [ ] NPC Debugging: Debug interface for AI behavior inspection
    // [ ] NPC Integration: Integration with faction and economic systems
    // [ ] NPC Extensions: Extension points for modding and custom behaviors
    // [ ] NPC Validation: Validation of NPC state and AI consistency

protected:
    virtual void UpdateAI(double dt) = 0;
    
    // TODO: Enhanced NPC protected interface
    // [ ] AI Utilities: Shared AI utility functions for derived classes
    // [ ] Behavior Templates: Template behaviors for common NPC actions
    // [ ] Decision Framework: Framework for AI decision-making processes
    // [ ] Communication Tools: Tools for NPC-to-NPC communication
    // [ ] Navigation Helpers: Helper functions for navigation and pathfinding
    // [ ] Combat Utilities: Shared combat behavior utilities
    // [ ] Economic Tools: Tools for economic decision-making
    // [ ] State Management: State management utilities for AI behaviors
    // [ ] Event Handling: Event handling framework for AI responses
    // [ ] Performance Tools: Performance optimization tools for AI systems

private:
    // TODO: Enhanced NPC data members
    // [ ] AI State: Comprehensive AI state tracking and memory systems
    // [ ] Behavior Stack: Stack-based behavior management for complex AI
    // [ ] Decision History: Track decision history for learning and debugging
    // [ ] Performance Counters: Performance monitoring for AI operations
    // [ ] Event Handlers: Event handling for AI responses and triggers
    // [ ] Communication State: Communication state and message queues
    // [ ] Navigation Data: Navigation state and pathfinding data
    // [ ] Combat State: Combat-related state and target tracking
    // [ ] Economic State: Economic state and trading history
    // [ ] Social State: Social relationships and reputation data
    
    NPCType npcType_;
    std::string faction_ = "neutral";
    bool aiEnabled_ = true;
    SpaceshipClassType spaceshipClass_ = SpaceshipClassType::Fighter;

    std::unique_ptr<simplejson::JsonObject> config_;

    // NPC-specific properties loaded from JSON
    std::string name_;
    double speed_ = 100.0;
    double health_ = 1000.0;
    double shield_ = 500.0;
    std::string model_;
};

/**
 * TraderNPC: Merchant spaceship that trades goods between stations
 * 
 * TODO: Advanced Trader NPC Features
 * [ ] Trade Route Planning: Intelligent trade route optimization
 * [ ] Market Analysis: Real-time market analysis for profitable trades
 * [ ] Cargo Management: Advanced cargo loading and inventory management
 * [ ] Risk Assessment: Assess piracy and market risks before trading
 * [ ] Negotiation AI: Advanced price negotiation with stations and players
 * [ ] Convoy Formation: Form trading convoys for protection
 * [ ] Emergency Protocols: Emergency responses to pirate attacks
 * [ ] Economic Intelligence: Gather and use economic intelligence
 * [ ] Reputation Management: Build and maintain trading reputation
 * [ ] Contract System: Generate and fulfill trading contracts
 */
class TraderNPC : public NPC {
public:
    TraderNPC() : NPC(NPCType::Trader) {}
    ~TraderNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "TraderNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> traderConfig_;
};

/**
 * PirateNPC: Hostile spaceship that attacks traders and players
 * 
 * TODO: Advanced Pirate NPC Features
 * [ ] Hunting Strategies: Sophisticated hunting and ambush strategies
 * [ ] Pack Behavior: Coordinate attacks with other pirates
 * [ ] Territory Control: Control and defend pirate territories
 * [ ] Loot Management: Intelligent looting and cargo assessment
 * [ ] Escape Tactics: Advanced escape and evasion tactics
 * [ ] Target Selection: Smart target selection based on risk/reward
 * [ ] Intimidation System: Use reputation to intimidate victims
 * [ ] Hideout Management: Manage pirate bases and hideouts
 * [ ] Black Market: Participate in black market trading
 * [ ] Faction Warfare: Engage in warfare with law enforcement
 */
class PirateNPC : public NPC {
public:
    PirateNPC() : NPC(NPCType::Pirate) {}
    ~PirateNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "PirateNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> pirateConfig_;
};

/**
 * PatrolNPC: Military spaceship that patrols areas and responds to threats
 * 
 * TODO: Advanced Patrol NPC Features
 * [ ] Patrol Routes: Dynamic patrol route generation and optimization
 * [ ] Threat Response: Rapid response to threats and distress calls
 * [ ] Formation Flying: Military formation flying and coordination
 * [ ] Investigation: Investigate suspicious activities and anomalies
 * [ ] Checkpoint Control: Manage checkpoints and inspection procedures
 * [ ] Backup Coordination: Coordinate with backup and reinforcements
 * [ ] Rules of Engagement: Complex rules of engagement and escalation
 * [ ] Intelligence Gathering: Gather intelligence on criminal activities
 * [ ] Peacekeeping: Maintain peace and order in patrol areas
 * [ ] Emergency Response: Respond to emergencies and disasters
 */
class PatrolNPC : public NPC {
public:
    PatrolNPC() : NPC(NPCType::Patrol) {}
    ~PatrolNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "PatrolNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> patrolConfig_;
};

// Inline implementations

inline void NPC::Initialize() {
    config_ = ActorConfig::LoadFromFile("assets/actors/ships/npc.json");
    if (config_) {
        name_ = ActorConfig::GetString(*config_, "name", "NPC");
        speed_ = ActorConfig::GetNumber(*config_, "speed", 100.0);
        health_ = ActorConfig::GetNumber(*config_, "health", 1000.0);
        shield_ = ActorConfig::GetNumber(*config_, "shield", 500.0);
        model_ = ActorConfig::GetString(*config_, "model", "npc_ship");
        faction_ = ActorConfig::GetString(*config_, "faction", "neutral");
    }

    // Set up basic ECS components
    if (auto* em = context_.GetEntityManager()) {
        em->AddComponent<Position>(context_.GetEntity(), std::make_shared<Position>(0.0, 0.0, 0.0));
        em->AddComponent<Velocity>(context_.GetEntity(), std::make_shared<Velocity>(0.0, 0.0, 0.0));
        // Auto-add ViewportID component for rendering
        em->AddComponent<ViewportID>(context_.GetEntity(), std::make_shared<ViewportID>(0));
    }
}

inline std::string NPC::GetName() const {
    if (!name_.empty()) return name_;
    
    switch (npcType_) {
        case NPCType::Trader: return "Trader";
        case NPCType::Pirate: return "Pirate";
        case NPCType::Patrol: return "Patrol";
        default: return "NPC";
    }
}

inline void NPC::Update(double dt) {
    if (aiEnabled_) {
        UpdateAI(dt);
    }
}

// TraderNPC implementations

inline void TraderNPC::Initialize() {
    NPC::Initialize();
    traderConfig_ = ActorConfig::LoadFromFile("assets/actors/ships/trader.json");
    SetFaction("traders_guild");
}

inline void TraderNPC::UpdateAI(double dt) {
    // TODO: Implement trader AI
    // - Navigate to trading stations
    // - Avoid pirates
    // - Trade goods
    (void)dt;
}

// PirateNPC implementations

inline void PirateNPC::Initialize() {
    NPC::Initialize();
    pirateConfig_ = ActorConfig::LoadFromFile("assets/actors/ships/pirate.json");
    SetFaction("pirates");
}

inline void PirateNPC::UpdateAI(double dt) {
    // TODO: Implement pirate AI
    // - Hunt traders
    // - Attack players
    // - Flee from patrols
    (void)dt;
}

// PatrolNPC implementations

inline void PatrolNPC::Initialize() {
    NPC::Initialize();
    patrolConfig_ = ActorConfig::LoadFromFile("assets/actors/ships/patrol.json");
    SetFaction("military");
}

inline void PatrolNPC::UpdateAI(double dt) {
    // TODO: Implement patrol AI
    // - Follow patrol routes
    // - Respond to distress calls
    // - Engage pirates
    (void)dt;
}