#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

#include "vibe/graphics/FluidLookShader.hxx"

namespace vibe::gfx {

// fullscreen dye shader: smooth upscale, glow (blurred mip), tonemap,
// dye or palette colors, hue rotation, zoom/pan. Draw a 3-vertex mesh without attributes.
class FluidShader : public FluidLookShader {
public:
    explicit FluidShader(Magnum::NoCreateT) : FluidLookShader{Magnum::NoCreate} {}
    FluidShader();

    // the view's middle shows field point `center`, magnified by `zoom`
    FluidShader& setView(float zoom, const Magnum::Vector2& center);

private:
    Magnum::Int _viewCenterUniform{0};
    Magnum::Int _viewScaleUniform{0};
};

} // namespace vibe::gfx
