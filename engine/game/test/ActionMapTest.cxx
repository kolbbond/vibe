#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "vibe/game/ActionMap.hxx"

using namespace vibe::game;

struct ActionMapTest : Corrade::TestSuite::Tester {
    explicit ActionMapTest();

    void builtInsExist();
    void builtInDefaults();
    void gameButtonGetsNextId();
    void axisRoles();
    void usesKeyPerSide();
};

ActionMapTest::ActionMapTest() {
    addTests({&ActionMapTest::builtInsExist,
        &ActionMapTest::builtInDefaults,
        &ActionMapTest::gameButtonGetsNextId,
        &ActionMapTest::axisRoles,
        &ActionMapTest::usesKeyPerSide});
}

void ActionMapTest::builtInsExist() {
    const ActionMap map;
    CORRADE_COMPARE(map.buttonCount(), 3);
    CORRADE_COMPARE(map.axisCount(), 2);
    CORRADE_COMPARE(map.buttonName(ActionMap::Confirm), "confirm");
    CORRADE_COMPARE(map.buttonName(ActionMap::Back), "back");
    CORRADE_COMPARE(map.buttonName(ActionMap::Start), "start");
    CORRADE_COMPARE(map.axisName(ActionMap::NavX), "navX");
    CORRADE_COMPARE(map.axisName(ActionMap::NavY), "navY");
}

void ActionMapTest::builtInDefaults() {
    const ActionMap map;
    CORRADE_VERIFY(map.buttonBound(ActionMap::Confirm, Source::KeyboardLeft, Control::key(Key::Space)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Confirm, Source::KeyboardRight, Control::key(Key::Return)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Confirm, Source::Gamepad, Control::pad(PadButton::A)));
    CORRADE_VERIFY(!map.buttonBound(ActionMap::Confirm, Source::KeyboardLeft, Control::key(Key::Return)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Back, Source::KeyboardRight, Control::key(Key::Backspace)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Start, Source::KeyboardLeft, Control::key(Key::Escape)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Start, Source::KeyboardRight, Control::key(Key::Escape)));
    CORRADE_VERIFY(map.buttonBound(ActionMap::Start, Source::Gamepad, Control::pad(PadButton::Start)));
}

void ActionMapTest::gameButtonGetsNextId() {
    ActionMap map;
    const ButtonId boost = map.button("boost")
        .key(Source::KeyboardLeft, Key::LShift)
        .pad(PadButton::X);
    CORRADE_COMPARE(boost.index, 3);
    CORRADE_COMPARE(map.buttonCount(), 4);
    CORRADE_COMPARE(map.buttonName(boost), "boost");
    CORRADE_VERIFY(map.buttonBound(boost, Source::KeyboardLeft, Control::key(Key::LShift)));
    CORRADE_VERIFY(map.buttonBound(boost, Source::Gamepad, Control::pad(PadButton::X)));
    CORRADE_VERIFY(!map.buttonBound(boost, Source::KeyboardRight, Control::key(Key::LShift)));
}

void ActionMapTest::axisRoles() {
    ActionMap map;
    const AxisId steer = map.axis("steer")
        .keys(Source::KeyboardRight, Key::Left, Key::Right)
        .padAxis(PadAxis::RightX)
        .padButtons(PadButton::X, PadButton::B);
    CORRADE_COMPARE(steer.index, 2);
    CORRADE_VERIFY(map.axisRole(steer, Source::KeyboardRight, Control::key(Key::Left)) == AxisRole::Negative);
    CORRADE_VERIFY(map.axisRole(steer, Source::KeyboardRight, Control::key(Key::Right)) == AxisRole::Positive);
    CORRADE_VERIFY(map.axisRole(steer, Source::Gamepad, Control::axis(PadAxis::RightX)) == AxisRole::Analog);
    CORRADE_VERIFY(map.axisRole(steer, Source::Gamepad, Control::pad(PadButton::B)) == AxisRole::Positive);
    CORRADE_VERIFY(map.axisRole(steer, Source::KeyboardLeft, Control::key(Key::Left)) == AxisRole::None);
    CORRADE_VERIFY(map.axisRole(ActionMap::NavY, Source::KeyboardLeft, Control::key(Key::W)) == AxisRole::Negative);
    CORRADE_VERIFY(map.axisRole(ActionMap::NavY, Source::Gamepad, Control::axis(PadAxis::LeftY)) == AxisRole::Analog);
}

void ActionMapTest::usesKeyPerSide() {
    ActionMap map;
    map.button("boost").key(Source::KeyboardLeft, Key::LShift);
    CORRADE_VERIFY(map.usesKey(Source::KeyboardLeft, Key::LShift));
    CORRADE_VERIFY(!map.usesKey(Source::KeyboardRight, Key::LShift));
    CORRADE_VERIFY(map.usesKey(Source::KeyboardLeft, Key::Escape));
    CORRADE_VERIFY(map.usesKey(Source::KeyboardRight, Key::Escape));
    CORRADE_VERIFY(map.usesKey(Source::KeyboardRight, Key::Up)); // navY built-in
}

CORRADE_TEST_MAIN(ActionMapTest)
