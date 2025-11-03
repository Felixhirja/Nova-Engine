# Actor Configuration Examples

This directory contains example actor configuration files that demonstrate proper JSON schema usage for Nova Engine actors.

## Station Examples

### Trading Station
**File**: `trading_station_example.json`

A commercial trading hub that provides essential services to travelers:
- **services**: ["trading", "refuel", "repair", "storage"] - Multiple commercial services
- **type**: "trading" - Commercial station type
- **dockingCapacity**: 12 - High capacity for busy trading routes

### Military Station
**File**: `military_station_example.json`

A defensive military outpost:
- **services**: ["repair", "missions", "upgrade"] - Military-focused services
- **type**: "military" - Armed defensive station
- **dockingCapacity**: 8 - Moderate capacity for patrol fleets

### Research Station
**File**: `research_station_example.json`

A scientific research facility:
- **services**: ["refuel", "upgrade", "storage"] - Research support services
- **type**: "research" - Scientific research station
- **dockingCapacity**: 6 - Lower capacity for scientific vessels

### Mining Station
**File**: `mining_station_example.json`

An industrial mining operation:
- **services**: ["trading", "refuel", "manufacturing"] - Industrial services
- **type**: "mining" - Resource extraction facility
- **dockingCapacity**: 15 - High capacity for cargo haulers

## Schema Validation

All examples conform to the `simple_station_config` schema which requires:

### Required Properties
- `name` (string) - Display name for the station
- `description` (string) - Detailed description 
- `entityType` (string) - Must be "station"
- `services` (array) - Services provided (minimum 1 item)
- `type` (string) - Station type from allowed enum

### Optional Properties  
- `dockingCapacity` (integer) - Maximum docked ships (1-50)

### Validation Rules
- **additionalProperties**: false - No extra properties allowed
- **services**: Must contain unique values from predefined enum
- **type**: Must be one of: military, trading, mining, research, civilian, industrial
- **dockingCapacity**: Must be integer between 1 and 50

## Testing Examples

To validate these examples:

```bash
# Build and run schema validation test
mingw32-make tests/test_schema_validation
./tests/test_schema_validation.exe
```

The test will validate example configs and show detailed error reports for any issues.

## Error Handling

When validation fails, the system provides:
- **Path**: Exact location of the error in JSON structure
- **Error**: Clear description of what went wrong  
- **Rule**: Which schema rule was violated
- **💡 Suggestion**: Developer-friendly fix recommendations

Example error output:
```
Validation failed with 1 error(s):
  1. Path: /services
     Error: Required property 'services' is missing
     Rule: required
     💡 Suggestion: Add "services" property to the configuration object
```

## Integration

These examples work with Nova Engine's actor system:
- Automatically loaded by `EntityConfigManager`
- Validated during `Station::Initialize()`
- Used by `EntityFactory` for spawning
- Support live validation in development builds