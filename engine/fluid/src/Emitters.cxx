#include "vibe/fluid/Emitters.hxx"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace vibe::fluid {

namespace {

constexpr float kTau = 6.28318530718f;
const Vec2 kCenter{0.5f, 0.5f};

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

float unit(std::uint64_t& state) { return static_cast<float>(splitmix64(state) >> 40) / 16777216.0f; }

// an offset in field heights around `origin`, as a normalized position
Vec2 around(Vec2 origin, Vec2 offset, float aspect) { return origin + Vec2{offset.x() / aspect, offset.y()}; }

} // namespace

EmitterParams defaultEmitterParams() {
    EmitterParams p;
    p.burst.color = Color3{1.0f, 0.95f, 0.85f};
    p.vortex.color = Color3{0.45f, 0.1f, 0.9f};
    p.vortex.threshold = 0.2f;
    p.jets.color = Color3{0.0f, 0.85f, 1.0f};
    p.jets.threshold = 0.15f;
    p.splash.color = Color3{1.0f, 0.15f, 0.75f};
    p.sparks.color = Color3{1.0f, 1.0f, 0.9f};
    p.sparks.threshold = 0.2f;
    return p;
}

void Burst::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    static_cast<void>(dt);
    if(!params.enabled || params.gain <= 0.0f || !frame.onsets[Kick]) return;
    if(frame.time - _last < params.refractory) return;
    _last = frame.time;
    const float level = 0.5f + frame.bands[Kick];
    const float power = params.strength * params.gain * level;
    const int n = std::max(1, params.splats);
    for(int k = 0; k < n; ++k) {
        const float a = kTau * static_cast<float>(k) / static_cast<float>(n);
        const Vec2 dir{std::cos(a), std::sin(a)};
        out.push_back({around(kCenter, dir * params.ringRadius, aspect), dir * power, params.color * (params.gain * level), 0.04f});
    }
}

void Vortex::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    static_cast<void>(dt);
    const float level = (frame.bands[Sub] - params.threshold) * params.gain;
    if(!params.enabled || level <= 0.0f) return;
    for(int k = 0; k < 8; ++k) {
        const float a = kTau * static_cast<float>(k) / 8.0f;
        const Vec2 radial{std::cos(a), std::sin(a)};
        const Vec2 tangent{-radial.y(), radial.x()};
        out.push_back({around(kCenter, radial * params.radius, aspect), tangent * (level * params.strength), params.color * (level * 0.3f), 0.06f});
    }
}

void Jets::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    const float rate = frame.bpm > 0.0f ? frame.bpm / 240.0f : params.rate; // 120 bpm -> half a turn per second
    _angle = std::fmod(_angle + dt * kTau * rate, kTau);
    const float level = (frame.bands[Mid] - params.threshold) * params.gain;
    if(!params.enabled || level <= 0.0f) return;
    for(const float offset : {0.0f, kTau * 0.5f}) {
        const float a = _angle + offset;
        const Vec2 radial{std::cos(a), std::sin(a)};
        const Vec2 tangent{-radial.y(), radial.x()};
        out.push_back({around(kCenter, radial * params.radius, aspect), tangent * (level * params.strength), params.color * level, 0.035f});
    }
}

void Splash::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    static_cast<void>(dt);
    if(!_seeded || _seededWith != params.seed) {
        _rng = params.seed;
        _seededWith = params.seed;
        _seeded = true;
    }
    if(!params.enabled || params.gain <= 0.0f || !frame.onsets[Snare]) return;
    if(frame.time - _last < params.refractory) return;
    _last = frame.time;
    const float level = 0.5f + frame.bands[Snare];
    const Vec2 at{0.15f + 0.7f * unit(_rng), 0.15f + 0.7f * unit(_rng)};
    const float a = kTau * unit(_rng);
    const Vec2 dir{std::cos(a), std::sin(a)};
    out.push_back({at, dir * (params.strength * params.gain * level), params.color * (params.gain * level), 0.06f});
}

void Sparks::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    static_cast<void>(dt);
    const float level = (frame.bands[Hat] - params.threshold) * params.gain;
    if(!params.enabled || level <= 0.0f || params.count <= 0) return;
    const int n = std::clamp(static_cast<int>(std::lround(level * static_cast<float>(params.count))), 1, params.count);
    for(int k = 0; k < n; ++k) {
        const Vec2 at{0.05f + 0.9f * unit(_rng), 0.05f + 0.9f * unit(_rng)};
        const float a = kTau * unit(_rng);
        out.push_back({at, Vec2{std::cos(a), std::sin(a)} * (0.2f * params.gain), params.color * (0.8f * params.gain), 0.012f});
    }
}

void Ambient::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    static_cast<void>(frame);
    _t += dt * params.speed;
    if(!params.enabled || params.points <= 0) return;
    for(int k = 0; k < params.points; ++k) {
        // each point runs its own Lissajous loop; the force follows the path's direction
        const float phase = kTau * static_cast<float>(k) / static_cast<float>(params.points);
        const float fx = 1.0f + 0.37f * static_cast<float>(k);
        const float fy = 1.3f + 0.23f * static_cast<float>(k);
        const float ax = kTau * fx * _t + phase;
        const float ay = kTau * fy * _t + 2.0f * phase;
        const Vec2 at{0.5f + 0.35f * std::sin(ax), 0.5f + 0.35f * std::sin(ay)};
        Vec2 dir{fx * std::cos(ax) / aspect, fy * std::cos(ay)};
        const float len = dir.length();
        dir = len > 1.0e-6f ? dir / len : Vec2{1.0f, 0.0f};
        out.push_back({at, dir * params.strength, params.color * params.dye, params.radius});
    }
}

EmitterSet::EmitterSet() { setParams(defaultEmitterParams()); }

void EmitterSet::setParams(const EmitterParams& p) {
    burst.params = p.burst;
    vortex.params = p.vortex;
    jets.params = p.jets;
    splash.params = p.splash;
    sparks.params = p.sparks;
    glowParams = p.glow;
    hueParams = p.hue;
    ambient.params = p.ambient;
    mix = p.mix;
}

EmitterParams EmitterSet::params() const {
    EmitterParams p;
    p.burst = burst.params;
    p.vortex = vortex.params;
    p.jets = jets.params;
    p.splash = splash.params;
    p.sparks = sparks.params;
    p.glow = glowParams;
    p.hue = hueParams;
    p.ambient = ambient.params;
    p.mix = mix;
    return p;
}

void EmitterSet::setAspect(float aspect) {
    for(Emitter* e : std::initializer_list<Emitter*>{&burst, &vortex, &jets, &splash, &sparks, &ambient}) e->aspect = aspect;
}

void EmitterSet::emit(const AudioFrame& frame, float dt, std::vector<Splat>& out) {
    // the music emitters always run (they keep their own clocks and refractories); muting just drops their splats
    const std::size_t first = out.size();
    for(Emitter* e : std::initializer_list<Emitter*>{&burst, &vortex, &jets, &splash, &sparks}) e->emit(frame, dt, out);
    if(!mix.music) out.resize(first);
    else
        for(std::size_t k = first; k < out.size(); ++k) out[k].force *= mix.musicStrength;
    ambient.emit(frame, dt, out);
    updateGlowHue(frame, dt);
}

void EmitterSet::apply(const AudioFrame& frame, float dt, FluidSolver& solver) {
    _scratch.clear();
    emit(frame, dt, _scratch);
    for(const Splat& s : _scratch) {
        if(s.force.dot() > 0.0f) solver.addForce(s.at, s.force, s.radius);
        if(s.dye.r() + s.dye.g() + s.dye.b() > 0.0f) solver.addDye(s.at, s.dye, s.radius);
    }
}

void EmitterSet::updateGlowHue(const AudioFrame& frame, float dt) {
    _glow = glowParams.base + glowParams.gain * frame.loudness;
    if(!hueParams.enabled) return;
    const auto best = std::max_element(frame.chroma.begin(), frame.chroma.end());
    if(*best < hueParams.minChroma) return;
    const float target = static_cast<float>(best - frame.chroma.begin()) / 12.0f;
    float delta = target - _hue;
    delta -= std::round(delta); // the short way around the color wheel
    _hue += delta * std::min(1.0f, hueParams.rate * dt * 4.0f);
    _hue -= std::floor(_hue);
}

} // namespace vibe::fluid
