#include <cmath>

#include <Corrade/TestSuite/Tester.h>

#include "vibe/game/InputSystem.hxx"

using namespace vibe::game;

namespace {

const Source kLeft = Source::keyboard(Source::KeyboardLeft);
const Source kRight = Source::keyboard(Source::KeyboardRight);

void key(InputSystem& in, Source s, Key k, bool down) {
    in.handle(InputEvent{s, Control::key(k), down ? 1.0f : 0.0f});
}

void padButton(InputSystem& in, int id, PadButton b, bool down) {
    in.handle(InputEvent{Source::gamepad(id), Control::pad(b), down ? 1.0f : 0.0f});
}

void padAxis(InputSystem& in, int id, PadAxis a, float value) {
    in.handle(InputEvent{Source::gamepad(id), Control::axis(a), value});
}

} // namespace

struct InputSystemTest : Corrade::TestSuite::Tester {
    explicit InputSystemTest();

    void unassignedSlotReadsNothing();
    void pressMapsToAssignedSlot();
    void pressedLastsOneTick();
    void tapBetweenTicksIsNotLost();
    void pressWaitsForNextTick();
    void otherKeyboardSideIsIgnored();
    void axisFromKeyPair();
    void padAxisDeadzone();
    void digitalBeatsAnalog();
    void unclaimedConfirmIsReported();
    void assignDoesNotReplayHeldButton();
    void gamepadRemovalReleasesButtons();
};

InputSystemTest::InputSystemTest() {
    addTests({&InputSystemTest::unassignedSlotReadsNothing,
        &InputSystemTest::pressMapsToAssignedSlot,
        &InputSystemTest::pressedLastsOneTick,
        &InputSystemTest::tapBetweenTicksIsNotLost,
        &InputSystemTest::pressWaitsForNextTick,
        &InputSystemTest::otherKeyboardSideIsIgnored,
        &InputSystemTest::axisFromKeyPair,
        &InputSystemTest::padAxisDeadzone,
        &InputSystemTest::digitalBeatsAnalog,
        &InputSystemTest::unclaimedConfirmIsReported,
        &InputSystemTest::assignDoesNotReplayHeldButton,
        &InputSystemTest::gamepadRemovalReleasesButtons});
}

void InputSystemTest::unassignedSlotReadsNothing() {
    const ActionMap actions;
    InputSystem in{actions};
    key(in, kLeft, Key::Space, true);
    in.beginTick();
    CORRADE_VERIFY(!in.assigned(0));
    CORRADE_VERIFY(!in.down(0, ActionMap::Confirm));
    CORRADE_VERIFY(!in.pressed(0, ActionMap::Confirm));
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), 0.0f);
}

void InputSystemTest::pressMapsToAssignedSlot() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    CORRADE_COMPARE(in.slotOf(kLeft), 0);
    key(in, kLeft, Key::Space, true);
    in.beginTick();
    CORRADE_VERIFY(in.pressed(0, ActionMap::Confirm));
    CORRADE_VERIFY(in.down(0, ActionMap::Confirm));
    CORRADE_VERIFY(!in.released(0, ActionMap::Confirm));
}

void InputSystemTest::pressedLastsOneTick() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    key(in, kLeft, Key::Space, true);
    in.beginTick();
    CORRADE_VERIFY(in.pressed(0, ActionMap::Confirm));
    in.beginTick(); // a second tick in the same frame
    CORRADE_VERIFY(!in.pressed(0, ActionMap::Confirm));
    CORRADE_VERIFY(in.down(0, ActionMap::Confirm));
}

void InputSystemTest::tapBetweenTicksIsNotLost() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    key(in, kLeft, Key::Space, true);
    key(in, kLeft, Key::Space, false);
    in.beginTick();
    CORRADE_VERIFY(in.pressed(0, ActionMap::Confirm));
    CORRADE_VERIFY(in.released(0, ActionMap::Confirm));
    CORRADE_VERIFY(!in.down(0, ActionMap::Confirm));
}

void InputSystemTest::pressWaitsForNextTick() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    in.beginTick();
    key(in, kLeft, Key::Space, true);
    // frames with zero ticks: nothing latched yet
    CORRADE_VERIFY(!in.pressed(0, ActionMap::Confirm));
    in.beginTick();
    CORRADE_VERIFY(in.pressed(0, ActionMap::Confirm));
}

void InputSystemTest::otherKeyboardSideIsIgnored() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    key(in, kRight, Key::Return, true);
    in.beginTick();
    CORRADE_VERIFY(!in.pressed(0, ActionMap::Confirm));
}

void InputSystemTest::axisFromKeyPair() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, kLeft);
    key(in, kLeft, Key::A, true);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), -1.0f);
    key(in, kLeft, Key::D, true);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), 0.0f);
    key(in, kLeft, Key::A, false);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), 1.0f);
}

void InputSystemTest::padAxisDeadzone() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, Source::gamepad(7));
    padAxis(in, 7, PadAxis::LeftX, 0.2f);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), 0.0f);
    padAxis(in, 7, PadAxis::LeftX, -1.0f);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), -1.0f);
    padAxis(in, 7, PadAxis::LeftX, 0.625f);
    in.beginTick();
    CORRADE_VERIFY(std::abs(in.axis(0, ActionMap::NavX) - 0.5f) < 1e-6f);
}

void InputSystemTest::digitalBeatsAnalog() {
    const ActionMap actions;
    InputSystem in{actions};
    in.assign(0, Source::gamepad(2));
    padAxis(in, 2, PadAxis::LeftX, 0.7f);
    padButton(in, 2, PadButton::DpadLeft, true);
    in.beginTick();
    CORRADE_COMPARE(in.axis(0, ActionMap::NavX), -1.0f);
}

void InputSystemTest::unclaimedConfirmIsReported() {
    const ActionMap actions;
    InputSystem in{actions};
    key(in, kRight, Key::Return, true);
    in.beginTick();
    CORRADE_COMPARE(in.unclaimedPresses().size(), 1u);
    CORRADE_VERIFY(in.unclaimedPresses()[0].source == kRight);
    CORRADE_COMPARE(in.unclaimedPresses()[0].button.index, ActionMap::Confirm.index);
    in.beginTick();
    CORRADE_VERIFY(in.unclaimedPresses().empty());
}

void InputSystemTest::assignDoesNotReplayHeldButton() {
    const ActionMap actions;
    InputSystem in{actions};
    key(in, kLeft, Key::Space, true);
    in.beginTick();
    in.assign(0, kLeft);
    in.beginTick();
    CORRADE_VERIFY(!in.pressed(0, ActionMap::Confirm));
    CORRADE_VERIFY(in.down(0, ActionMap::Confirm));
}

void InputSystemTest::gamepadRemovalReleasesButtons() {
    const ActionMap actions;
    InputSystem in{actions};
    in.handle(DeviceEvent{DeviceEvent::Kind::Added, 3});
    in.assign(0, Source::gamepad(3));
    padButton(in, 3, PadButton::A, true);
    in.beginTick();
    CORRADE_VERIFY(in.down(0, ActionMap::Confirm));
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 3});
    in.beginTick();
    CORRADE_VERIFY(in.released(0, ActionMap::Confirm));
    CORRADE_VERIFY(!in.down(0, ActionMap::Confirm));
    CORRADE_COMPARE(in.deviceEvents().size(), 1u);
    CORRADE_VERIFY(in.deviceEvents()[0].kind == DeviceEvent::Kind::Removed);
}

CORRADE_TEST_MAIN(InputSystemTest)
