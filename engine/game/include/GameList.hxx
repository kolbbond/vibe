#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace vibe::game {

// one launchable game from build/games.json (games/<name>/game.json + the built binary)
struct GameEntry {
    std::string name; // directory, executable and window class
    std::string title;
    std::string players;
    std::string description;
    std::string binary; // absolute path to the executable
    std::vector<std::string> args;
};

// parse build/games.json (an array of entries). On failure `out` is empty and
// `error` is "entry <i>: ..." or "games.json: ..."
bool loadGameList(std::string_view json, std::vector<GameEntry>& out, std::string& error);

} // namespace vibe::game
