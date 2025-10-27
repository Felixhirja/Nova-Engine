#pragma once

#include "MathUtils.h"
#include "ecs/Components.h"
#include <vector>

// Navigation systems for AI pathfinding and movement
namespace NavigationSystem {
    // Update patrol route navigation
    inline void updatePatrolNavigation(PatrolRoute& patrol, NavigationState& nav, const Position& currentPos) {
        if (patrol.waypoints.empty()) {
            nav.hasTarget = false;
            return;
        }

        // Check if we've reached current target
        Point3D currentPoint(currentPos.x, currentPos.y, currentPos.z);
        double distance = currentPoint.distanceTo(patrol.waypoints[patrol.currentWaypointIndex]);

        if (distance < patrol.arrivalThreshold) {
            // Move to next waypoint
            patrol.currentWaypointIndex = (patrol.currentWaypointIndex + 1) % patrol.waypoints.size();
        }

        // Set target to current waypoint
        nav.targetPosition = patrol.waypoints[patrol.currentWaypointIndex];
        nav.hasTarget = true;
    }

    // Calculate movement inputs to reach target
    inline void calculateMovementInputs(const Position& currentPos, const NavigationState& nav,
                                      float& throttle, double& yaw, double& pitch) {
        if (!nav.hasTarget) {
            throttle = 0.0f;
            yaw = 0.0;
            pitch = 0.0;
            return;
        }

        Point3D currentPoint(currentPos.x, currentPos.y, currentPos.z);
        MathUtils::calculateFacingAngles(currentPoint, nav.targetPosition, yaw, pitch);

        double distance = currentPoint.distanceTo(nav.targetPosition);
        if (distance > 0.0) {
            // Set throttle based on distance (moderate speed)
            throttle = 0.5f;
        } else {
            throttle = 0.0f;
        }
    }
}