#include <cmath>

#include <Corrade/TestSuite/Tester.h>

#include "vibe/game/FixedStepper.hxx"

using namespace vibe::game;

struct FixedStepperTest : Corrade::TestSuite::Tester {
    explicit FixedStepperTest();

    void steadyFramesAverageToTickRate();
    void speedMultiplies();
    void pauseRunsNothing();
    void hitchIsClamped();
    void alphaIsFractionOfTick();
    void resetClears();
};

FixedStepperTest::FixedStepperTest() {
    addTests({&FixedStepperTest::steadyFramesAverageToTickRate,
        &FixedStepperTest::speedMultiplies,
        &FixedStepperTest::pauseRunsNothing,
        &FixedStepperTest::hitchIsClamped,
        &FixedStepperTest::alphaIsFractionOfTick,
        &FixedStepperTest::resetClears});
}

void FixedStepperTest::steadyFramesAverageToTickRate() {
    FixedStepper s{20.0};
    int ticks = 0;
    for(int f = 0; f < 600; ++f) ticks += s.advance(1.0 / 60.0, 1.0); // 10 s at 60 fps
    CORRADE_VERIFY(ticks >= 199 && ticks <= 200);
}

void FixedStepperTest::speedMultiplies() {
    FixedStepper s{20.0};
    int ticks = 0;
    for(int f = 0; f < 60; ++f) ticks += s.advance(1.0 / 60.0, 4.0); // 1 s at 4x
    CORRADE_VERIFY(ticks >= 79 && ticks <= 80);
}

void FixedStepperTest::pauseRunsNothing() {
    FixedStepper s{20.0};
    for(int f = 0; f < 100; ++f) CORRADE_COMPARE(s.advance(1.0 / 60.0, 0.0), 0);
}

void FixedStepperTest::hitchIsClamped() {
    FixedStepper s{20.0};
    CORRADE_COMPARE(s.advance(10.0, 1.0), 5);  // 0.25 s * 20 Hz
    s.reset();
    CORRADE_COMPARE(s.advance(10.0, 4.0), 5);  // clamp applies to frameDt * speed
}

void FixedStepperTest::alphaIsFractionOfTick() {
    FixedStepper s{20.0};
    CORRADE_COMPARE(s.advance(0.075, 1.0), 1); // 1.5 ticks
    CORRADE_VERIFY(std::abs(s.alpha() - 0.5) < 1e-9);
    CORRADE_VERIFY(s.alpha() >= 0.0 && s.alpha() < 1.0);
}

void FixedStepperTest::resetClears() {
    FixedStepper s{20.0};
    s.advance(0.04, 1.0);
    s.reset();
    CORRADE_COMPARE(s.alpha(), 0.0);
}

CORRADE_TEST_MAIN(FixedStepperTest)
