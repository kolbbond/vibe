#pragma once

#include <string>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>

#include "vibe/game/InputSystem.hxx"

namespace vibe::game {

struct PlayerSlot {
    bool joined = false;
    bool connected = true;
    Source source;
};

// press-to-join slots; runs on InputSystem ticks
class Lobby {
public:
    Lobby(InputSystem& input, int minPlayers);

    // lobby screen tick: joins (confirm from an unclaimed source), leaves (back), unplugs.
    // returns true when a joined player pressed start and enough players joined
    bool update();
    // in-match tick: an unplugged pad marks its player disconnected; confirm from an
    // unclaimed source then takes over the lowest disconnected slot
    void updateInMatch();
    // back to the lobby screen: disconnected players lose their slot
    void returnToLobby();

    int maxPlayers() const { return static_cast<int>(_slots.size()); }
    int minPlayers() const { return _minPlayers; }
    const PlayerSlot& slot(int i) const { return _slots[i]; }
    int joinedCount() const;
    std::vector<int> joinedSlots() const;
    bool anyDisconnected() const;

    static Magnum::Color4 slotColor(int slot);

private:
    void join(int slot, Source source);
    void leave(int slot);
    int connectedSlotOfGamepad(int gamepadId) const;

    InputSystem& _input;
    int _minPlayers;
    std::vector<PlayerSlot> _slots;
};

// human-readable source name for lobby panels
std::string sourceName(const Source& source);

} // namespace vibe::game
