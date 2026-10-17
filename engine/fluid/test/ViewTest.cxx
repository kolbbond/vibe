#include <Corrade/TestSuite/Tester.h>

#include "vibe/fluid/View.hxx"

using namespace vibe::fluid;

struct ViewTest : Corrade::TestSuite::Tester {
    explicit ViewTest();

    void identityByDefault();
    void zoomAtKeepsCursorFixed();
    void roundTrip();
    void zoomIsClamped();
    void centerIsClamped();
};

ViewTest::ViewTest() {
    addTests({&ViewTest::identityByDefault, &ViewTest::zoomAtKeepsCursorFixed, &ViewTest::roundTrip, &ViewTest::zoomIsClamped,
        &ViewTest::centerIsClamped});
}

void ViewTest::identityByDefault() {
    FluidView v;
    CORRADE_COMPARE(v.toField({0.25f, 0.75f}), (Vec2{0.25f, 0.75f}));
}

// the field point under the mouse stays under the mouse while zooming
void ViewTest::zoomAtKeepsCursorFixed() {
    FluidView v;
    const Vec2 cursor{0.8f, 0.3f};
    const Vec2 before = v.toField(cursor);
    v.zoomAt(cursor, 2.0f);
    CORRADE_COMPARE(v.zoom, 2.0f);
    CORRADE_COMPARE(v.toField(cursor), before);
    // zoomed 2x: a quarter-screen step covers an eighth of the field
    CORRADE_COMPARE(v.toField({0.55f, 0.3f}).x() - v.toField({0.3f, 0.3f}).x(), 0.125f);
}

void ViewTest::roundTrip() {
    FluidView v;
    v.zoomAt({0.3f, 0.6f}, 3.0f);
    const Vec2 s{0.42f, 0.17f};
    const Vec2 back = v.toScreen(v.toField(s));
    CORRADE_COMPARE(back, s);
}

void ViewTest::zoomIsClamped() {
    FluidView v;
    v.zoomAt({0.5f, 0.5f}, 1000.0f);
    CORRADE_COMPARE(v.zoom, FluidView::kMaxZoom);
    v.zoomAt({0.5f, 0.5f}, 1.0e-6f);
    CORRADE_COMPARE(v.zoom, FluidView::kMinZoom);
}

// panning can't lose the field entirely: the center stays on it
void ViewTest::centerIsClamped() {
    FluidView v;
    v.zoom = 4.0f;
    v.pan({10.0f, -10.0f}); // drag the picture far right and down: the view heads left and up, then stops at the edge
    CORRADE_COMPARE(v.center, (Vec2{0.0f, 1.0f}));
}

CORRADE_TEST_MAIN(ViewTest)
