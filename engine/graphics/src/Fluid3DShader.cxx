#include "Fluid3DShader.hxx"

#include <Corrade/Utility/Assert.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

using namespace Magnum;

namespace {

constexpr const char* Vert = R"GLSL(
layout(location = 0) in vec2 uv;
uniform mat4 viewProjection;
uniform int mode;
uniform float aspect;
uniform float relief;
uniform float spin;
uniform vec2 texel;
out vec2 st;
out vec3 worldPos;
out vec3 normal;

vec3 surface(vec2 p) {
    float h = plasmaHeight(p) * relief;
    if(mode == 0) return vec3((p.x - 0.5) * aspect, h, 0.5 - p.y);
    float lon = p.x * 6.28318530718 + spin;
    float lat = (p.y - 0.5) * 3.14159265359;
    return (1.0 + h) * vec3(cos(lat) * sin(lon), sin(lat), cos(lat) * cos(lon));
}

void main() {
    st = uv;
    worldPos = surface(uv);
    vec3 dx = surface(uv + vec2(texel.x, 0.0)) - surface(uv - vec2(texel.x, 0.0));
    vec3 dy = surface(uv + vec2(0.0, texel.y)) - surface(uv - vec2(0.0, texel.y));
    vec3 n = cross(dx, dy);
    if(mode == 1 && length(n) < 1e-6) n = worldPos; // poles
    // terrain: u runs +x and v runs -z, so the cross product points down
    normal = normalize(mode == 0 ? -n : n);
    gl_Position = viewProjection * vec4(worldPos, 1.0);
}
)GLSL";

constexpr const char* Frag = R"GLSL(
in vec2 st;
in vec3 worldPos;
in vec3 normal;
uniform int mode;
uniform vec3 eye;
out vec4 FragColor;

void main() {
    vec3 n = normalize(normal);
    vec3 v = normalize(eye - worldPos);
    if(dot(n, v) < 0.0) n = -n; // terrain seen from below
    vec3 l = normalize(vec3(0.4, 0.8, 0.3));
    vec3 base = plasmaColor(plasmaSample(st));
    float diffuse = max(dot(n, l), 0.0);
    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    vec3 color = base * (0.45 + 0.75 * diffuse) + base * rim * 0.8;
    if(mode == 1) color += vec3(0.25, 0.45, 1.0) * rim * 0.6 * brightness; // thin atmosphere
    else color *= exp(-0.25 * max(length(eye - worldPos) - 1.5, 0.0));     // fog to black
    FragColor = vec4(clamp(color, 0.0, 1.0) + ditherNoise(gl_FragCoord.xy), 1.0);
}
)GLSL";

} // namespace

namespace vibe::gfx {

Fluid3DShader::Fluid3DShader() {
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(lookGlsl()).addSource(Vert);
    frag.addSource(lookGlsl()).addSource(Frag);
    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    locateLookUniforms();
    _viewProjection = uniformLocation("viewProjection");
    _mode = uniformLocation("mode");
    _aspect = uniformLocation("aspect");
    _relief = uniformLocation("relief");
    _spin = uniformLocation("spin");
    _texel = uniformLocation("texel");
    _eye = uniformLocation("eye");
}

Fluid3DShader& Fluid3DShader::setViewProjection(const Matrix4& matrix) {
    setUniform(_viewProjection, matrix);
    return *this;
}

Fluid3DShader& Fluid3DShader::setMode(Mode mode) {
    setUniform(_mode, static_cast<Int>(mode));
    return *this;
}

Fluid3DShader& Fluid3DShader::setAspect(float aspect) {
    setUniform(_aspect, aspect);
    return *this;
}

Fluid3DShader& Fluid3DShader::setRelief(float relief) {
    setUniform(_relief, relief);
    return *this;
}

Fluid3DShader& Fluid3DShader::setSpin(float radians) {
    setUniform(_spin, radians);
    return *this;
}

Fluid3DShader& Fluid3DShader::setTexel(const Vector2& texel) {
    setUniform(_texel, texel);
    return *this;
}

Fluid3DShader& Fluid3DShader::setEye(const Vector3& eye) {
    setUniform(_eye, eye);
    return *this;
}

} // namespace vibe::gfx
