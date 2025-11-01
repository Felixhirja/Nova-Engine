#pragma once

// Common includes for all entities - simplifies entity development for designers
// CRITICAL ORDER: EntityManager.h MUST come before ActorContext.h to avoid incomplete type issues
#include "ecs/EntityManager.h"
#include "ecs/Components.h"
#include "IActor.h"
#include "ActorContext.h"
#include "ActorContextImpl.h"
#include "ActorConfig.h"

// Standard library includes commonly used by entities
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

// Math utilities
#include <cmath>

// JSON support
#include "SimpleJson.h"