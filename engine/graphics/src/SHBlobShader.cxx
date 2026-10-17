#include "SHBlobShader.hxx"

#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Corrade/Utility/Assert.h>

using namespace Magnum;

namespace {

constexpr const char* SHBlobVert = R"GLSL(
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;
uniform float time;
uniform float snapResolution;

// Audio band coefficients (for overall energy)
uniform float u_sub;
uniform float u_kick;
uniform float u_mid;
uniform float u_snare;
uniform float u_hat;

// FFT bin data — mapped onto sphere surface
uniform float u_fft[128];
uniform int u_numBins;

out vec3 fragNormal;
out vec3 viewDirection;
out float displacement;

// --- 3D simplex noise (Ashima Arts) ---
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 10.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    i = mod289(i);
    vec4 p = permute(permute(permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0));
    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;
    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;
    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

void main() {
    vec3 n = normalize(normal);

    float d = 0.0;

    if (u_numBins > 0) {
        // --- FFT-driven radial displacement ---
        // Map vertex normal to spherical coordinates
        float theta = acos(clamp(n.y, -1.0, 1.0));  // 0 = top, PI = bottom
        float phi = atan(n.z, n.x);                   // -PI to PI
        float t = theta / 3.14159265;                 // 0..1 → low freq to high freq

        // Bilinear lookup into FFT bins
        float binF = t * float(u_numBins - 1);
        int bin0 = int(floor(binF));
        int bin1 = min(bin0 + 1, u_numBins - 1);
        float frac = binF - float(bin0);
        float mag = mix(u_fft[bin0], u_fft[bin1], frac);

        // Break horizontal banding: add phi variation
        // Different phi angles get slightly different amplitude
        float phiVar = 0.7 + 0.3 * sin(phi * 3.0 + time * 0.4 + t * 6.28);
        d = mag * phiVar;

        // Add secondary noise layer for organic jiggle
        d += mag * 0.15 * snoise(n * 8.0 + vec3(time * 0.6, -time * 0.4, time * 0.5));
    } else {
        // Fallback: 5-band noise displacement (when no FFT data)
        d += u_sub  * 0.6  * snoise(n * 1.5 + vec3(time * 0.2, time * 0.15, -time * 0.1));
        d += u_kick * 0.5  * snoise(n * 3.0 + vec3(time * 0.5, -time * 0.3, time * 0.4));
        d += u_mid  * 0.35 * snoise(n * 5.0 + vec3(-time * 0.8, time * 0.6, time * 0.7));
        d += u_snare* 0.3  * snoise(n * 10.0 + vec3(time * 2.5, time * 1.8, -time * 2.0));
        d += u_hat  * 0.2  * snoise(n * 20.0 + vec3(time * 4.0, -time * 3.5, time * 3.0));
    }

    // Subtle idle breathing (always)
    d += 0.03 * snoise(n * 2.0 + vec3(time * 0.3, time * 0.2, -time * 0.25));

    displacement = d;

    // Displace vertex along normal (= change radius)
    vec3 displaced = position.xyz + n * d;

    vec4 worldPos = transformationMatrix * vec4(displaced, 1.0);
    fragNormal = normalize(normalMatrix * n);
    viewDirection = normalize(-worldPos.xyz);

    vec4 clipPos = projectionMatrix * worldPos;

    // PS2 vertex snapping
    if (snapResolution > 0.0) {
        vec2 snapped = floor(clipPos.xy / clipPos.w * snapResolution + 0.5) / snapResolution * clipPos.w;
        clipPos.xy = snapped;
    }

    gl_Position = clipPos;
}
)GLSL";

constexpr const char* SHBlobFrag = R"GLSL(
in vec3 fragNormal;
in vec3 viewDirection;
in float displacement;

uniform vec3 baseColor;
uniform vec3 rimColor;
uniform float rimPower;
uniform float time;
uniform float snapResolution;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 viewDir = normalize(viewDirection);

    // Fresnel rim glow
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, rimPower);

    // Displacement drives color: bulges glow, dips darken
    float d = clamp(displacement * 1.5, -1.0, 1.0);
    float bulge = max(d, 0.0);
    float dip = max(-d, 0.0);

    vec3 hot = rimColor * 1.3;
    vec3 cool = baseColor * 0.6;
    vec3 surfColor = mix(baseColor, hot, bulge);
    surfColor = mix(surfColor, cool, dip * 0.7);

    vec3 rimGlow = rimColor * rim * (0.8 + bulge * 0.6);

    float pulse = 0.95 + 0.05 * sin(time * 1.5 + displacement * 4.0);

    vec3 result = surfColor * pulse + rimGlow;

    // PS2 color banding
    if (snapResolution > 0.0) {
        float bands = 12.0;
        result = floor(result * bands + 0.5) / bands;
    }

    FragColor = vec4(result, 1.0);
}
)GLSL";

} // anonymous namespace

namespace vibe {

SHBlobShader::SHBlobShader() {
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(SHBlobVert);
    frag.addSource(SHBlobFrag);

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
    _timeUniform = uniformLocation("time");
    _snapResolutionUniform = uniformLocation("snapResolution");
    _baseColorUniform = uniformLocation("baseColor");
    _rimColorUniform = uniformLocation("rimColor");
    _rimPowerUniform = uniformLocation("rimPower");
    _subUniform = uniformLocation("u_sub");
    _kickUniform = uniformLocation("u_kick");
    _midUniform = uniformLocation("u_mid");
    _snareUniform = uniformLocation("u_snare");
    _hatUniform = uniformLocation("u_hat");
    _fftBinsUniform = uniformLocation("u_fft");
    _numBinsUniform = uniformLocation("u_numBins");
}

SHBlobShader& SHBlobShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(_transformationMatrixUniform, matrix);
    return *this;
}

SHBlobShader& SHBlobShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_projectionMatrixUniform, matrix);
    return *this;
}

SHBlobShader& SHBlobShader::setNormalMatrix(const Matrix3& matrix) {
    setUniform(_normalMatrixUniform, matrix);
    return *this;
}

SHBlobShader& SHBlobShader::setTime(float time) {
    setUniform(_timeUniform, time);
    return *this;
}

SHBlobShader& SHBlobShader::setSnapResolution(float res) {
    setUniform(_snapResolutionUniform, res);
    return *this;
}

SHBlobShader& SHBlobShader::setBaseColor(const Color3& color) {
    setUniform(_baseColorUniform, color);
    return *this;
}

SHBlobShader& SHBlobShader::setRimColor(const Color3& color) {
    setUniform(_rimColorUniform, color);
    return *this;
}

SHBlobShader& SHBlobShader::setRimPower(float power) {
    setUniform(_rimPowerUniform, power);
    return *this;
}

SHBlobShader& SHBlobShader::setSub(float v) {
    setUniform(_subUniform, v);
    return *this;
}

SHBlobShader& SHBlobShader::setKick(float v) {
    setUniform(_kickUniform, v);
    return *this;
}

SHBlobShader& SHBlobShader::setMid(float v) {
    setUniform(_midUniform, v);
    return *this;
}

SHBlobShader& SHBlobShader::setSnare(float v) {
    setUniform(_snareUniform, v);
    return *this;
}

SHBlobShader& SHBlobShader::setHat(float v) {
    setUniform(_hatUniform, v);
    return *this;
}

SHBlobShader& SHBlobShader::setFFTBins(Corrade::Containers::ArrayView<const float> bins) {
    setUniform(_fftBinsUniform, bins);
    return *this;
}

SHBlobShader& SHBlobShader::setNumBins(int count) {
    setUniform(_numBinsUniform, count);
    return *this;
}

} // namespace vibe
