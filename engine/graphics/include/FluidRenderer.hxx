#pragma once

#include <optional>
#include <vector>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

#include "vibe/fluid/FluidSolver.hxx"
#include "vibe/fluid/Look.hxx"
#include "vibe/fluid/View.hxx"
#include "vibe/graphics/Batch2D.hxx"
#include "vibe/graphics/Fluid3DShader.hxx"
#include "vibe/graphics/FluidShader.hxx"
#include "vibe/graphics/GridMesh.hxx"
#include "vibe/graphics/OrbitCamera.hxx"

namespace vibe::gfx {

// draws a FluidSolver's dye field: the grid goes into a mipmapped RGBA16F texture,
// and one fullscreen triangle upscales it with glow, tonemap and colors.
// Needs a GL context. The texture is the seam for a future GPU solver.
class FluidRenderer {
public:
    FluidRenderer();

    // uploads the dye grid (re-creates the texture when the grid size changes)
    void update(const fluid::FluidSolver& solver);
    // draws into the current framebuffer and viewport
    void draw(const fluid::RenderLook& look, float glowScale, float hue, const fluid::FluidView& view = {});
    // debug: velocity arrows every `stride` cells
    void drawVelocity(const fluid::FluidSolver& solver, int stride = 8, const fluid::FluidView& view = {});
    // 3D modes: draw into the current framebuffer, which needs a depth buffer cleared by the caller
    void drawTerrain(const fluid::RenderLook& look, float glowScale, float hue, const OrbitCamera& camera, float viewAspect);
    void drawPlanet(const fluid::RenderLook& look, float glowScale, float hue, const OrbitCamera& camera, float viewAspect,
        float spinTurns);

private:
    Magnum::GL::Texture2D _texture{Magnum::NoCreate};
    Magnum::Vector2i _size{};
    int _levels = 1;
    Magnum::GL::Mesh _triangle;
    FluidShader _shader;
    Batch2D _batch;
    std::vector<float> _upload;

    struct Surface {
        Magnum::GL::Buffer uvs{Magnum::NoCreate};
        Magnum::GL::Buffer indices{Magnum::NoCreate};
        Magnum::GL::Mesh mesh{Magnum::NoCreate};
        Magnum::Vector2i grid{};
    };
    void buildSurface(Surface& surface, Magnum::Vector2i grid);
    void draw3D(Surface& surface, Fluid3DShader::Mode mode, const fluid::RenderLook& look, float glowScale, float hue,
        const OrbitCamera& camera, float viewAspect, float relief, float spin);
    std::optional<Fluid3DShader> _shader3d; // created on the first 3D draw
    Surface _terrain;
    Surface _sphere;
};

} // namespace vibe::gfx
