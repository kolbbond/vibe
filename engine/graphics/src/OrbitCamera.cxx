#include "OrbitCamera.hxx"

#include <algorithm>
#include <cmath>

namespace vibe::gfx {

using namespace Magnum;

void OrbitCamera::orbit(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, -kMaxPitch, kMaxPitch);
}

void OrbitCamera::zoom(float factor) {
    if(factor > 0.0f) distance = std::clamp(distance / factor, kMinDistance, kMaxDistance);
}

Vector3 OrbitCamera::eye() const {
    return target + distance * Vector3{std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
}

Matrix4 OrbitCamera::view() const { return Matrix4::lookAt(eye(), target, Vector3::yAxis()).invertedRigid(); }

Matrix4 OrbitCamera::projection(float aspect) const { return Matrix4::perspectiveProjection(Deg{45.0f}, aspect, 0.05f, 50.0f); }

} // namespace vibe::gfx
