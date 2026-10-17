#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector2.h>

namespace vibe::gfx {

// orthographic 2D camera: `height` world units visible vertically, width from the view aspect
class Camera2D {
public:
    Camera2D() = default;
    Camera2D(Magnum::Vector2 center, float height) : _center{center}, _height{height} {}

    Magnum::Vector2 center() const { return _center; }
    float height() const { return _height; }

    // world -> clip space for a view with the given width/height aspect
    Magnum::Matrix3 projection(float aspect) const;
    // viewPx: pixels from the view's top-left corner, y down
    Magnum::Vector2 screenToWorld(Magnum::Vector2 viewPx, Magnum::Vector2i viewSize) const;
    Magnum::Vector2 worldToScreen(Magnum::Vector2 world, Magnum::Vector2i viewSize) const;

    // centered camera showing all of `world` in a view of `aspect` (letterboxed)
    static Camera2D fit(const Magnum::Range2D& world, float aspect);

private:
    Magnum::Vector2 _center{};
    float _height = 2.0f;
};

} // namespace vibe::gfx
