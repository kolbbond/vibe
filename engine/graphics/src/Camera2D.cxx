#include "vibe/graphics/Camera2D.hxx"

#include <algorithm>

namespace vibe::gfx {

using Magnum::Matrix3;
using Magnum::Vector2;

Matrix3 Camera2D::projection(float aspect) const {
    return Matrix3::projection({_height * aspect, _height}) * Matrix3::translation(-_center);
}

Vector2 Camera2D::screenToWorld(Vector2 viewPx, Magnum::Vector2i viewSize) const {
    const Vector2 size{viewSize};
    const float aspect = size.x() / size.y();
    const Vector2 ndc{viewPx.x() / size.x() * 2.0f - 1.0f, 1.0f - viewPx.y() / size.y() * 2.0f};
    return _center + ndc * Vector2{_height * aspect * 0.5f, _height * 0.5f};
}

Vector2 Camera2D::worldToScreen(Vector2 world, Magnum::Vector2i viewSize) const {
    const Vector2 size{viewSize};
    const float aspect = size.x() / size.y();
    const Vector2 ndc = (world - _center) / Vector2{_height * aspect * 0.5f, _height * 0.5f};
    return {(ndc.x() + 1.0f) * 0.5f * size.x(), (1.0f - ndc.y()) * 0.5f * size.y()};
}

Camera2D Camera2D::fit(const Magnum::Range2D& world, float aspect) {
    const float height = std::max(world.sizeY(), world.sizeX() / aspect);
    return Camera2D{world.center(), height};
}

} // namespace vibe::gfx
