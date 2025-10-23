# Spaceship Systems - Quick Start Guide for Next Tasks

## 🚀 Executive Summary

- **Immediate focus:** Stand up the text rendering system so HUD/UI work can finally unblock downstream features.
- **Parallel prep:** Document component validation copy so designers can plug in better error messaging while engineering finalizes suggestions.
- **Upcoming deep work:** Begin modelling power, heat, and crew loops once rendering + validation are stable; these systems feed directly into combat + economy milestones.
- **Risks to track:** Font library integration time, availability of component catalog data for suggestions, and balancing parameters for the performance simulator.

## ✅ TODO Checklist

- [ ] Wire `TextRenderer` initialization into the main engine bootstrap sequence.
- [ ] Capture font asset pipeline steps in `docs/fonts_pipeline.md` so art can supply new glyph sets quickly.
- [ ] Surface `ShipAssemblyDiagnostics` suggestions inside the build UI overlay with severity coloring.
- [ ] Author regression tests for component suggestion fallbacks using `tests/test_validation.cpp` fixtures.
- [ ] Model heat dissipation curves against environmental modifiers (nebulae, vacuum, atmosphere) before shipping the simulator.
- [ ] Define cascading failure events for critical reactors and document alert flow for the damage model.
- [ ] Connect crew workload metrics to morale modifiers in the simulation loop.
- [ ] Extend cargo manifest serialization to sync with save-game schema updates in `assets/saves/`.

## 🎯 Recommended Implementation Order

Based on dependencies and priority, here's the optimal order to tackle remaining work:

---

## Priority 1: Text Rendering System (CRITICAL)

**Why First:** Blocks all UI/HUD work including energy management, ship status, targeting info, etc.

**Estimated Time:** 8-12 hours

### Option A: Integrate Existing Library (Recommended)
Use a proven text rendering solution:

1. **STB TrueType** (easiest, header-only)
   ```cpp
   // Single header include
   #define STB_TRUETYPE_IMPLEMENTATION
   #include "stb_truetype.h"
   ```
   - Pros: No dependencies, simple API, widely used
   - Cons: Basic features only

2. **SDL_ttf** (if using SDL)
   ```cpp
   #include <SDL_ttf.h>
   // Integrates naturally with SDL renderer
   ```
   - Pros: Works with existing SDL setup, full TTF support
   - Cons: External dependency

3. **FreeType** (most feature-rich)
   ```cpp
   #include <ft2build.h>
   #include FT_FREETYPE_H
   ```
   - Pros: Professional-grade, extensive features
   - Cons: Heavier dependency, more complex

### Implementation Steps

1. **Create TextRenderer class** (`src/TextRenderer.h/cpp`)
   ```cpp
   class TextRenderer {
   public:
       bool Init(const std::string& fontPath, int fontSize);
       void RenderText(const std::string& text, int x, int y, 
                      Color color = {255, 255, 255, 255});
       void RenderTextCentered(const std::string& text, int x, int y, 
                              Color color = {255, 255, 255, 255});
       Vector2 MeasureText(const std::string& text);
       
   private:
       // Font data/cache
   };
   ```

2. **Integrate with Viewport3D**
   - Add TextRenderer member to Viewport3D
   - Initialize in constructor
   - Call RenderText() in DrawHUD()

3. **Test with simple HUD elements**
   ```cpp
   textRenderer->RenderText("FPS: " + std::to_string(fps), 10, 10);
   textRenderer->RenderText("Power: 45.2 MW", 10, 30);
   ```

4. **Add to Makefile**
   ```makefile
   # If using SDL_ttf
   LIBS += -lSDL2_ttf
   
   # If using FreeType
   LIBS += -lfreetype
   ```

**Deliverable:** Working text rendering visible in HUD

---

## Priority 2: Component Validation Enhancement

**Estimated Time:** 4-6 hours

### Current State
- Basic validation works (SlotSizeFits, category matching)
- Errors stored in `ShipAssemblyDiagnostics`
- No user-friendly messages or suggestions

### Enhancement Goals

1. **Improve Error Messages** (`src/ShipAssembly.cpp`)
   
   Current:
   ```cpp
   diagnostics.AddError("Component size mismatch");
   ```
   
   Better:
   ```cpp
   diagnostics.AddError(
       "Component '" + component.displayName + "' (size " + 
       ToString(component.size) + ") does not fit in slot '" + 
       slot.slotId + "' (size " + ToString(slot.size) + ")"
   );
   ```

2. **Add Suggestions System**
   ```cpp
   struct ComponentSuggestion {
       std::string slotId;
       std::string reason;  // "Missing required component"
       std::vector<std::string> suggestedComponentIds;
   };
   
   // Add to ShipAssemblyDiagnostics
   std::vector<ComponentSuggestion> suggestions;
   ```

3. **Auto-Suggest Compatible Components**
   ```cpp
   // In ShipAssembler::Assemble()
   if (slot.required && !assigned) {
       auto compatible = FindCompatibleComponents(slot);
       diagnostics.AddSuggestion(slot.slotId, 
           "Required slot empty", compatible);
   }
   ```

4. **Add Helper Functions**
   ```cpp
   std::vector<const ShipComponentBlueprint*> 
   FindCompatibleComponents(const HullSlot& slot) {
       std::vector<const ShipComponentBlueprint*> result;
       for (auto& comp : ShipComponentCatalog::All()) {
           if (comp.category == slot.category && 
               SlotSizeFits(slot.size, comp.size)) {
               result.push_back(&comp);
           }
       }
       return result;
   }
   ```

**Deliverable:** Helpful validation with suggestions

---

## Priority 3: Advanced Performance Model

**Estimated Time:** 8-12 hours

### Enhancements Needed

#### 3.1 Power Overload System

Add to `ShipPerformanceMetrics`:
```cpp
struct PowerState {
    double outputMW;
    double drawMW;
    double availableMW;
    double overloadPercent;  // 0-100, >100 = overload
    bool isOverloaded;
    double timeToShutdown;   // Seconds until emergency shutdown
};

PowerState GetPowerState() const {
    PowerState state;
    state.outputMW = powerOutputMW;
    state.drawMW = powerDrawMW;
    state.availableMW = powerOutputMW - powerDrawMW;
    state.overloadPercent = (powerDrawMW / powerOutputMW) * 100.0;
    state.isOverloaded = powerDrawMW > powerOutputMW;
    state.timeToShutdown = state.isOverloaded ? 
        (powerOutputMW * 0.1) / (powerDrawMW - powerOutputMW) : -1.0;
    return state;
}
```

#### 3.2 Heat Simulation

Add to `ShipPerformanceMetrics`:
```cpp
struct HeatState {
    double generationMW;
    double dissipationMW;
    double netHeatMW;
    double currentTempC;       // Current temperature
    double maxTempC;           // Max safe temperature
    double criticalTempC;      // Damage threshold
    double heatPercent;        // 0-100
    bool isOverheating;
    double timeToCritical;     // Seconds until damage
};

// Update over time (call in simulation loop)
void UpdateHeat(double dt) {
    // Accumulate heat
    currentTempC += (netHeatMW * dt) / heatCapacity;
    
    // Check thresholds
    isOverheating = currentTempC > maxTempC;
    
    // Calculate time to critical
    if (isOverheating && netHeatMW > 0) {
        timeToCritical = (criticalTempC - currentTempC) / 
                        (netHeatMW / heatCapacity);
    }
}
```

#### 3.3 Crew Workload

Add to `ShipPerformanceMetrics`:
```cpp
struct CrewState {
    int required;
    int assigned;
    int capacity;
    double utilizationPercent;
    double efficiencyMultiplier;  // 0.5-1.5 based on utilization
    bool isUnderstaffed;
    bool isOverworked;
};

CrewState GetCrewState(int assignedCrew) const {
    CrewState state;
    state.required = crewRequired;
    state.assigned = assignedCrew;
    state.capacity = crewCapacity;
    state.utilizationPercent = (double)assignedCrew / crewRequired * 100.0;
    state.isUnderstaffed = assignedCrew < crewRequired;
    state.isOverworked = assignedCrew > crewCapacity;
    
    // Efficiency curve
    if (state.isUnderstaffed) {
        state.efficiencyMultiplier = 0.5 + 
            (0.5 * assignedCrew / crewRequired);
    } else if (state.isOverworked) {
        state.efficiencyMultiplier = 1.0 - 
            (0.5 * (assignedCrew - crewCapacity) / crewCapacity);
    } else {
        state.efficiencyMultiplier = 1.0;
    }
    return state;
}
```

**Deliverable:** Realistic power, heat, and crew simulation

---

## Priority 4: Damage Model Implementation

**Estimated Time:** 12-18 hours

### Implementation Phases

#### Phase A: Component Health Tracking

1. **Add to ShipComponentBlueprint**
   ```cpp
   struct ShipComponentBlueprint {
       // ... existing fields ...
       
       double maxHealth = 100.0;
       double armor = 0.0;            // Damage reduction
       bool isCritical = false;       // Ship fails if destroyed
       std::vector<std::string> requiredForComponents;  // Dependencies
   };
   ```

2. **Runtime Component State**
   ```cpp
   struct ComponentInstance {
       const ShipComponentBlueprint* blueprint;
       double currentHealth;
       bool isOperational;
       bool isDamaged;      // < 50% health
       bool isCritical;     // < 25% health
       double efficiency;   // 0-1, scales with health
   };
   ```

#### Phase B: Damage Application

```cpp
class DamageModel {
public:
    struct DamageEvent {
        double amount;
        std::string damageType;  // "kinetic", "energy", "thermal"
        Vector3 hitLocation;
        Vector3 hitDirection;
    };
    
    void ApplyDamage(ShipAssemblyResult& ship, 
                    const DamageEvent& damage) {
        // 1. Distribute damage to components
        // 2. Check for component destruction
        // 3. Trigger cascading failures
        // 4. Update ship performance metrics
    }
    
private:
    ComponentInstance* FindNearestComponent(
        const ShipAssemblyResult& ship,
        const Vector3& hitLocation
    );
    
    void ApplyCascadingFailures(
        ShipAssemblyResult& ship,
        const std::string& failedComponentId
    );
};
```

#### Phase C: Repair System

```cpp
struct RepairAction {
    std::string componentId;
    double repairAmount;
    double timeRequired;   // Seconds
    double cost;           // Credits
    bool requiresDockyard; // Field repair vs. station
};

class RepairSystem {
public:
    std::vector<RepairAction> GetRepairOptions(
        const ShipAssemblyResult& ship
    );
    
    bool CanRepairInField(const ShipComponentBlueprint& component);
    
    void ApplyRepair(ShipAssemblyResult& ship, 
                    const RepairAction& repair);
};
```

**Deliverable:** Working damage and repair mechanics

---

## Priority 5: Cargo Management System

**Estimated Time:** 8-12 hours

### Implementation

1. **Cargo Component Blueprint**
   ```cpp
   // Add to ShipComponentBlueprint
   double cargoVolumeCubicMeters = 0.0;
   double cargoMassKg = 0.0;
   bool isRefrigerated = false;
   bool isShielded = false;  // For hazardous cargo
   ```

2. **Cargo Hold System** (`src/CargoSystem.h`)
   ```cpp
   struct CargoItem {
       std::string commodityId;
       double volumeCubicMeters;
       double massKg;
       int quantity;
       bool requiresRefrigeration;
       bool requiresShielding;
   };
   
   class CargoHold {
   public:
       bool AddCargo(const CargoItem& item);
       bool RemoveCargo(const std::string& commodityId, int quantity);
       double GetAvailableVolume() const;
       double GetAvailableMass() const;
       double GetTotalCargoMass() const;
       const std::vector<CargoItem>& GetInventory() const;
       
   private:
       double maxVolume_;
       double maxMass_;
       std::vector<CargoItem> inventory_;
   };
   ```

3. **Integration with Ship**
   ```cpp
   // Add to ShipAssemblyResult
   std::unique_ptr<CargoHold> cargoHold;
   
   // Initialize during assembly
   if (HasSubsystem(ComponentSlotCategory::Cargo)) {
       auto& cargoSubsystem = GetSubsystem(ComponentSlotCategory::Cargo);
       double totalVolume = /* sum cargo component volumes */;
       cargoHold = std::make_unique<CargoHold>(totalVolume, totalMass);
   }
   ```

**Deliverable:** Functional cargo system for freighters

---

## Testing Strategy

After each implementation, add tests:

```cpp
// tests/test_text_rendering.cpp
TEST(TextRenderer, BasicRendering) {
    TextRenderer renderer;
    ASSERT_TRUE(renderer.Init("fonts/arial.ttf", 16));
    // Visual inspection or bitmap comparison
}

// tests/test_validation.cpp
TEST(ComponentValidation, SuggestionSystem) {
    ShipAssemblyRequest request;
    request.hullId = "fighter_basic";
    // Don't assign required power plant
    auto result = ShipAssembler::Assemble(request);
    ASSERT_TRUE(result.diagnostics.HasErrors());
    ASSERT_FALSE(result.diagnostics.suggestions.empty());
}

// tests/test_performance_model.cpp
TEST(PerformanceModel, PowerOverload) {
    // ... create ship with insufficient power ...
    auto powerState = result.performance.GetPowerState();
    ASSERT_TRUE(powerState.isOverloaded);
    ASSERT_GT(powerState.overloadPercent, 100.0);
}

// tests/test_damage_model.cpp
TEST(DamageModel, ComponentDestruction) {
    DamageModel::DamageEvent damage;
    damage.amount = 1000.0;
    damageModel.ApplyDamage(ship, damage);
    // Assert component health decreased
}

// tests/test_cargo_system.cpp
TEST(CargoSystem, VolumeLimit) {
    CargoHold hold(100.0, 50000.0);
    CargoItem item{"ore", 60.0, 10000.0, 1, false, false};
    ASSERT_TRUE(hold.AddCargo(item));
    
    CargoItem item2{"ore", 50.0, 10000.0, 1, false, false};
    ASSERT_FALSE(hold.AddCargo(item2));  // Exceeds volume
}
```

---

## Documentation Updates

As you implement, update:

1. **todo_spaceship.txt** - Mark tasks complete
2. **docs/spaceship_implementation_status.md** - Update completion percentages
3. **Code comments** - Document new systems
4. **README.md** - Add feature highlights

---

## Quick Wins (Low-Hanging Fruit)

If you want quick visible progress:

1. **Text rendering** → Immediate UI improvements
2. **Better error messages** → Better developer experience
3. **Power/heat visualization** → Cool HUD effects
4. **Damage decals** → Visual combat feedback

---

## Estimated Timeline

| Week | Focus | Hours | Deliverables |
|------|-------|-------|--------------|
| 1 | Text rendering + validation | 12-18 | Working HUD text, better errors |
| 2 | Performance model | 8-12 | Heat/power/crew simulation |
| 3 | Damage model Phase A+B | 12-18 | Component health, damage application |
| 4 | Damage model Phase C + Cargo | 10-16 | Repair system, cargo holds |
| **Total** | **4 weeks** | **42-64 hours** | **Core systems complete** |

---

## Getting Help

- **Text Rendering:** Check SDL_ttf docs, STB TrueType examples
- **Performance:** Reference real-world spaceship power/thermal specs
- **Damage:** Look at mechanic games (MechWarrior, Space Engineers)
- **Cargo:** Study Elite Dangerous or X series cargo mechanics

---

*Quick Start Guide Created: October 10, 2025*  
*Priority: Complete text rendering first, then tackle others in order*  
*Goal: Fully functional ship systems within 4-6 weeks*
