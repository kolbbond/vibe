#include "FluidLookShader.hxx"

#include <Magnum/Math/Color.h>

using namespace Magnum;

namespace vibe::gfx {

namespace {

constexpr const char* LookGlsl = R"GLSL(
uniform sampler2D dye;
uniform float glow;
uniform float glowLod;
uniform float exposure;
uniform int paletteMode;
uniform vec3 palette0;
uniform vec3 palette1;
uniform vec3 palette2;
uniform float hue;
uniform float brightness;

// rotate chroma in YIQ space; luminance stays
vec3 rotateHue(vec3 c, float turns) {
    float a = turns * 6.28318530718;
    float s = sin(a);
    float k = cos(a);
    float y = dot(c, vec3(0.299, 0.587, 0.114));
    float i = dot(c, vec3(0.596, -0.274, -0.322));
    float q = dot(c, vec3(0.211, -0.523, 0.312));
    float i2 = k * i - s * q;
    float q2 = s * i + k * q;
    return vec3(y + 0.956 * i2 + 0.621 * q2, y - 0.272 * i2 - 0.647 * q2, y - 1.106 * i2 + 1.703 * q2);
}

// Catmull-Rom bicubic in 9 bilinear taps: the dye grid is far coarser than the screen,
// and plain bilinear shows the cells as soft diamonds when upscaled
vec3 dyeBicubic(vec2 st) {
    vec2 texSize = vec2(textureSize(dye, 0));
    vec2 pos = st * texSize - 0.5;
    vec2 p1 = floor(pos);
    vec2 f = pos - p1;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;
    vec2 t0 = (p1 - 0.5) / texSize;
    vec2 t3 = (p1 + 2.5) / texSize;
    vec2 t12 = (p1 + 0.5 + w2 / w12) / texSize;
    vec3 c = (textureLod(dye, vec2(t0.x, t0.y), 0.0).rgb * w0.x + textureLod(dye, vec2(t12.x, t0.y), 0.0).rgb * w12.x
                 + textureLod(dye, vec2(t3.x, t0.y), 0.0).rgb * w3.x) * w0.y
           + (textureLod(dye, vec2(t0.x, t12.y), 0.0).rgb * w0.x + textureLod(dye, vec2(t12.x, t12.y), 0.0).rgb * w12.x
                 + textureLod(dye, vec2(t3.x, t12.y), 0.0).rgb * w3.x) * w12.y
           + (textureLod(dye, vec2(t0.x, t3.y), 0.0).rgb * w0.x + textureLod(dye, vec2(t12.x, t3.y), 0.0).rgb * w12.x
                 + textureLod(dye, vec2(t3.x, t3.y), 0.0).rgb * w3.x) * w3.y;
    return max(c, vec3(0.0)); // the negative lobes can undershoot near hard edges
}

// dye plus its blurred (glow) sample
vec3 plasmaSample(vec2 st) { return dyeBicubic(st) + textureLod(dye, st, glowLod).rgb * glow; }

// +-0.5/255 triangular noise so dark gradients don't band in the 8-bit output
vec3 ditherNoise(vec2 fragCoord) {
    float r1 = fract(sin(dot(fragCoord, vec2(12.9898, 78.233))) * 43758.5453);
    float r2 = fract(sin(dot(fragCoord, vec2(39.3468, 11.1353))) * 24634.6345);
    return vec3((r1 + r2 - 1.0) / 255.0);
}

// tonemap, dye or palette colors, hue rotation, brightness
vec3 plasmaColor(vec3 c) {
    vec3 color;
    if(paletteMode == 1) {
        float t = 1.0 - exp(-exposure * dot(max(c, vec3(0.0)), vec3(0.299, 0.587, 0.114)));
        vec3 p = t < 0.5 ? mix(palette0, palette1, t * 2.0) : mix(palette1, palette2, (t - 0.5) * 2.0);
        color = p * smoothstep(0.0, 0.2, t);
    } else {
        color = vec3(1.0) - exp(-exposure * max(c, vec3(0.0)));
    }
    return clamp(rotateHue(color, hue) * brightness, 0.0, 1.0);
}

// 0..1 relief from the slightly blurred plasma
float plasmaHeight(vec2 st) {
    vec3 c = textureLod(dye, st, 1.0).rgb;
    return 1.0 - exp(-exposure * dot(max(c, vec3(0.0)), vec3(0.299, 0.587, 0.114)));
}
)GLSL";

} // namespace

const char* FluidLookShader::lookGlsl() { return LookGlsl; }

void FluidLookShader::locateLookUniforms() {
    setUniform(uniformLocation("dye"), 0); // texture unit 0
    _glow = uniformLocation("glow");
    _glowLod = uniformLocation("glowLod");
    _exposure = uniformLocation("exposure");
    _paletteMode = uniformLocation("paletteMode");
    _palette0 = uniformLocation("palette0");
    _palette1 = uniformLocation("palette1");
    _palette2 = uniformLocation("palette2");
    _hue = uniformLocation("hue");
    _brightness = uniformLocation("brightness");
}

FluidLookShader& FluidLookShader::bindDye(GL::Texture2D& texture) {
    texture.bind(0);
    return *this;
}

FluidLookShader& FluidLookShader::setLook(const fluid::RenderLook& look, float glowScale, float glowLod, float hue) {
    setUniform(_glow, look.glow * glowScale);
    setUniform(_glowLod, glowLod);
    setUniform(_exposure, look.exposure);
    setUniform(_paletteMode, look.colorMode == fluid::ColorMode::Palette ? 1 : 0);
    setUniform(_palette0, look.palette[0]);
    setUniform(_palette1, look.palette[1]);
    setUniform(_palette2, look.palette[2]);
    setUniform(_hue, hue);
    setUniform(_brightness, look.brightness);
    return *this;
}

} // namespace vibe::gfx
