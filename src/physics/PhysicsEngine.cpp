#include "PhysicsEngine.h"

namespace physics {

std::string ToString(PhysicsBackendType type) {
    switch (type) {
        case PhysicsBackendType::BuiltIn: return "BuiltIn";
        case PhysicsBackendType::Bullet: return "Bullet";
        case PhysicsBackendType::PhysX: return "PhysX";
        default: return "Unknown";
    }
}

} // namespace physics
