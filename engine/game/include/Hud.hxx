#pragma once

#include <string_view>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector2.h>

#include "vibe/game/ViewLayout.hxx"

namespace vibe::game {

// ImGui foreground text and panels, positioned relative to a view's top-left in
// window units and clipped to the view. Call between drawBegin() and drawEnd().
class Hud {
public:
    // hudSize: ImGui's DisplaySize (window size / DPI scaling)
    void setFrame(Magnum::Vector2i framebufferSize, Magnum::Vector2 hudSize);

    Magnum::Vector2 viewSize(const View& view) const;
    void text(const View& view, Magnum::Vector2 posInView, const Magnum::Color4& color, std::string_view str, float scale = 1.0f);
    void textCentered(const View& view, Magnum::Vector2 centerInView, const Magnum::Color4& color, std::string_view str, float scale = 1.0f);
    void panel(const View& view, Magnum::Vector2 minInView, Magnum::Vector2 size, const Magnum::Color4& color);

private:
    Magnum::Range2D windowRect(const View& view) const;

    Magnum::Vector2i _framebuffer{1, 1};
    Magnum::Vector2 _window{1.0f, 1.0f};
};

} // namespace vibe::game
