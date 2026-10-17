#pragma once

#include <cstdint>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector2.h>

namespace vibe::game {

enum class LayoutMode : std::uint8_t { Shared, Split };

struct View {
    Magnum::Range2Di viewport; // framebuffer pixels, GL origin bottom-left
    int player = -1;           // -1: shared view or an empty split cell
    float aspect = 1.0f;       // width / height
};

// Shared: one full view. Split: 1 player full, 2 side-by-side (landscape) or stacked
// (portrait), 3-4 a 2x2 grid; cells are row-major from the top-left. The last
// column/row takes any remainder pixels. Empty for a zero-area framebuffer.
std::vector<View> layoutViews(Magnum::Vector2i framebuffer, int playerCount, LayoutMode mode, int gutterPx = 0);

// a viewport in window (ImGui) coordinates: origin top-left, y down, window units
Magnum::Range2D viewToWindow(const Magnum::Range2Di& viewport, Magnum::Vector2i framebuffer, Magnum::Vector2 windowSize);

} // namespace vibe::game
