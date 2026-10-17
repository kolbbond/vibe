#pragma once

#include <cstdint>
#include <vector>

#include "vibe/game/ActionMap.hxx"
#include "vibe/game/Player.hxx"
#include "vibe/game/ViewLayout.hxx"

namespace vibe::game {

class Hud;

enum class Status : std::uint8_t { Running, Finished };

// what a game implements; GameApp owns the lobby, pause and results flow
class Game {
public:
    virtual ~Game() = default;

    // called once, before the lobby opens
    virtual void declareActions(ActionMap& actions) = 0;
    // lobby finished, or rematch with the same players
    virtual void start(const std::vector<Player>& players) = 0;
    // one fixed-timestep tick; return Finished when the match is over
    virtual Status tick(double dt, const std::vector<Player>& players) = 0;
    // viewport and scissor are already set to view.viewport and cleared
    virtual void drawView(const View& view, double alpha) = 0;
    virtual void drawHud(Hud& hud, const View& view) {
        static_cast<void>(hud);
        static_cast<void>(view);
    }
};

} // namespace vibe::game
