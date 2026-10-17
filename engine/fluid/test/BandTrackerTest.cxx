#include <algorithm>

#include <Corrade/TestSuite/Tester.h>

#include "vibe/fluid/BandTracker.hxx"

using namespace vibe::fluid;

namespace {
constexpr float kDt = 1.0f / 60.0f;

// feed `frames` frames of a constant raw kick value; returns the number of kick onsets
int feed(BandTracker& tracker, AudioFrame& frame, double& time, float kick, int frames) {
    int hits = 0;
    for(int k = 0; k < frames; ++k) {
        tracker.update({0.0f, kick, 0.0f, 0.0f, 0.0f}, time, kDt, frame);
        hits += frame.onsets[Kick] ? 1 : 0;
        time += kDt;
    }
    return hits;
}
} // namespace

struct BandTrackerTest : Corrade::TestSuite::Tester {
    explicit BandTrackerTest();

    void steadyToneHitsOnce();
    void pulsesHitEachTime();
    void refractoryLimitsRate();
    void levelsStayInRange();
    void silenceStaysQuiet();
    void quietAfterLoudStaysCalm();
};

BandTrackerTest::BandTrackerTest() {
    addTests({&BandTrackerTest::steadyToneHitsOnce,
        &BandTrackerTest::pulsesHitEachTime,
        &BandTrackerTest::refractoryLimitsRate,
        &BandTrackerTest::levelsStayInRange,
        &BandTrackerTest::silenceStaysQuiet,
        &BandTrackerTest::quietAfterLoudStaysCalm});
}

void BandTrackerTest::steadyToneHitsOnce() {
    BandTracker tracker;
    AudioFrame frame;
    double time = 0.0;
    CORRADE_COMPARE(feed(tracker, frame, time, 1.0f, 120), 1);
}

void BandTrackerTest::pulsesHitEachTime() {
    BandTracker tracker;
    AudioFrame frame;
    double time = 0.0;
    int hits = 0;
    for(int pulse = 0; pulse < 8; ++pulse) {
        hits += feed(tracker, frame, time, 1.0f, 3);
        hits += feed(tracker, frame, time, 0.05f, 12); // 0.25 s per pulse
    }
    CORRADE_COMPARE(hits, 8);
}

void BandTrackerTest::refractoryLimitsRate() {
    BandTracker tracker; // 0.1 s refractory
    AudioFrame frame;
    double time = 0.0;
    int hits = 0;
    for(int k = 0; k < 30; ++k) { // 1 s of 1-frame pulses every 2 frames
        hits += feed(tracker, frame, time, 1.0f, 1);
        hits += feed(tracker, frame, time, 0.0f, 1);
    }
    CORRADE_VERIFY(hits >= 5);
    CORRADE_VERIFY(hits <= 10);
}

void BandTrackerTest::levelsStayInRange() {
    BandTracker tracker;
    AudioFrame frame;
    double time = 0.0;
    for(int k = 0; k < 200; ++k) {
        tracker.update({float(k % 7), float(k % 3) * 10.0f, 0.5f, 100.0f, 0.0f}, time, kDt, frame);
        for(float b : frame.bands) CORRADE_VERIFY(b >= 0.0f && b <= 1.0f);
        time += kDt;
    }
}

void BandTrackerTest::silenceStaysQuiet() {
    BandTracker tracker;
    AudioFrame frame;
    double time = 0.0;
    feed(tracker, frame, time, 1.0f, 60);     // loud
    feed(tracker, frame, time, 1.0e-6f, 600); // 10 s of near silence: the peak decays to the floor
    CORRADE_VERIFY(frame.bands[Kick] < 0.05f);
    CORRADE_VERIFY(!frame.onsets[Kick]);
}

// review finding: after loud music, quiet input (room hiss, a fade-out) at or above the
// absolute floor must not be normalized back up to full-strength plasma
void BandTrackerTest::quietAfterLoudStaysCalm() {
    for(const float quiet : {1.0e-4f, 3.0e-4f, 1.0e-3f, 1.0e-2f}) {
        BandTracker tracker;
        AudioFrame frame;
        double time = 0.0;
        feed(tracker, frame, time, 1.0f, 600); // 10 s loud
        int lateHits = 0;
        float maxLevel = 0.0f;
        for(int k = 0; k < 1200; ++k) {        // 20 s of noisy quiet input (x0.5 .. x1.5)
            const float wobble = 0.5f + static_cast<float>((k * 37) % 101) / 100.0f;
            tracker.update({0.0f, quiet * wobble, 0.0f, 0.0f, 0.0f}, time, kDt, frame);
            time += kDt;
            if(k >= 900) {                     // the last 5 s
                lateHits += frame.onsets[Kick] ? 1 : 0;
                maxLevel = std::max(maxLevel, frame.bands[Kick]);
            }
        }
        CORRADE_ITERATION(quiet);
        CORRADE_COMPARE(lateHits, 0);
        CORRADE_VERIFY(maxLevel < 0.3f);
    }
}

CORRADE_TEST_MAIN(BandTrackerTest)
