#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "vibe/fluid/FluidSolver.hxx"

namespace vibe::fluid {

enum Band : int { Sub = 0, Kick = 1, Mid = 2, Snare = 3, Hat = 4 };
constexpr int kBands = 5;

// what the music is doing this frame (filled by the host, e.g. from vvavviz's analysis)
struct AudioFrame {
    std::array<float, kBands> bands{};  // normalized levels, 0..1
    std::array<bool, kBands> onsets{};  // a hit this frame
    float loudness = 0.0f;              // 0..1
    std::array<float, 12> chroma{};     // pitch-class energy
    float bpm = 0.0f;                   // 0 = unknown
    double time = 0.0;                  // seconds
};

// a push and a dab of dye; positions normalized, force in field heights per second
struct Splat {
    Vec2 at;
    Vec2 force;
    Color3 dye;
    float radius;
};

// settings shared by every emitter (all preset-tunable)
struct EmitterCommon {
    bool enabled = true;
    float gain = 1.0f;
    float threshold = 0.1f;
    Color3 color{1.0f, 1.0f, 1.0f};
};

struct BurstParams : EmitterCommon {
    float strength = 1.5f;
    float ringRadius = 0.08f;
    int splats = 12;
    float refractory = 0.08f;
};
struct VortexParams : EmitterCommon {
    float strength = 1.0f;
    float radius = 0.25f;
};
struct JetsParams : EmitterCommon {
    float strength = 0.8f;
    float radius = 0.2f;
    float rate = 0.5f; // revolutions per second when the tempo is unknown
};
struct SplashParams : EmitterCommon {
    float strength = 1.2f;
    std::uint64_t seed = 1;
    float refractory = 0.08f;
};
struct SparksParams : EmitterCommon {
    int count = 30; // sparks at full hat level
};
struct GlowParams {
    float base = 0.6f;
    float gain = 0.6f;
};
struct HueParams {
    bool enabled = true;
    float rate = 0.5f;
    float minChroma = 0.2f;
};
// the "default pressure": a few points wandering the field, stirring with or without music
struct AmbientParams {
    bool enabled = true;
    float strength = 0.6f;  // push along each point's path
    float speed = 0.15f;    // how fast the points wander
    int points = 3;
    float radius = 0.08f;
    Color3 color{0.3f, 0.2f, 0.6f};
    float dye = 0.15f;      // faint trail so the stirring shows in silence
};
// how the music-driven emitters mix in
struct MixParams {
    bool music = true;          // off: only ambient (and the mouse) move the fluid
    float musicStrength = 1.5f; // multiplies every music-driven force
};

struct EmitterParams {
    BurstParams burst;
    VortexParams vortex;
    JetsParams jets;
    SplashParams splash;
    SparksParams sparks;
    GlowParams glow;
    HueParams hue;
    AmbientParams ambient;
    MixParams mix;
};

// the spec's defaults: white-hot bursts, violet vortex, cyan jets, magenta splashes, pale sparks
EmitterParams defaultEmitterParams();

// one audio feature -> splats
class Emitter {
public:
    virtual ~Emitter() = default;
    virtual void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) = 0;

    float aspect = 16.0f / 9.0f; // field width / height, so rings stay round
};

// kick hit -> ring of outward force + white-hot flash at the center
class Burst : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    BurstParams params;

private:
    double _last = -1.0e9;
};

// sub level -> tangential force around the center
class Vortex : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    VortexParams params;
};

// mid level -> two jets orbiting the center at a tempo-driven rate
class Jets : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    JetsParams params;

private:
    float _angle = 0.0f;
};

// snare hit -> splash at a seeded-random position
class Splash : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    SplashParams params;

private:
    double _last = -1.0e9;
    std::uint64_t _rng = 0;
    std::uint64_t _seededWith = 0;
    bool _seeded = false;
};

// hat level -> scatter of tiny short-lived dots
class Sparks : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    SparksParams params;

private:
    std::uint64_t _rng = 0x5eed5eed5eedull;
};

// music-independent wandering stirrers (deterministic)
class Ambient : public Emitter {
public:
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) override;
    AmbientParams params;

private:
    float _t = 0.0f;
};

// all emitters plus the non-splat outputs (glow and hue) for the renderer
class EmitterSet {
public:
    EmitterSet();

    void setParams(const EmitterParams& params);
    EmitterParams params() const;
    void setAspect(float aspect);
    void emit(const AudioFrame& frame, float dt, std::vector<Splat>& out);
    void apply(const AudioFrame& frame, float dt, FluidSolver& solver);

    float glow() const { return _glow; } // renderer glow multiplier
    float hue() const { return _hue; }   // palette rotation in turns, [0, 1)

    Burst burst;
    Vortex vortex;
    Jets jets;
    Splash splash;
    Sparks sparks;
    Ambient ambient;
    MixParams mix;
    GlowParams glowParams;
    HueParams hueParams;

private:
    void updateGlowHue(const AudioFrame& frame, float dt);

    float _glow = 0.6f;
    float _hue = 0.0f;
    std::vector<Splat> _scratch;
};

} // namespace vibe::fluid
