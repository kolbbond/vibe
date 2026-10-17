#pragma once

#include <array>
#include <map>

#include <SDL.h>

#include "vibe/game/ActionMap.hxx"
#include "vibe/game/InputSystem.hxx"

namespace vibe::game {

// the only code reading SDL input: polls keyboard/controller state each frame,
// diffs it and feeds InputSystem; handles controller hotplug events
class SdlInput {
public:
    explicit SdlInput(const ActionMap& actions);
    ~SdlInput();
    SdlInput(const SdlInput&) = delete;
    SdlInput& operator=(const SdlInput&) = delete;

    // SDL_CONTROLLERDEVICEADDED / REMOVED; returns true when handled
    bool handleEvent(const SDL_Event& event, InputSystem& input);
    // emit InputEvents for everything that changed since the previous call
    void poll(InputSystem& input);

private:
    struct Pad {
        SDL_GameController* controller = nullptr;
        std::array<bool, kPadButtonCount> buttons{};
        std::array<float, kPadAxisCount> axes{};
    };

    const ActionMap& _actions;
    std::array<Uint8, SDL_NUM_SCANCODES> _keys{};
    std::map<int, Pad> _pads; // by joystick instance id
    bool _initialized = false;
};

} // namespace vibe::game
