#include "FluidRenderer.hxx"

#include <algorithm>
#include <cmath>

#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Sampler.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/PixelFormat.h>

namespace vibe::gfx {

using namespace Magnum;

FluidRenderer::FluidRenderer() { _triangle.setPrimitive(GL::MeshPrimitive::Triangles).setCount(3); }

void FluidRenderer::update(const fluid::FluidSolver& solver) {
    const Vector2i size{solver.width(), solver.height()};
    if(size != _size) {
        _size = size;
        _levels = 1 + static_cast<int>(std::floor(std::log2(static_cast<float>(std::max(size.x(), size.y())))));
        _texture = GL::Texture2D{};
        _texture.setStorage(_levels, GL::TextureFormat::RGBA16F, size)
            .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setWrapping(GL::SamplerWrapping::ClampToEdge);
    }
    const std::vector<fluid::Color3>& dye = solver.dye();
    _upload.resize(dye.size() * 3);
    for(std::size_t k = 0; k < dye.size(); ++k) {
        _upload[3 * k + 0] = dye[k].r();
        _upload[3 * k + 1] = dye[k].g();
        _upload[3 * k + 2] = dye[k].b();
    }
    _texture.setSubImage(0, {}, ImageView2D{PixelFormat::RGB32F, size, Containers::arrayView(_upload.data(), _upload.size())});
    _texture.generateMipmap();
}

void FluidRenderer::draw(const fluid::RenderLook& look, float glowScale, float hue, const fluid::FluidView& view) {
    if(_size.isZero()) return;
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge);
    _shader.bindDye(_texture).setLook(look, glowScale, static_cast<float>(std::clamp(look.glowLod, 0, _levels - 1)), hue);
    _shader.setView(view.zoom, view.center).draw(_triangle);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}

void FluidRenderer::drawVelocity(const fluid::FluidSolver& solver, int stride, const fluid::FluidView& view) {
    const float aspect = static_cast<float>(solver.width()) / static_cast<float>(solver.height());
    // normalized field -> view -> clip
    _batch.begin(Matrix3::projection({1.0f, 1.0f}) * Matrix3::scaling(Vector2{view.zoom}) * Matrix3::translation(-view.center));
    const int step = std::max(1, stride);
    for(int j = step / 2; j < solver.height(); j += step)
        for(int i = step / 2; i < solver.width(); i += step) {
            const Vector2 at{(static_cast<float>(i) + 0.5f) / static_cast<float>(solver.width()),
                (static_cast<float>(j) + 0.5f) / static_cast<float>(solver.height())};
            const Vector2 v = solver.velocityAt(at);
            _batch.line(at, at + Vector2{v.x() / aspect, v.y()} * 0.1f, 0.002f, Color4{1.0f, 1.0f, 1.0f, 0.6f});
        }
    _batch.end();
}

void FluidRenderer::buildSurface(Surface& surface, Vector2i grid) {
    const GridMesh m = gridUvs(grid.x(), grid.y());
    surface.uvs = GL::Buffer{};
    surface.uvs.setData(Containers::arrayView(m.uvs.data(), m.uvs.size()));
    surface.indices = GL::Buffer{};
    surface.indices.setData(Containers::arrayView(m.indices.data(), m.indices.size()));
    surface.mesh = GL::Mesh{};
    surface.mesh.setCount(static_cast<Int>(m.indices.size()))
        .addVertexBuffer(surface.uvs, 0, Fluid3DShader::Uv{})
        .setIndexBuffer(surface.indices, 0, GL::MeshIndexType::UnsignedInt);
    surface.grid = grid;
}

void FluidRenderer::draw3D(Surface& surface, Fluid3DShader::Mode mode, const fluid::RenderLook& look, float glowScale, float hue,
    const OrbitCamera& camera, float viewAspect, float relief, float spin) {
    if(!_shader3d) _shader3d.emplace();
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    _shader3d->bindDye(_texture).setLook(look, glowScale, static_cast<float>(std::clamp(look.glowLod, 0, _levels - 1)), hue);
    _shader3d->setViewProjection(camera.projection(viewAspect) * camera.view())
        .setMode(mode)
        .setAspect(static_cast<float>(_size.x()) / static_cast<float>(_size.y()))
        .setRelief(relief)
        .setSpin(spin)
        .setTexel(Vector2{1.0f} / Vector2{_size})
        .setEye(camera.eye())
        .draw(surface.mesh);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
}

void FluidRenderer::drawTerrain(const fluid::RenderLook& look, float glowScale, float hue, const OrbitCamera& camera, float viewAspect) {
    if(_size.isZero()) return;
    const Vector2i grid = _size - Vector2i{1};
    if(_terrain.grid != grid) buildSurface(_terrain, grid);
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge);
    draw3D(_terrain, Fluid3DShader::Terrain, look, glowScale, hue, camera, viewAspect, 0.35f * look.height, 0.0f);
}

void FluidRenderer::drawPlanet(const fluid::RenderLook& look, float glowScale, float hue, const OrbitCamera& camera, float viewAspect,
    float spinTurns) {
    if(_size.isZero()) return;
    if(_sphere.grid.isZero()) buildSurface(_sphere, {128, 64});
    // longitude wraps (the solver runs with wrapX); latitude stops at the poles
    _texture.setWrapping({GL::SamplerWrapping::Repeat, GL::SamplerWrapping::ClampToEdge});
    draw3D(_sphere, Fluid3DShader::Planet, look, glowScale, hue, camera, viewAspect, 0.12f * look.height, spinTurns * 6.28318530718f);
}

} // namespace vibe::gfx
