#include "SpaceshipPhysicsSystem.h"
#include "Components.h"

#include <algorithm>
#include <cmath>

namespace {
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3() = default;
    Vec3(double ix, double iy, double iz) : x(ix), y(iy), z(iz) {}
};

Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator*(const Vec3& v, double s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

Vec3& operator+=(Vec3& a, const Vec3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

double Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double Length(const Vec3& v) {
    return std::sqrt(Dot(v, v));
}

Vec3 Normalize(const Vec3& v) {
    double len = Length(v);
    if (len < 1e-6) {
        return Vec3{0.0, 0.0, 0.0};
    }
    return v * (1.0 / len);
}

Vec3 RotateVector(const Vec3& v, double yaw, double pitch, double roll) {
    double cy = std::cos(yaw);
    double sy = std::sin(yaw);
    double cp = std::cos(pitch);
    double sp = std::sin(pitch);
    double cr = std::cos(roll);
    double sr = std::sin(roll);

    // Yaw around Z axis
    Vec3 afterYaw{
        cy * v.x - sy * v.y,
        sy * v.x + cy * v.y,
        v.z
    };

    // Pitch around X axis
    Vec3 afterPitch{
        afterYaw.x,
        cp * afterYaw.y - sp * afterYaw.z,
        sp * afterYaw.y + cp * afterYaw.z
    };

    // Roll around Y axis
    Vec3 afterRoll{
        cr * afterPitch.x + sr * afterPitch.z,
        afterPitch.y,
        -sr * afterPitch.x + cr * afterPitch.z
    };

    return afterRoll;
}

struct OrientationBasis {
    Vec3 forward;
    Vec3 right;
    Vec3 up;
};

OrientationBasis BuildOrientationBasis(double yaw, double pitch, double roll) {
    OrientationBasis basis;
    basis.forward = RotateVector(Vec3{0.0, 1.0, 0.0}, yaw, pitch, roll);
    basis.right = RotateVector(Vec3{1.0, 0.0, 0.0}, yaw, pitch, roll);
    basis.up = RotateVector(Vec3{0.0, 0.0, 1.0}, yaw, pitch, roll);
    return basis;
}

double WrapAngle(double angle) {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = 6.28318530717958647692;
    while (angle > kPi) {
        angle -= kTwoPi;
    }
    while (angle < -kPi) {
        angle += kTwoPi;
    }
    return angle;
}

inline double ClampInput(double value) {
    return std::clamp(value, -1.0, 1.0);
}

} // namespace

void SpaceshipPhysicsSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<SpaceshipFlightModel, Velocity, Position>(
        [&](Entity entity, SpaceshipFlightModel& flight, Velocity& velocity, Position& position) {
            const double mass = std::max(flight.massKg, 1.0);
            const OrientationBasis basis = BuildOrientationBasis(flight.yaw, flight.pitch, flight.roll);

            Vec3 totalForce{0.0, 0.0, 0.0};
            Vec3 velocityVec{velocity.vx, velocity.vy, velocity.vz};

            // Thrust forces from user input
            double throttleInput = ClampInput(flight.throttle);
            double forwardThrust = throttleInput >= 0.0
                ? throttleInput * flight.maxMainThrustN
                : throttleInput * flight.maxReverseThrustN;
            totalForce += basis.forward * forwardThrust;

            double strafeInput = ClampInput(flight.strafeInput);
            totalForce += basis.right * (strafeInput * flight.maxLateralThrustN);

            double verticalInput = ClampInput(flight.verticalInput);
            totalForce += basis.up * (verticalInput * flight.maxVerticalThrustN);

            // Damping to simulate inertia bleed-off in space or when powered down
            totalForce += velocityVec * (-flight.linearDamping);

            // Atmospheric effects
            double atmosphericDensity = 0.0;
            if (flight.atmosphericFlightEnabled) {
                double altitude = std::max(0.0, position.z - flight.atmosphereBaseAltitude);
                double scaleHeight = std::max(1.0, flight.atmosphereScaleHeight);
                double seaDensity = std::max(0.0, flight.seaLevelAtmosphericDensity);
                atmosphericDensity = seaDensity * std::exp(-altitude / scaleHeight);
                flight.currentAtmosphericDensity = atmosphericDensity;

                double speed = Length(velocityVec);
                if (speed > 1e-4) {
                    Vec3 velDir = velocityVec * (1.0 / speed);
                    double dragMagnitude = 0.5 * atmosphericDensity * speed * speed * flight.dragCoefficient * flight.referenceArea;
                    totalForce += velDir * (-dragMagnitude);
                }

                double forwardSpeed = Dot(velocityVec, basis.forward);
                if (forwardSpeed > 0.0 && flight.liftCoefficient > 0.0) {
                    double liftMagnitude = 0.5 * atmosphericDensity * forwardSpeed * forwardSpeed * flight.liftCoefficient * flight.referenceArea;
                    totalForce += basis.up * liftMagnitude;
                }
            } else {
                flight.currentAtmosphericDensity = 0.0;
            }

            // Gravity contribution (acts along global Z)
            totalForce.z += mass * flight.gravity;

            Vec3 linearAcceleration = totalForce * (1.0 / mass);
            velocityVec += linearAcceleration * dt;

            if (flight.maxLinearSpeed > 0.0) {
                double speed = Length(velocityVec);
                if (speed > flight.maxLinearSpeed) {
                    velocityVec = Normalize(velocityVec) * flight.maxLinearSpeed;
                }
            }

            velocity.vx = velocityVec.x;
            velocity.vy = velocityVec.y;
            velocity.vz = velocityVec.z;

            if (auto* acceleration = entityManager.GetComponent<Acceleration>(entity)) {
                acceleration->ax = linearAcceleration.x;
                acceleration->ay = linearAcceleration.y;
                acceleration->az = linearAcceleration.z;
            }

            flight.lastAppliedForceX = totalForce.x;
            flight.lastAppliedForceY = totalForce.y;
            flight.lastAppliedForceZ = totalForce.z;
            flight.lastLinearAccelerationX = linearAcceleration.x;
            flight.lastLinearAccelerationY = linearAcceleration.y;
            flight.lastLinearAccelerationZ = linearAcceleration.z;

            Vec3 angularVelocity{flight.angularVelocityX, flight.angularVelocityY, flight.angularVelocityZ};

            Vec3 controlTorque{
                ClampInput(flight.pitchInput) * flight.maxPitchTorque,
                ClampInput(flight.yawInput) * flight.maxYawTorque,
                ClampInput(flight.rollInput) * flight.maxRollTorque
            };

            Vec3 dampingTorque = angularVelocity * (-flight.angularDamping);
            if (atmosphericDensity > 0.0 && flight.atmosphericAngularDrag > 0.0) {
                dampingTorque += angularVelocity * (-flight.atmosphericAngularDrag * atmosphericDensity);
            }

            Vec3 totalTorque = controlTorque + dampingTorque;

            double inertiaX = std::max(flight.inertiaTensorX, 1.0);
            double inertiaY = std::max(flight.inertiaTensorY, 1.0);
            double inertiaZ = std::max(flight.inertiaTensorZ, 1.0);

            Vec3 angularAcceleration{
                totalTorque.x / inertiaX,
                totalTorque.y / inertiaY,
                totalTorque.z / inertiaZ
            };

            angularVelocity += angularAcceleration * dt;

            flight.angularVelocityX = angularVelocity.x;
            flight.angularVelocityY = angularVelocity.y;
            flight.angularVelocityZ = angularVelocity.z;

            flight.pitch = WrapAngle(flight.pitch + angularVelocity.x * dt);
            flight.yaw = WrapAngle(flight.yaw + angularVelocity.y * dt);
            flight.roll = WrapAngle(flight.roll + angularVelocity.z * dt);

            flight.lastAppliedTorqueX = totalTorque.x;
            flight.lastAppliedTorqueY = totalTorque.y;
            flight.lastAppliedTorqueZ = totalTorque.z;
            flight.lastAngularAccelerationX = angularAcceleration.x;
            flight.lastAngularAccelerationY = angularAcceleration.y;
            flight.lastAngularAccelerationZ = angularAcceleration.z;
        });
}

