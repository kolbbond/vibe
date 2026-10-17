#include "vibe/game/Lobby.hxx"

#include <algorithm>

namespace vibe::game {

using namespace Magnum::Math::Literals;

Lobby::Lobby(InputSystem& input, int minPlayers)
    : _input{input}, _minPlayers{std::max(1, minPlayers)}, _slots(static_cast<std::size_t>(input.maxPlayers())) {}

Magnum::Color4 Lobby::slotColor(int slot) {
    switch(slot) {
    case 0: return 0x00e5ffff_rgbaf; // cyan
    case 1: return 0xff8a00ff_rgbaf; // orange
    case 2: return 0xff2bd6ff_rgbaf; // magenta
    default: return 0x9dff00ff_rgbaf; // lime
    }
}

int Lobby::joinedCount() const {
    return static_cast<int>(std::count_if(_slots.begin(), _slots.end(), [](const PlayerSlot& s) { return s.joined; }));
}

std::vector<int> Lobby::joinedSlots() const {
    std::vector<int> out;
    for(int i = 0; i < maxPlayers(); ++i)
        if(_slots[i].joined) out.push_back(i);
    return out;
}

bool Lobby::anyDisconnected() const {
    return std::any_of(_slots.begin(), _slots.end(), [](const PlayerSlot& s) { return s.joined && !s.connected; });
}

void Lobby::join(int slot, Source source) {
    _slots[slot] = PlayerSlot{true, true, source};
    _input.assign(slot, source);
}

void Lobby::leave(int slot) {
    _slots[slot] = PlayerSlot{};
    _input.release(slot);
}

int Lobby::connectedSlotOfGamepad(int gamepadId) const {
    for(int i = 0; i < maxPlayers(); ++i)
        if(_slots[i].joined && _slots[i].connected && _slots[i].source == Source::gamepad(gamepadId)) return i;
    return -1;
}

bool Lobby::update() {
    for(const DeviceEvent& e : _input.deviceEvents()) {
        if(e.kind != DeviceEvent::Kind::Removed) continue;
        const int s = connectedSlotOfGamepad(e.gamepadId);
        if(s >= 0) leave(s);
    }

    bool start = false;
    for(int s = 0; s < maxPlayers(); ++s) {
        if(!_slots[s].joined) continue;
        if(_input.pressed(s, ActionMap::Back))
            leave(s);
        else if(_input.pressed(s, ActionMap::Start))
            start = true;
    }

    for(const InputSystem::UnclaimedPress& p : _input.unclaimedPresses()) {
        if(p.button.index != ActionMap::Confirm.index || _input.slotOf(p.source) >= 0) continue;
        const auto free = std::find_if(_slots.begin(), _slots.end(), [](const PlayerSlot& s) { return !s.joined; });
        if(free == _slots.end()) break;
        join(static_cast<int>(free - _slots.begin()), p.source);
    }

    return start && joinedCount() >= _minPlayers;
}

void Lobby::updateInMatch() {
    for(const DeviceEvent& e : _input.deviceEvents()) {
        if(e.kind != DeviceEvent::Kind::Removed) continue;
        const int s = connectedSlotOfGamepad(e.gamepadId);
        if(s < 0) continue;
        _slots[s].connected = false;
        _input.release(s);
    }

    for(const InputSystem::UnclaimedPress& p : _input.unclaimedPresses()) {
        if(p.button.index != ActionMap::Confirm.index || _input.slotOf(p.source) >= 0) continue;
        const auto lost = std::find_if(_slots.begin(), _slots.end(), [](const PlayerSlot& s) { return s.joined && !s.connected; });
        if(lost == _slots.end()) break;
        lost->source = p.source;
        lost->connected = true;
        _input.assign(static_cast<int>(lost - _slots.begin()), p.source);
    }
}

void Lobby::returnToLobby() {
    for(int s = 0; s < maxPlayers(); ++s)
        if(_slots[s].joined && !_slots[s].connected) leave(s);
}

std::string sourceName(const Source& source) {
    switch(source.kind) {
    case Source::KeyboardLeft: return "keyboard (WASD)";
    case Source::KeyboardRight: return "keyboard (arrows)";
    case Source::Gamepad: return "gamepad " + std::to_string(source.gamepadId);
    }
    return "unknown";
}

} // namespace vibe::game
