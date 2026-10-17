#include "vibe/graphics/Batch2D.hxx"

#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Renderer.h>

namespace vibe::gfx {

using namespace Magnum;

Batch2D::Batch2D()
    : _shader{Shaders::FlatGL2D::Configuration{}.setFlags(Shaders::FlatGL2D::Flag::VertexColor)} {
    _mesh.setPrimitive(GL::MeshPrimitive::Triangles)
        .addVertexBuffer(_vertices, 0, Shaders::FlatGL2D::Position{}, Shaders::FlatGL2D::Color4{})
        .setIndexBuffer(_indices, 0, GL::MeshIndexType::UnsignedInt);
}

void Batch2D::begin(const Matrix3& projection) {
    _builder.clear();
    _projection = projection;
}

void Batch2D::end() {
    const auto& vertices = _builder.vertices();
    const auto& indices = _builder.indices();
    if(indices.empty()) return;

    _vertices.setData(Corrade::Containers::arrayView(vertices.data(), vertices.size()), GL::BufferUsage::StreamDraw);
    _indices.setData(Corrade::Containers::arrayView(indices.data(), indices.size()), GL::BufferUsage::StreamDraw);
    _mesh.setCount(static_cast<Int>(indices.size()));

    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    _shader.setTransformationProjectionMatrix(_projection).draw(_mesh);

    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}

} // namespace vibe::gfx
