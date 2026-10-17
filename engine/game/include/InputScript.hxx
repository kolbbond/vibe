#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "vibe/game/Input.hxx"

namespace vibe::game {

struct ScriptedInput {
    int frame = 0;
    InputEvent event;
};

// scripted input for smoke tests, one event per line:
//   frame source control value      e.g. "12 kl key:Space 1", "40 pad0 axis:LeftX -1"
// source: kl | kr | pad<N>; control: key:<Key> | btn:<PadButton> | axis:<PadAxis>
// '#' starts a comment; frames must not decrease. On failure `error` is "line <n>: ..."
bool parseInputScript(std::string_view text, std::vector<ScriptedInput>& out, std::string& error);

} // namespace vibe::game
