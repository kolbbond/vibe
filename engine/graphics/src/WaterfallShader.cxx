#include "WaterfallShader.hxx"

#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Corrade/Utility/Assert.h>

using namespace Magnum;

namespace {

constexpr const char* WaterfallVert = R"GLSL(
layout(location = 0) in vec3 position;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform float time;
uniform float heightScale;
uniform vec2 gridSize;  // (cols, rows)

out float vHeight;      // normalized height for color
out float vRow;         // row position (0 = oldest, 1 = newest)
out float vCol;         // column position (0 = low freq, 1 = high freq)

void main() {
    // position.x = column index (freq bin)
    // position.y = height (magnitude)
    // position.z = row index (time)

    float h = position.y * heightScale;

    // Normalize for fragment shader
    vHeight = position.y;
    vRow = position.z / max(gridSize.y - 1.0, 1.0);
    vCol = position.x / max(gridSize.x - 1.0, 1.0);

    // Map grid to world space: center X, Z goes back in time
    float x = (vCol - 0.5) * 4.0;         // spread across -2..2
    float z = -(1.0 - vRow) * 3.0;        // newest at front (z=0), oldest at back (z=-3)

    vec4 worldPos = transformationMatrix * vec4(x, h, z, 1.0);
    gl_Position = projectionMatrix * worldPos;
}
)GLSL";

constexpr const char* WaterfallFrag = R"GLSL(
in float vHeight;
in float vRow;
in float vCol;

uniform float time;
uniform vec3 colorLow;
uniform vec3 colorMid;
uniform vec3 colorHigh;

out vec4 FragColor;

void main() {
    // Height-based color gradient: low → mid → high (abs so negative amp = same color)
    float h = clamp(abs(vHeight) * 3.0, 0.0, 1.0);

    vec3 color;
    if (h < 0.5) {
        color = mix(colorLow, colorMid, h * 2.0);
    } else {
        color = mix(colorMid, colorHigh, (h - 0.5) * 2.0);
    }

    // Fade older rows (farther back in time)
    float rowFade = 0.3 + 0.7 * vRow;
    color *= rowFade;

    // Glow on peaks
    float glow = smoothstep(0.3, 1.0, h);
    color += colorHigh * glow * 0.3;

    // Subtle scanline effect on rows
    float scanline = 0.9 + 0.1 * sin(vRow * 200.0 + time * 2.0);
    color *= scanline;

    FragColor = vec4(color, 1.0);
}
)GLSL";

} // anonymous namespace

namespace vibe {

WaterfallShader::WaterfallShader() {
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(WaterfallVert);
    frag.addSource(WaterfallFrag);

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _timeUniform = uniformLocation("time");
    _heightScaleUniform = uniformLocation("heightScale");
    _colorLowUniform = uniformLocation("colorLow");
    _colorMidUniform = uniformLocation("colorMid");
    _colorHighUniform = uniformLocation("colorHigh");
    _gridSizeUniform = uniformLocation("gridSize");
}

WaterfallShader& WaterfallShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(_transformationMatrixUniform, matrix);
    return *this;
}

WaterfallShader& WaterfallShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_projectionMatrixUniform, matrix);
    return *this;
}

WaterfallShader& WaterfallShader::setTime(float time) {
    setUniform(_timeUniform, time);
    return *this;
}

WaterfallShader& WaterfallShader::setHeightScale(float scale) {
    setUniform(_heightScaleUniform, scale);
    return *this;
}

WaterfallShader& WaterfallShader::setColorLow(const Color3& color) {
    setUniform(_colorLowUniform, color);
    return *this;
}

WaterfallShader& WaterfallShader::setColorMid(const Color3& color) {
    setUniform(_colorMidUniform, color);
    return *this;
}

WaterfallShader& WaterfallShader::setColorHigh(const Color3& color) {
    setUniform(_colorHighUniform, color);
    return *this;
}

WaterfallShader& WaterfallShader::setGridSize(const Vector2& size) {
    setUniform(_gridSizeUniform, size);
    return *this;
}

} // namespace vibe
