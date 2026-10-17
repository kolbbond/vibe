#include "FluidShader.hxx"

#include <Corrade/Utility/Assert.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

using namespace Magnum;

namespace {

constexpr const char* FluidVert = R"GLSL(
out vec2 uv;

void main() {
    // one oversized triangle covering the viewport: uv (0,0) (2,0) (0,2)
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

constexpr const char* FluidFrag = R"GLSL(
uniform vec2 viewCenter;
uniform float viewScale;

in vec2 uv;
out vec4 FragColor;

void main() {
    vec2 st = viewCenter + (uv - 0.5) / viewScale; // zoom/pan: screen uv -> field uv
    if(any(lessThan(st, vec2(0.0))) || any(greaterThan(st, vec2(1.0)))) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    FragColor = vec4(plasmaColor(plasmaSample(st)) + ditherNoise(gl_FragCoord.xy), 1.0);
}
)GLSL";

} // namespace

namespace vibe::gfx {

FluidShader::FluidShader() {
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(FluidVert);
    frag.addSource(lookGlsl()).addSource(FluidFrag);
    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    locateLookUniforms();
    _viewCenterUniform = uniformLocation("viewCenter");
    _viewScaleUniform = uniformLocation("viewScale");
    setView(1.0f, Vector2{0.5f});
}

FluidShader& FluidShader::setView(float zoom, const Vector2& center) {
    setUniform(_viewScaleUniform, zoom);
    setUniform(_viewCenterUniform, center);
    return *this;
}

} // namespace vibe::gfx
