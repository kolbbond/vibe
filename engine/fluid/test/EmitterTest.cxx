#include <cmath>

#include <Corrade/TestSuite/Tester.h>

#include "vibe/fluid/CpuStableFluid.hxx"
#include "vibe/fluid/Emitters.hxx"

using namespace vibe::fluid;

namespace {

constexpr float kDt = 1.0f / 60.0f;

AudioFrame kickFrame(double time, float level) {
    AudioFrame f;
    f.time = time;
    f.bands[Kick] = level;
    f.onsets[Kick] = true;
    return f;
}

float totalForce(const std::vector<Splat>& splats) {
    float sum = 0.0f;
    for(const Splat& s : splats) sum += s.force.length();
    return sum;
}

} // namespace

struct EmitterTest : Corrade::TestSuite::Tester {
    explicit EmitterTest();

    void burstOnKickOnset();
    void noOnsetNoBurst();
    void burstFiresOncePerOnset();
    void vortexIsTangentialAndScales();
    void zeroGainEmitsNothing();
    void splashIsSeeded();
    void sparksScaleWithHats();
    void hueFollowsChroma();
    void glowFollowsLoudness();
    void applyFeedsTheSolver();
    void ambientMovesWithoutMusic();
    void ambientIsDeterministic();
    void musicOffSilencesMusicEmitters();
    void ambientOffLeavesOnlyMusic();
    void musicStrengthScalesForces();
};

EmitterTest::EmitterTest() {
    addTests({&EmitterTest::burstOnKickOnset,
        &EmitterTest::noOnsetNoBurst,
        &EmitterTest::burstFiresOncePerOnset,
        &EmitterTest::vortexIsTangentialAndScales,
        &EmitterTest::zeroGainEmitsNothing,
        &EmitterTest::splashIsSeeded,
        &EmitterTest::sparksScaleWithHats,
        &EmitterTest::hueFollowsChroma,
        &EmitterTest::glowFollowsLoudness,
        &EmitterTest::applyFeedsTheSolver,
        &EmitterTest::ambientMovesWithoutMusic,
        &EmitterTest::ambientIsDeterministic,
        &EmitterTest::musicOffSilencesMusicEmitters,
        &EmitterTest::ambientOffLeavesOnlyMusic,
        &EmitterTest::musicStrengthScalesForces});
}

void EmitterTest::burstOnKickOnset() {
    Burst burst;
    burst.params = defaultEmitterParams().burst;
    burst.aspect = 1.0f;
    std::vector<Splat> out;
    burst.emit(kickFrame(1.0, 0.8f), kDt, out);
    CORRADE_COMPARE(out.size(), 12u);
    for(const Splat& s : out) {
        const Vec2 radial = s.at - Vec2{0.5f, 0.5f};
        CORRADE_VERIFY(Magnum::Math::dot(radial, s.force) > 0.0f); // outward
        CORRADE_VERIFY(s.dye.r() > 0.0f);
    }
}

void EmitterTest::noOnsetNoBurst() {
    Burst burst;
    burst.params = defaultEmitterParams().burst;
    AudioFrame loud;
    loud.bands[Kick] = 0.9f; // loud, but no hit this frame
    std::vector<Splat> out;
    burst.emit(loud, kDt, out);
    CORRADE_VERIFY(out.empty());
}

void EmitterTest::burstFiresOncePerOnset() {
    Burst burst;
    burst.params = defaultEmitterParams().burst; // 80 ms refractory
    std::vector<Splat> out;
    burst.emit(kickFrame(1.00, 0.8f), kDt, out);
    burst.emit(kickFrame(1.04, 0.8f), kDt, out); // inside the refractory window
    CORRADE_COMPARE(out.size(), 12u);
    burst.emit(kickFrame(1.20, 0.8f), kDt, out);
    CORRADE_COMPARE(out.size(), 24u);
}

void EmitterTest::vortexIsTangentialAndScales() {
    Vortex vortex;
    vortex.params = defaultEmitterParams().vortex; // threshold 0.2
    vortex.aspect = 16.0f / 9.0f;
    AudioFrame frame;
    frame.bands[Sub] = 0.6f; // level 0.4
    std::vector<Splat> weak;
    vortex.emit(frame, kDt, weak);
    CORRADE_VERIFY(!weak.empty());
    for(const Splat& s : weak) {
        const Vec2 radial{(s.at.x() - 0.5f) * vortex.aspect, s.at.y() - 0.5f}; // back to field heights
        CORRADE_VERIFY(std::abs(Magnum::Math::dot(radial.normalized(), s.force.normalized())) < 1e-4f);
    }
    frame.bands[Sub] = 1.0f; // level 0.8: twice as strong
    std::vector<Splat> strong;
    vortex.emit(frame, kDt, strong);
    CORRADE_VERIFY(std::abs(totalForce(strong) - 2.0f * totalForce(weak)) < 1e-4f * totalForce(strong));
}

void EmitterTest::zeroGainEmitsNothing() {
    EmitterParams p = defaultEmitterParams();
    for(EmitterCommon* c : {static_cast<EmitterCommon*>(&p.burst), static_cast<EmitterCommon*>(&p.vortex),
            static_cast<EmitterCommon*>(&p.jets), static_cast<EmitterCommon*>(&p.splash), static_cast<EmitterCommon*>(&p.sparks)})
        c->gain = 0.0f;
    p.ambient.enabled = false; // ambient stirs regardless of the music gains
    EmitterSet set;
    set.setParams(p);
    AudioFrame frame;
    frame.bands = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    frame.onsets = {true, true, true, true, true};
    std::vector<Splat> out;
    set.emit(frame, kDt, out);
    CORRADE_VERIFY(out.empty());
}

void EmitterTest::splashIsSeeded() {
    const auto first = [](std::uint64_t seed) {
        Splash splash;
        splash.params = defaultEmitterParams().splash;
        splash.params.seed = seed;
        AudioFrame frame;
        frame.onsets[Snare] = true;
        frame.bands[Snare] = 0.7f;
        std::vector<Splat> out;
        splash.emit(frame, kDt, out);
        return out.at(0).at;
    };
    CORRADE_VERIFY(first(5) == first(5));
    CORRADE_VERIFY(first(5) != first(6));
    const Vec2 at = first(5);
    CORRADE_VERIFY(at.x() >= 0.15f && at.x() <= 0.85f && at.y() >= 0.15f && at.y() <= 0.85f);
}

void EmitterTest::sparksScaleWithHats() {
    const auto count = [](float hat) {
        Sparks sparks;
        sparks.params = defaultEmitterParams().sparks; // threshold 0.2, count 30
        AudioFrame frame;
        frame.bands[Hat] = hat;
        std::vector<Splat> out;
        sparks.emit(frame, kDt, out);
        return out.size();
    };
    CORRADE_COMPARE(count(0.1f), 0u); // below threshold
    CORRADE_VERIFY(count(0.6f) > count(0.3f));
}

void EmitterTest::hueFollowsChroma() {
    EmitterSet set;
    AudioFrame frame;
    frame.chroma[3] = 1.0f; // pitch class 3 of 12 -> hue 0.25
    std::vector<Splat> out;
    for(int k = 0; k < 300; ++k) set.emit(frame, kDt, out);
    CORRADE_VERIFY(std::abs(set.hue() - 0.25f) < 0.01f);
}

void EmitterTest::glowFollowsLoudness() {
    EmitterSet set; // glow base 0.6, gain 0.6
    AudioFrame frame;
    frame.loudness = 0.5f;
    std::vector<Splat> out;
    set.emit(frame, kDt, out);
    CORRADE_VERIFY(std::abs(set.glow() - 0.9f) < 1e-6f);
}

void EmitterTest::applyFeedsTheSolver() {
    CpuStableFluid solver{64, 36};
    EmitterSet set;
    set.setAspect(64.0f / 36.0f);
    set.apply(kickFrame(0.5, 0.9f), kDt, solver);
    solver.step(kDt);
    CORRADE_VERIFY(solver.totalDye().r() > 0.0f);
    CORRADE_VERIFY(solver.totalAbsCurl() >= 0.0f);
}

// ambient ("default pressure") keeps the plasma moving with no music at all
void EmitterTest::ambientMovesWithoutMusic() {
    EmitterSet set;
    std::vector<Splat> out;
    AudioFrame silent;
    for(int k = 0; k < 30; ++k) {
        silent.time = k * kDt;
        set.emit(silent, kDt, out);
    }
    CORRADE_VERIFY(!out.empty());
    CORRADE_VERIFY(totalForce(out) > 0.0f);
}

void EmitterTest::ambientIsDeterministic() {
    EmitterSet a, b;
    std::vector<Splat> outA, outB;
    AudioFrame silent;
    for(int k = 0; k < 20; ++k) {
        silent.time = k * kDt;
        a.emit(silent, kDt, outA);
        b.emit(silent, kDt, outB);
    }
    CORRADE_COMPARE(outA.size(), outB.size());
    for(std::size_t k = 0; k < outA.size(); ++k) {
        CORRADE_COMPARE(outA[k].at, outB[k].at);
        CORRADE_COMPARE(outA[k].force, outB[k].force);
    }
}

void EmitterTest::musicOffSilencesMusicEmitters() {
    EmitterSet set;
    EmitterParams p = set.params();
    p.mix.music = false;
    p.ambient.enabled = false;
    set.setParams(p);
    std::vector<Splat> out;
    AudioFrame f = kickFrame(0.0, 1.0f);
    f.bands = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    f.onsets = {true, true, true, true, true};
    set.emit(f, kDt, out);
    CORRADE_VERIFY(out.empty());
}

void EmitterTest::ambientOffLeavesOnlyMusic() {
    EmitterSet set;
    EmitterParams p = set.params();
    p.ambient.enabled = false;
    set.setParams(p);
    std::vector<Splat> out;
    AudioFrame silent;
    set.emit(silent, kDt, out);
    CORRADE_VERIFY(out.empty());                // no music, no ambient
    set.emit(kickFrame(1.0, 1.0f), kDt, out);
    CORRADE_VERIFY(!out.empty());               // the kick still bursts
}

void EmitterTest::musicStrengthScalesForces() {
    const auto burstForce = [](float strength) {
        EmitterSet set;
        EmitterParams p = set.params();
        p.ambient.enabled = false;
        p.mix.musicStrength = strength;
        set.setParams(p);
        std::vector<Splat> out;
        set.emit(kickFrame(0.0, 1.0f), kDt, out);
        return totalForce(out);
    };
    CORRADE_VERIFY(burstForce(1.0f) > 0.0f);
    CORRADE_VERIFY(std::abs(burstForce(2.0f) - 2.0f * burstForce(1.0f)) < 1e-3f * burstForce(1.0f));
}

CORRADE_TEST_MAIN(EmitterTest)
