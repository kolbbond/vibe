#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Matrix3.h>

#include "vibe/graphics/Camera2D.hxx"

using namespace vibe::gfx;
using Magnum::Range2D;
using Magnum::Vector2;
using Magnum::Vector2i;

struct Camera2DTest : Corrade::TestSuite::Tester {
    explicit Camera2DTest();

    void projectionMapsVisibleEdgesToClip();
    void screenCornersMatchVisibleRect();
    void screenWorldRoundTrip();
    void fitWideView();
    void fitTallView();
};

Camera2DTest::Camera2DTest() {
    addTests({&Camera2DTest::projectionMapsVisibleEdgesToClip,
        &Camera2DTest::screenCornersMatchVisibleRect,
        &Camera2DTest::screenWorldRoundTrip,
        &Camera2DTest::fitWideView,
        &Camera2DTest::fitTallView});
}

void Camera2DTest::projectionMapsVisibleEdgesToClip() {
    const Camera2D cam{{10.0f, 5.0f}, 4.0f}; // aspect 2 -> 8 x 4 world units visible
    const auto p = cam.projection(2.0f);
    CORRADE_COMPARE(p.transformPoint({14.0f, 7.0f}), (Vector2{1.0f, 1.0f}));
    CORRADE_COMPARE(p.transformPoint({6.0f, 3.0f}), (Vector2{-1.0f, -1.0f}));
    CORRADE_COMPARE(p.transformPoint({10.0f, 5.0f}), (Vector2{0.0f, 0.0f}));
}

void Camera2DTest::screenCornersMatchVisibleRect() {
    const Camera2D cam{{10.0f, 5.0f}, 4.0f};
    const Vector2i view{800, 400};
    CORRADE_COMPARE(cam.screenToWorld({0.0f, 0.0f}, view), (Vector2{6.0f, 7.0f}));
    CORRADE_COMPARE(cam.screenToWorld({800.0f, 400.0f}, view), (Vector2{14.0f, 3.0f}));
}

void Camera2DTest::screenWorldRoundTrip() {
    const Camera2D cam{{-3.0f, 2.0f}, 10.0f};
    const Vector2i view{640, 480};
    const Vector2 px{123.0f, 321.0f};
    CORRADE_COMPARE(cam.worldToScreen(cam.screenToWorld(px, view), view), px);
}

void Camera2DTest::fitWideView() {
    const Camera2D cam = Camera2D::fit(Range2D{{0.0f, 0.0f}, {96.0f, 64.0f}}, 2.0f);
    CORRADE_COMPARE(cam.height(), 64.0f); // 96 / 2 = 48 < 64: height limits
    CORRADE_COMPARE(cam.center(), (Vector2{48.0f, 32.0f}));
}

void Camera2DTest::fitTallView() {
    const Camera2D cam = Camera2D::fit(Range2D{{0.0f, 0.0f}, {96.0f, 64.0f}}, 1.0f);
    CORRADE_COMPARE(cam.height(), 96.0f); // width limits
}

CORRADE_TEST_MAIN(Camera2DTest)
