#pragma once

#include <cstdint>

namespace vibe::game {

// where input comes from; a player owns exactly one source
struct Source {
    enum Kind : std::uint8_t { KeyboardLeft, KeyboardRight, Gamepad };

    Kind kind = KeyboardLeft;
    int gamepadId = -1; // SDL joystick instance id, Gamepad only

    static constexpr Source keyboard(Kind side) { return Source{side, -1}; }
    static constexpr Source gamepad(int id) { return Source{Gamepad, id}; }

    friend constexpr bool operator==(const Source& a, const Source& b) {
        return a.kind == b.kind && a.gamepadId == b.gamepadId;
    }
    friend constexpr bool operator!=(const Source& a, const Source& b) { return !(a == b); }
};

// keyboard scancodes; values equal SDL_Scancode (SdlInput static_asserts this)
enum class Key : std::uint16_t {
    A = 4,
    D = 7,
    Q = 20,
    S = 22,
    W = 26,
    Return = 40,
    Escape = 41,
    Backspace = 42,
    Space = 44,
    Right = 79,
    Left = 80,
    Down = 81,
    Up = 82,
    LShift = 225,
    RShift = 229,
};
constexpr int kKeyCount = 512; // SDL_NUM_SCANCODES

// values equal SDL_GameControllerButton
enum class PadButton : std::uint8_t {
    A,
    B,
    X,
    Y,
    Back,
    Guide,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
};
constexpr int kPadButtonCount = 15;

// values equal SDL_GameControllerAxis; sticks are -1..1 with +y down
enum class PadAxis : std::uint8_t { LeftX, LeftY, RightX, RightY, TriggerLeft, TriggerRight };
constexpr int kPadAxisCount = 6;

// one physical control on a source
struct Control {
    enum class Type : std::uint8_t { Key, PadButton, PadAxis };

    Type type = Type::Key;
    std::uint16_t code = 0;

    static constexpr Control key(Key k) { return Control{Type::Key, static_cast<std::uint16_t>(k)}; }
    static constexpr Control pad(PadButton b) { return Control{Type::PadButton, static_cast<std::uint16_t>(b)}; }
    static constexpr Control axis(PadAxis a) { return Control{Type::PadAxis, static_cast<std::uint16_t>(a)}; }

    friend constexpr bool operator==(const Control& a, const Control& b) {
        return a.type == b.type && a.code == b.code;
    }
    friend constexpr bool operator!=(const Control& a, const Control& b) { return !(a == b); }
};

// keys and buttons: value 0 or 1; axes: -1..1
struct InputEvent {
    Source source;
    Control control;
    float value = 0.0f;
};

struct DeviceEvent {
    enum class Kind : std::uint8_t { Added, Removed };

    Kind kind = Kind::Added;
    int gamepadId = -1;
};

} // namespace vibe::game
