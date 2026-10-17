#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <smg/GuiBase.hh>

#include "vibe/game/ActionMap.hxx"
#include "vibe/game/FixedStepper.hxx"
#include "vibe/game/Game.hxx"
#include "vibe/game/Hud.hxx"
#include "vibe/game/InputScript.hxx"
#include "vibe/game/InputSystem.hxx"
#include "vibe/game/Lobby.hxx"
#include "vibe/game/Player.hxx"
#include "vibe/game/SdlInput.hxx"
#include "vibe/game/ViewLayout.hxx"
#include "vibe/audio/MusicPlayer.hxx"

namespace vibe::game {

struct GameConfig {
    std::string title = "vibe game";
    double tickHz = 60.0;
    int minPlayers = 1;
    int maxPlayers = 4;
    LayoutMode layout = LayoutMode::Shared;
    int gutterPx = 4;
    std::string music;         // track under the repo's assets/ dir; empty = no music
    double musicVolume = 0.6;
};

// --screenshot <file.ppm> --frames <n> --script <file>
struct AppOptions {
    std::string screenshotPath; // empty = interactive
    int screenshotFrames = 120;
    std::string scriptPath;
    bool mute = false; // --mute: no music
};

AppOptions parseAppOptions(int argc, char** argv);

// app shell: lobby -> playing <-> paused -> finished (rematch or lobby)
class GameApp : public smg::GuiBase {
public:
    enum class State { Lobby, Playing, Paused, Finished };

    GameApp(const Arguments& arguments, GameConfig config, std::unique_ptr<Game> game, AppOptions options = {});

    State state() const { return _state; }

private:
    void drawEvent() override;
    void anyEvent(SDL_Event& event) override;

    void loadScript();
    void updateLobby();
    void updatePlaying(double frameSeconds);
    void updatePaused();
    void updateFinished();
    void startMatch();
    bool anyPlayerPressed(ButtonId b) const;
    void drawViews(double alpha);
    void drawOverlay();
    void drawLobby(const View& full);
    View fullView() const;
    void writeScreenshot();

    GameConfig _config;
    AppOptions _options;
    std::unique_ptr<Game> _game;
    ActionMap _actions;
    InputSystem _input;
    Lobby _lobby;
    SdlInput _sdl;
    vibe::daw::MusicPlayer _music;
    FixedStepper _stepper;
    Hud _hud;
    std::vector<Player> _players;
    std::vector<View> _views;
    std::vector<ScriptedInput> _script;
    std::size_t _scriptNext = 0;
    State _state = State::Lobby;
    int _frame = 0;
    bool _firstFrame = true;
    std::chrono::steady_clock::time_point _lastFrame;
};

} // namespace vibe::game
