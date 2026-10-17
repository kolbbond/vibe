#pragma once

#include <array>
#include <map>
#include <vector>

#include "vibe/game/ActionMap.hxx"
#include "vibe/game/Input.hxx"

namespace vibe::game {

// routes source input to player slots and latches it per game tick:
// pressed()/released() are true for exactly one tick after the edge,
// even if the edge happened between ticks or a frame ran several ticks
class InputSystem {
public:
    struct UnclaimedPress {
        Source source;
        ButtonId button;
    };

    explicit InputSystem(const ActionMap& actions, int maxPlayers = 4);

    void handle(const InputEvent& event);
    void handle(const DeviceEvent& event);
    // latch everything since the previous tick; call once per tick before reading
    void beginTick();

    int maxPlayers() const { return static_cast<int>(_slots.size()); }
    void assign(int slot, Source source);
    void release(int slot);
    bool assigned(int slot) const;
    Source source(int slot) const { return _slots[slot].source; }
    int slotOf(Source source) const; // -1 when unclaimed

    bool down(int slot, ButtonId b) const;
    bool pressed(int slot, ButtonId b) const;
    bool released(int slot, ButtonId b) const;
    float axis(int slot, AxisId a) const;

    // built-in button presses (confirm/back/start) from unclaimed sources, latched per tick
    const std::vector<UnclaimedPress>& unclaimedPresses() const { return _tickUnclaimed; }
    // gamepad hotplug, latched per tick
    const std::vector<DeviceEvent>& deviceEvents() const { return _tickDevices; }

    const ActionMap& actions() const { return _actions; }

    // 0.25 deadzone, rescaled so the live range still reaches 1
    static float applyDeadzone(float v);

private:
    struct Raw {
        std::array<bool, kKeyCount> keys{};
        std::array<bool, kPadButtonCount> buttons{};
        std::array<float, kPadAxisCount> axes{};
    };

    struct ButtonState {
        bool live = false; // latest state from events
        bool pendingPress = false;
        bool pendingRelease = false;
        bool down = false; // latched at beginTick
        bool pressed = false;
        bool released = false;
    };

    struct Slot {
        bool assigned = false;
        Source source;
        std::vector<ButtonState> buttons;
        std::vector<float> axes;
    };

    Raw& raw(const Source& source);
    const Raw* rawOrNull(const Source& source) const;
    static bool apply(Raw& raw, Control control, float value);
    static bool held(const Raw& raw, Control control);
    bool computeButton(const Source& source, ButtonId b) const;
    float computeAxis(const Source& source, AxisId a) const;
    void refreshSlot(int slot);
    bool validSlot(int slot) const { return slot >= 0 && slot < maxPlayers() && _slots[slot].assigned; }

    const ActionMap& _actions;
    std::array<Raw, 2> _keyboards; // KeyboardLeft, KeyboardRight
    std::map<int, Raw> _pads;
    std::vector<Slot> _slots;
    std::vector<UnclaimedPress> _pendingUnclaimed;
    std::vector<UnclaimedPress> _tickUnclaimed;
    std::vector<DeviceEvent> _pendingDevices;
    std::vector<DeviceEvent> _tickDevices;
};

} // namespace vibe::game
