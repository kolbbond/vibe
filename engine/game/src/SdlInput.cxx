#include "vibe/game/SdlInput.hxx"

#include <algorithm>
#include <cmath>

namespace vibe::game {

static_assert(kKeyCount == SDL_NUM_SCANCODES, "Key values must be SDL scancodes");
static_assert(int(Key::A) == SDL_SCANCODE_A && int(Key::W) == SDL_SCANCODE_W && int(Key::Space) == SDL_SCANCODE_SPACE &&
                  int(Key::Return) == SDL_SCANCODE_RETURN && int(Key::Escape) == SDL_SCANCODE_ESCAPE &&
                  int(Key::Backspace) == SDL_SCANCODE_BACKSPACE && int(Key::Up) == SDL_SCANCODE_UP &&
                  int(Key::LShift) == SDL_SCANCODE_LSHIFT && int(Key::RShift) == SDL_SCANCODE_RSHIFT,
    "Key values must be SDL scancodes");
static_assert(int(PadButton::A) == SDL_CONTROLLER_BUTTON_A && int(PadButton::Start) == SDL_CONTROLLER_BUTTON_START &&
                  int(PadButton::DpadRight) == SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    "PadButton values must be SDL controller buttons");
static_assert(int(PadAxis::LeftX) == SDL_CONTROLLER_AXIS_LEFTX && int(PadAxis::TriggerRight) == SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    "PadAxis values must be SDL controller axes");

SdlInput::SdlInput(const ActionMap& actions) : _actions{actions} {
    // pads keep working when the window is unfocused: friends play on the TV while
    // the developer types on another monitor (keyboard input still needs focus)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    // SDL then sends CONTROLLERDEVICEADDED for every controller already connected
    _initialized = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
}

SdlInput::~SdlInput() {
    for(auto& [id, pad] : _pads) SDL_GameControllerClose(pad.controller);
    if(_initialized) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

bool SdlInput::handleEvent(const SDL_Event& event, InputSystem& input) {
    if(event.type == SDL_CONTROLLERDEVICEADDED) {
        SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which); // device index
        if(!controller) return true;
        const int id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
        if(_pads.count(id)) {
            SDL_GameControllerClose(controller); // already open; drop the extra reference
            return true;
        }
        _pads[id].controller = controller;
        input.handle(DeviceEvent{DeviceEvent::Kind::Added, id});
        return true;
    }
    if(event.type == SDL_CONTROLLERDEVICEREMOVED) {
        const int id = event.cdevice.which; // instance id
        const auto it = _pads.find(id);
        if(it == _pads.end()) return true;
        SDL_GameControllerClose(it->second.controller);
        _pads.erase(it);
        input.handle(DeviceEvent{DeviceEvent::Kind::Removed, id});
        return true;
    }
    return false;
}

void SdlInput::poll(InputSystem& input) {
    int count = 0;
    const Uint8* keys = SDL_GetKeyboardState(&count);
    count = std::min(count, kKeyCount);
    for(int sc = 0; sc < count; ++sc) {
        if(keys[sc] == _keys[sc]) continue;
        _keys[sc] = keys[sc];
        const Key key = static_cast<Key>(sc);
        const float value = keys[sc] ? 1.0f : 0.0f;
        for(const Source::Kind side : {Source::KeyboardLeft, Source::KeyboardRight})
            if(_actions.usesKey(side, key)) input.handle(InputEvent{Source::keyboard(side), Control::key(key), value});
    }

    for(auto& [id, pad] : _pads) {
        const Source source = Source::gamepad(id);
        for(int b = 0; b < kPadButtonCount; ++b) {
            const bool down = SDL_GameControllerGetButton(pad.controller, static_cast<SDL_GameControllerButton>(b)) != 0;
            if(down == pad.buttons[b]) continue;
            pad.buttons[b] = down;
            input.handle(InputEvent{source, Control::pad(static_cast<PadButton>(b)), down ? 1.0f : 0.0f});
        }
        for(int a = 0; a < kPadAxisCount; ++a) {
            const float raw = float(SDL_GameControllerGetAxis(pad.controller, static_cast<SDL_GameControllerAxis>(a))) / 32767.0f;
            const float value = std::clamp(raw, -1.0f, 1.0f);
            if(std::abs(value - pad.axes[a]) < 1e-3f) continue;
            pad.axes[a] = value;
            input.handle(InputEvent{source, Control::axis(static_cast<PadAxis>(a)), value});
        }
    }
}

} // namespace vibe::game
