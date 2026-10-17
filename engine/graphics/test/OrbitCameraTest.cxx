#include <cmath>

#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Matrix4.h>

#include "vibe/graphics/OrbitCamera.hxx"

using namespace vibe::gfx;
using Magnum::Vector3;

struct OrbitCameraTest : Corrade::TestSuite::Tester {
    explicit OrbitCameraTest();

    void pitchClamps();
    void eyeAtDistance();
    void yawQuarterTurn();
    void zoomClamps();
    void viewLooksAtTarget();
};

OrbitCameraTest::OrbitCameraTest() {
    addTests({&OrbitCameraTest::pitchClamps, &OrbitCameraTest::eyeAtDistance, &OrbitCameraTest::yawQuarterTurn,
        &OrbitCameraTest::zoomClamps, &OrbitCameraTest::viewLooksAtTarget});
}

void OrbitCameraTest::pitchClamps() {
    OrbitCamera c;
    c.orbit(0.0f, 10.0f);
    CORRADE_COMPARE(c.pitch, OrbitCamera::kMaxPitch);
    c.orbit(0.0f, -20.0f);
    CORRADE_COMPARE(c.pitch, -OrbitCamera::kMaxPitch);
}

void OrbitCameraTest::eyeAtDistance() {
    OrbitCamera c;
    c.target = Vector3{1.0f, 2.0f, 3.0f};
    c.orbit(0.7f, 0.3f);
    CORRADE_COMPARE((c.eye() - c.target).length(), c.distance);
}

void OrbitCameraTest::yawQuarterTurn() {
    OrbitCamera c;
    c.pitch = 0.0f;
    CORRADE_COMPARE(c.eye(), (Vector3{0.0f, 0.0f, c.distance}));
    c.orbit(1.5707963f, 0.0f);
    CORRADE_COMPARE(c.eye(), (Vector3{c.distance, 0.0f, 0.0f}));
}

void OrbitCameraTest::zoomClamps() {
    OrbitCamera c;
    c.zoom(1000.0f);
    CORRADE_COMPARE(c.distance, OrbitCamera::kMinDistance);
    c.zoom(0.0001f);
    CORRADE_COMPARE(c.distance, OrbitCamera::kMaxDistance);
}

// the target lands on the camera's -z axis
void OrbitCameraTest::viewLooksAtTarget() {
    OrbitCamera c;
    c.target = Vector3{0.5f, -0.2f, 0.1f};
    c.orbit(0.4f, 0.2f);
    const Vector3 t = c.view().transformPoint(c.target);
    CORRADE_COMPARE(t.xy(), (Magnum::Vector2{0.0f}));
    CORRADE_COMPARE(t.z(), -c.distance);
}

CORRADE_TEST_MAIN(OrbitCameraTest)
