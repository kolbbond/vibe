#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>

#include "vibe/game/InputSystem.hxx"
#include "vibe/game/Lobby.hxx"

namespace vibe::game {

// a joined player as the game sees it; input reads are empty while disconnected
class Player {
public:
    Player(const InputSystem& input, const Lobby& lobby, int slot) : _input{&input}, _lobby{&lobby}, _slot{slot} {}

    int slot() const { return _slot; }
    Magnum::Color4 color() const { return Lobby::slotColor(_slot); }
    Source source() const { return _lobby->slot(_slot).source; }
    bool connected() const { return _lobby->slot(_slot).connected; }

    bool down(ButtonId b) const { return connected() && _input->down(_slot, b); }
    bool pressed(ButtonId b) const { return connected() && _input->pressed(_slot, b); }
    bool released(ButtonId b) const { return connected() && _input->released(_slot, b); }
    float axis(AxisId a) const { return connected() ? _input->axis(_slot, a) : 0.0f; }

private:
    const InputSystem* _input;
    const Lobby* _lobby;
    int _slot;
};

} // namespace vibe::game
