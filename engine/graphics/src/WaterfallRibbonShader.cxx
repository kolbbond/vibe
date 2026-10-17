#include "WaterfallRibbonShader.hxx"

#include <Corrade/Utility/Assert.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

using namespace Magnum;

namespace {

// grid space -> world: same mapping as WaterfallShader
constexpr const char* RibbonVert = R"GLSL(
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 prevPos;
layout(location = 2) in vec3 nextPos;
layout(location = 3) in vec2 side; // x: across-line side / fill floor flag, y: kind (0 line, 1 fill)

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform vec2 gridSize;
uniform float heightScale;
uniform vec2 viewport;
uniform float lineWidth;
uniform float glow;
uniform float floorY;

out float vEdge; // signed px from the line center
out float vAmp;
out float vRow;
out float vKind;

vec3 toWorld(vec3 p) {
    float col = p.x / max(gridSize.x - 1.0, 1.0);
    float row = p.z / max(gridSize.y - 1.0, 1.0);
    return vec3((col - 0.5) * 4.0, p.y * heightScale, -(1.0 - row) * 3.0);
}

vec4 toClip(vec3 w) { return projectionMatrix * (transformationMatrix * vec4(w, 1.0)); }

void main() {
    vAmp = position.y;
    vRow = position.z / max(gridSize.y - 1.0, 1.0);
    vKind = side.y;
    vEdge = 0.0;

    if(side.y > 0.5) { // fill: the curve point, or the same point dropped to the floor
        vec3 w = toWorld(position);
        if(side.x > 0.5) w.y = floorY;
        gl_Position = toClip(w);
        return;
    }

    vec4 c = toClip(toWorld(position));
    vec4 p = toClip(toWorld(prevPos));
    vec4 n = toClip(toWorld(nextPos));
    vec2 halfVp = viewport * 0.5;
    vec2 sc = c.xy / c.w * halfVp;
    vec2 sp = p.xy / p.w * halfVp;
    vec2 sn = n.xy / n.w * halfVp;

    // each quad is one segment: the start vertex has prev == position, the end next == position
    vec2 seg = sn - sp;
    vec2 t = dot(seg, seg) < 1e-8 ? vec2(1.0, 0.0) : normalize(seg);
    vec2 normal = vec2(-t.y, t.x);
    float along = dot(sc - sp, sc - sp) < 1e-8 ? -1.0 : 1.0; // start: extend back, end: forward

    float halfWidth = glow > 0.0 ? lineWidth * 3.0 : lineWidth * 0.5;
    float extent = halfWidth + 1.0; // +1 px for the AA falloff
    // push out across the line, and past the ends by half a width so joints overlap
    c.xy += (normal * side.x * extent + t * along * halfWidth) / halfVp * c.w;
    vEdge = side.x * extent;
    // halo vs its own row's fill: same plane, so win the depth tie (rows are ~1e-4 apart in NDC)
    if(glow > 0.0) c.z -= 1e-5 * c.w;
    gl_Position = c;
}
)GLSL";

// premultiplied output; blend One, OneMinusSourceAlpha. Glow writes alpha 0 = additive.
constexpr const char* RibbonFrag = R"GLSL(
in float vEdge;
in float vAmp;
in float vRow;
in float vKind;

uniform float lineWidth;
uniform float glow;
uniform vec3 colorBase;
uniform vec3 colorPeak;
uniform vec3 fillColor;
uniform float fade;
uniform float scanlines;
uniform float time;

out vec4 FragColor;

void main() {
    float depthFade = mix(1.0, 0.12 + 0.88 * vRow, fade);
    if(vKind > 0.5) {
        FragColor = vec4(fillColor * depthFade, 1.0);
        return;
    }
    // never darker than the base color, so zero crossings stay lit
    vec3 color = mix(colorBase, colorPeak, smoothstep(0.25, 1.0, abs(vAmp)));
    color *= depthFade;
    color *= 1.0 - scanlines * (0.5 - 0.5 * sin(vRow * 200.0 + time * 2.0));

    if(glow > 0.0) {
        float r = abs(vEdge) / (lineWidth * 3.0);
        float g = exp(-r * r * 4.0) * glow;
        FragColor = vec4(color * g, 0.0);
        return;
    }
    float coverage = clamp(lineWidth * 0.5 + 0.5 - abs(vEdge), 0.0, 1.0);
    FragColor = vec4(color * coverage, coverage);
}
)GLSL";

} // namespace

namespace vibe {

WaterfallRibbonShader::WaterfallRibbonShader() {
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(RibbonVert);
    frag.addSource(RibbonFrag);
    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _transformationMatrix = uniformLocation("transformationMatrix");
    _projectionMatrix = uniformLocation("projectionMatrix");
    _gridSize = uniformLocation("gridSize");
    _heightScale = uniformLocation("heightScale");
    _viewport = uniformLocation("viewport");
    _lineWidth = uniformLocation("lineWidth");
    _glow = uniformLocation("glow");
    _floor = uniformLocation("floorY");
    _colorBase = uniformLocation("colorBase");
    _colorPeak = uniformLocation("colorPeak");
    _fillColor = uniformLocation("fillColor");
    _fade = uniformLocation("fade");
    _scanlines = uniformLocation("scanlines");
    _time = uniformLocation("time");
}

WaterfallRibbonShader& WaterfallRibbonShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(_transformationMatrix, matrix);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_projectionMatrix, matrix);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setGridSize(const Vector2& size) {
    setUniform(_gridSize, size);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setHeightScale(float scale) {
    setUniform(_heightScale, scale);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setViewport(const Vector2& size) {
    setUniform(_viewport, size);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setLineWidth(float px) {
    setUniform(_lineWidth, px);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setGlow(float strength) {
    setUniform(_glow, strength);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setFloor(float worldY) {
    setUniform(_floor, worldY);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setColors(const Color3& base, const Color3& peak) {
    setUniform(_colorBase, base);
    setUniform(_colorPeak, peak);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setFillColor(const Color3& color) {
    setUniform(_fillColor, color);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setFade(float amount) {
    setUniform(_fade, amount);
    return *this;
}

WaterfallRibbonShader& WaterfallRibbonShader::setScanlines(float amount, float time) {
    setUniform(_scanlines, amount);
    setUniform(_time, time);
    return *this;
}

} // namespace vibe
