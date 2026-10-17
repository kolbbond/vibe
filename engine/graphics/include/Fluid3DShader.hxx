#pragma once

#include <Magnum/GL/Attribute.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>

#include "vibe/graphics/FluidLookShader.hxx"

namespace vibe::gfx {

// the plasma as a lit 3D surface over a uv grid (see gridUvs):
// mode 0 = terrain (heightfield on a plane), 1 = planet (sphere, u = longitude)
class Fluid3DShader : public FluidLookShader {
public:
    typedef Magnum::GL::Attribute<0, Magnum::Vector2> Uv;

    enum Mode : int { Terrain = 0, Planet = 1 };

    explicit Fluid3DShader(Magnum::NoCreateT) : FluidLookShader{Magnum::NoCreate} {}
    Fluid3DShader();

    Fluid3DShader& setViewProjection(const Magnum::Matrix4& matrix);
    Fluid3DShader& setMode(Mode mode);
    Fluid3DShader& setAspect(float aspect); // terrain plane width / depth
    Fluid3DShader& setRelief(float relief); // world units at full plasma height
    Fluid3DShader& setSpin(float radians);  // planet rotation
    Fluid3DShader& setTexel(const Magnum::Vector2& texel);
    Fluid3DShader& setEye(const Magnum::Vector3& eye);

private:
    Magnum::Int _viewProjection{0};
    Magnum::Int _mode{0};
    Magnum::Int _aspect{0};
    Magnum::Int _relief{0};
    Magnum::Int _spin{0};
    Magnum::Int _texel{0};
    Magnum::Int _eye{0};
};

} // namespace vibe::gfx
