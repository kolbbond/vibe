#include <Corrade/TestSuite/Tester.h>

#include "vibe/game/Lobby.hxx"
#include "vibe/game/Player.hxx"

using namespace vibe::game;

namespace {

const Source kLeft = Source::keyboard(Source::KeyboardLeft);
const Source kRight = Source::keyboard(Source::KeyboardRight);

// press and release a key, then run one lobby tick
bool tapLobby(InputSystem& in, Lobby& lobby, Source s, Key k) {
    in.handle(InputEvent{s, Control::key(k), 1.0f});
    in.handle(InputEvent{s, Control::key(k), 0.0f});
    in.beginTick();
    return lobby.update();
}

void padTap(InputSystem& in, int id, PadButton b) {
    in.handle(InputEvent{Source::gamepad(id), Control::pad(b), 1.0f});
    in.handle(InputEvent{Source::gamepad(id), Control::pad(b), 0.0f});
}

} // namespace

struct LobbyTest : Corrade::TestSuite::Tester {
    explicit LobbyTest();

    void confirmJoinsLowestFreeSlot();
    void sameSourceCannotJoinTwice();
    void backLeaves();
    void startNeedsMinPlayers();
    void lobbyUnplugFreesSlot();
    void midMatchUnplugDisconnects();
    void reclaimTakesLowestDisconnected();
    void confirmMidMatchWithoutDisconnectIsIgnored();
    void returnToLobbyFreesDisconnected();
    void playerReadsItsSlot();
    void slotColorsDiffer();
};

LobbyTest::LobbyTest() {
    addTests({&LobbyTest::confirmJoinsLowestFreeSlot,
        &LobbyTest::sameSourceCannotJoinTwice,
        &LobbyTest::backLeaves,
        &LobbyTest::startNeedsMinPlayers,
        &LobbyTest::lobbyUnplugFreesSlot,
        &LobbyTest::midMatchUnplugDisconnects,
        &LobbyTest::reclaimTakesLowestDisconnected,
        &LobbyTest::confirmMidMatchWithoutDisconnectIsIgnored,
        &LobbyTest::returnToLobbyFreesDisconnected,
        &LobbyTest::playerReadsItsSlot,
        &LobbyTest::slotColorsDiffer});
}

void LobbyTest::confirmJoinsLowestFreeSlot() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kRight, Key::Return);
    tapLobby(in, lobby, kLeft, Key::Space);
    CORRADE_COMPARE(lobby.joinedCount(), 2);
    CORRADE_VERIFY(lobby.slot(0).joined && lobby.slot(0).source == kRight);
    CORRADE_VERIFY(lobby.slot(1).joined && lobby.slot(1).source == kLeft);
    CORRADE_COMPARE(in.slotOf(kRight), 0);
    CORRADE_COMPARE(in.slotOf(kLeft), 1);
}

void LobbyTest::sameSourceCannotJoinTwice() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    tapLobby(in, lobby, kLeft, Key::Space);
    CORRADE_COMPARE(lobby.joinedCount(), 1);
}

void LobbyTest::backLeaves() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    tapLobby(in, lobby, kLeft, Key::Q);
    CORRADE_COMPARE(lobby.joinedCount(), 0);
    CORRADE_COMPARE(in.slotOf(kLeft), -1);
}

void LobbyTest::startNeedsMinPlayers() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 2};
    tapLobby(in, lobby, kLeft, Key::Space);
    CORRADE_VERIFY(!tapLobby(in, lobby, kLeft, Key::Escape));
    tapLobby(in, lobby, kRight, Key::Return);
    CORRADE_VERIFY(tapLobby(in, lobby, kRight, Key::Escape));
}

void LobbyTest::lobbyUnplugFreesSlot() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    in.handle(DeviceEvent{DeviceEvent::Kind::Added, 5});
    padTap(in, 5, PadButton::A);
    in.beginTick();
    lobby.update();
    CORRADE_COMPARE(lobby.joinedCount(), 1);
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 5});
    in.beginTick();
    lobby.update();
    CORRADE_COMPARE(lobby.joinedCount(), 0);
}

void LobbyTest::midMatchUnplugDisconnects() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    padTap(in, 5, PadButton::A);
    in.beginTick();
    lobby.update();
    CORRADE_COMPARE(lobby.joinedCount(), 2);
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 5});
    in.beginTick();
    lobby.updateInMatch();
    CORRADE_VERIFY(lobby.slot(1).joined);
    CORRADE_VERIFY(!lobby.slot(1).connected);
    CORRADE_VERIFY(lobby.anyDisconnected());
    CORRADE_COMPARE(in.slotOf(Source::gamepad(5)), -1);
}

void LobbyTest::reclaimTakesLowestDisconnected() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    padTap(in, 5, PadButton::A);
    in.beginTick();
    lobby.update();
    padTap(in, 6, PadButton::A);
    in.beginTick();
    lobby.update();
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 5});
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 6});
    in.beginTick();
    lobby.updateInMatch();
    padTap(in, 9, PadButton::A);
    in.beginTick();
    lobby.updateInMatch();
    CORRADE_VERIFY(lobby.slot(0).connected);
    CORRADE_VERIFY(lobby.slot(0).source == Source::gamepad(9));
    CORRADE_VERIFY(!lobby.slot(1).connected);
    CORRADE_COMPARE(in.slotOf(Source::gamepad(9)), 0);
}

void LobbyTest::confirmMidMatchWithoutDisconnectIsIgnored() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    in.handle(InputEvent{kRight, Control::key(Key::Return), 1.0f});
    in.beginTick();
    lobby.updateInMatch();
    CORRADE_COMPARE(lobby.joinedCount(), 1);
    CORRADE_COMPARE(in.slotOf(kRight), -1);
}

void LobbyTest::returnToLobbyFreesDisconnected() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    padTap(in, 5, PadButton::A);
    in.beginTick();
    lobby.update();
    in.handle(DeviceEvent{DeviceEvent::Kind::Removed, 5});
    in.beginTick();
    lobby.updateInMatch();
    lobby.returnToLobby();
    CORRADE_COMPARE(lobby.joinedCount(), 1);
    CORRADE_VERIFY(!lobby.anyDisconnected());
}

void LobbyTest::playerReadsItsSlot() {
    const ActionMap actions;
    InputSystem in{actions};
    Lobby lobby{in, 1};
    tapLobby(in, lobby, kLeft, Key::Space);
    const Player p{in, lobby, 0};
    CORRADE_COMPARE(p.slot(), 0);
    CORRADE_VERIFY(p.connected());
    CORRADE_VERIFY(p.source() == kLeft);
    in.handle(InputEvent{kLeft, Control::key(Key::Q), 1.0f});
    in.beginTick();
    CORRADE_VERIFY(p.pressed(ActionMap::Back));
    CORRADE_VERIFY(p.color() == Lobby::slotColor(0));
}

void LobbyTest::slotColorsDiffer() {
    for(int a = 0; a < 4; ++a)
        for(int b = a + 1; b < 4; ++b) CORRADE_VERIFY(Lobby::slotColor(a) != Lobby::slotColor(b));
}

CORRADE_TEST_MAIN(LobbyTest)
