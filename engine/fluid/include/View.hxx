#pragma once

#include <algorithm>

#include "vibe/fluid/FluidSolver.hxx"

namespace vibe::fluid {

// zoom and pan over the field. Screen positions are normalized to the view
// ([0,1], y up, like field coords); the view's middle shows `center`.
struct FluidView {
    static constexpr float kMinZoom = 0.5f;
    static constexpr float kMaxZoom = 8.0f;

    float zoom = 1.0f;
    Vec2 center{0.5f, 0.5f};

    Vec2 toField(Vec2 screen) const { return center + (screen - Vec2{0.5f}) / zoom; }
    Vec2 toScreen(Vec2 field) const { return (field - center) * zoom + Vec2{0.5f}; }

    // zoom by `factor`, keeping the field point under `screen` where it is
    void zoomAt(Vec2 screen, float factor) {
        const Vec2 anchor = toField(screen);
        zoom = std::clamp(zoom * factor, kMinZoom, kMaxZoom);
        center = anchor - (screen - Vec2{0.5f}) / zoom;
        clampCenter();
    }
    // drag the picture by a screen-space delta
    void pan(Vec2 screenDelta) {
        center -= screenDelta / zoom;
        clampCenter();
    }
    void reset() { *this = FluidView{}; }

private:
    void clampCenter() { center = Vec2{std::clamp(center.x(), 0.0f, 1.0f), std::clamp(center.y(), 0.0f, 1.0f)}; }
};

} // namespace vibe::fluid
