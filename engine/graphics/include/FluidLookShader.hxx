#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Magnum.h>

#include "vibe/fluid/Look.hxx"

namespace vibe::gfx {

// base for the plasma shaders: the dye sampler plus the look uniforms (glow, tonemap,
// palette, hue, brightness) and the GLSL that uses them, so every display mode colors alike.
// A derived program adds lookGlsl() before its own sources, links, then calls locateLookUniforms().
class FluidLookShader : public Magnum::GL::AbstractShaderProgram {
public:
    explicit FluidLookShader(Magnum::NoCreateT) : Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {}

    FluidLookShader& bindDye(Magnum::GL::Texture2D& texture);
    // glowLod already clamped to the texture's mip levels
    FluidLookShader& setLook(const fluid::RenderLook& look, float glowScale, float glowLod, float hue);

    // uniforms + rotateHue, plasmaSample(st), plasmaColor(c), plasmaHeight(st)
    static const char* lookGlsl();

protected:
    FluidLookShader() = default;
    void locateLookUniforms();

private:
    Magnum::Int _glow{0};
    Magnum::Int _glowLod{0};
    Magnum::Int _exposure{0};
    Magnum::Int _paletteMode{0};
    Magnum::Int _palette0{0};
    Magnum::Int _palette1{0};
    Magnum::Int _palette2{0};
    Magnum::Int _hue{0};
    Magnum::Int _brightness{0};
};

} // namespace vibe::gfx
