#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

namespace vibe::gfx {

// a camera circling a target: yaw around the up axis, pitch above the horizon, distance
class OrbitCamera {
public:
    static constexpr float kMaxPitch = 1.4835299f; // 85 degrees
    static constexpr float kMinDistance = 0.6f;
    static constexpr float kMaxDistance = 8.0f;

    float yaw = 0.0f;   // radians, 0 = eye on +z
    float pitch = 0.6f; // radians above the target's horizon
    float distance = 2.4f;
    Magnum::Vector3 target{};

    void orbit(float dYaw, float dPitch); // clamps pitch
    void zoom(float factor);              // distance /= factor, clamped
    Magnum::Vector3 eye() const;
    Magnum::Matrix4 view() const;                   // world -> camera
    Magnum::Matrix4 projection(float aspect) const; // 45 degrees, near 0.05, far 50
};

} // namespace vibe::gfx
