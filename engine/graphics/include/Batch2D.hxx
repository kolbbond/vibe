#pragma once

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Shaders/FlatGL.h>

#include "vibe/graphics/Batch2DBuilder.hxx"

namespace vibe::gfx {

// immediate-mode 2D shapes drawn in one call with a vertex-colored flat shader.
// needs a GL context: construct it after the window exists
class Batch2D {
public:
    Batch2D();

    void begin(const Magnum::Matrix3& projection);
    void rect(Magnum::Vector2 min, Magnum::Vector2 size, const Magnum::Color4& color) { _builder.rect(min, size, color); }
    void line(Magnum::Vector2 a, Magnum::Vector2 b, float width, const Magnum::Color4& color) { _builder.line(a, b, width, color); }
    void grid(const Magnum::Range2D& area, Magnum::Vector2 cell, float width, const Magnum::Color4& color) {
        _builder.grid(area, cell, width, color);
    }
    // uploads and draws; leaves depth test and face culling on, blending off (smg's defaults)
    void end();

private:
    Batch2DBuilder _builder;
    Magnum::Matrix3 _projection;
    Magnum::Shaders::FlatGL2D _shader;
    Magnum::GL::Buffer _vertices;
    Magnum::GL::Buffer _indices;
    Magnum::GL::Mesh _mesh;
};

} // namespace vibe::gfx
