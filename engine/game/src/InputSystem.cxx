#include "vibe/game/InputSystem.hxx"

#include <algorithm>
#include <cmath>
#include <utility>

namespace vibe::game {

namespace {
constexpr int kBuiltInButtons = 3; // Confirm, Back, Start
}

InputSystem::InputSystem(const ActionMap& actions, int maxPlayers)
    : _actions{actions}, _slots(static_cast<std::size_t>(std::clamp(maxPlayers, 1, 4))) {}

float InputSystem::applyDeadzone(float v) {
    constexpr float kDeadzone = 0.25f;
    const float magnitude = std::abs(v);
    if(magnitude < kDeadzone) return 0.0f;
    const float scaled = std::min(1.0f, (magnitude - kDeadzone) / (1.0f - kDeadzone));
    return v < 0.0f ? -scaled : scaled;
}

InputSystem::Raw& InputSystem::raw(const Source& source) {
    if(source.kind == Source::Gamepad) return _pads[source.gamepadId];
    return _keyboards[source.kind == Source::KeyboardLeft ? 0 : 1];
}

const InputSystem::Raw* InputSystem::rawOrNull(const Source& source) const {
    if(source.kind == Source::Gamepad) {
        const auto it = _pads.find(source.gamepadId);
        return it == _pads.end() ? nullptr : &it->second;
    }
    return &_keyboards[source.kind == Source::KeyboardLeft ? 0 : 1];
}

bool InputSystem::apply(Raw& raw, Control control, float value) {
    switch(control.type) {
    case Control::Type::Key:
        if(control.code >= kKeyCount) return false;
        raw.keys[control.code] = value >= 0.5f;
        return true;
    case Control::Type::PadButton:
        if(control.code >= kPadButtonCount) return false;
        raw.buttons[control.code] = value >= 0.5f;
        return true;
    case Control::Type::PadAxis:
        if(control.code >= kPadAxisCount) return false;
        raw.axes[control.code] = std::clamp(value, -1.0f, 1.0f);
        return true;
    }
    return false;
}

bool InputSystem::held(const Raw& raw, Control control) {
    switch(control.type) {
    case Control::Type::Key: return control.code < kKeyCount && raw.keys[control.code];
    case Control::Type::PadButton: return control.code < kPadButtonCount && raw.buttons[control.code];
    case Control::Type::PadAxis: return control.code < kPadAxisCount && std::abs(raw.axes[control.code]) >= 0.5f;
    }
    return false;
}

bool InputSystem::computeButton(const Source& source, ButtonId b) const {
    const Raw* r = rawOrNull(source);
    if(!r) return false;
    for(const ActionBinding& binding : _actions.bindings(b))
        if(binding.kind == source.kind && held(*r, binding.control)) return true;
    return false;
}

float InputSystem::computeAxis(const Source& source, AxisId a) const {
    const Raw* r = rawOrNull(source);
    if(!r) return 0.0f;
    float digital = 0.0f;
    float analog = 0.0f;
    for(const ActionBinding& binding : _actions.bindings(a)) {
        if(binding.kind != source.kind) continue;
        switch(binding.role) {
        case AxisRole::Negative:
            if(held(*r, binding.control)) digital -= 1.0f;
            break;
        case AxisRole::Positive:
            if(held(*r, binding.control)) digital += 1.0f;
            break;
        case AxisRole::Analog: {
            const float v = binding.control.code < kPadAxisCount ? applyDeadzone(r->axes[binding.control.code]) : 0.0f;
            if(std::abs(v) > std::abs(analog)) analog = v;
            break;
        }
        case AxisRole::None: break;
        }
    }
    return digital != 0.0f ? std::clamp(digital, -1.0f, 1.0f) : analog;
}

void InputSystem::refreshSlot(int slot) {
    Slot& s = _slots[slot];
    for(std::size_t b = 0; b < s.buttons.size(); ++b) {
        ButtonState& state = s.buttons[b];
        const bool now = computeButton(s.source, ButtonId{static_cast<int>(b)});
        if(now && !state.live) state.pendingPress = true;
        if(!now && state.live) state.pendingRelease = true;
        state.live = now;
    }
}

void InputSystem::handle(const InputEvent& event) {
    const int slot = slotOf(event.source);
    std::array<bool, kBuiltInButtons> before{};
    if(slot < 0)
        for(int b = 0; b < kBuiltInButtons; ++b) before[b] = computeButton(event.source, ButtonId{b});

    if(!apply(raw(event.source), event.control, event.value)) return;

    if(slot >= 0) {
        refreshSlot(slot);
        return;
    }
    for(int b = 0; b < kBuiltInButtons; ++b)
        if(!before[b] && computeButton(event.source, ButtonId{b}))
            _pendingUnclaimed.push_back({event.source, ButtonId{b}});
}

void InputSystem::handle(const DeviceEvent& event) {
    if(event.kind == DeviceEvent::Kind::Added)
        _pads[event.gamepadId];
    else
        _pads.erase(event.gamepadId);
    const int slot = slotOf(Source::gamepad(event.gamepadId));
    if(slot >= 0) refreshSlot(slot);
    _pendingDevices.push_back(event);
}

void InputSystem::beginTick() {
    for(Slot& s : _slots) {
        if(!s.assigned) continue;
        for(ButtonState& state : s.buttons) {
            state.down = state.live;
            state.pressed = state.pendingPress;
            state.released = state.pendingRelease;
            state.pendingPress = false;
            state.pendingRelease = false;
        }
        for(std::size_t a = 0; a < s.axes.size(); ++a)
            s.axes[a] = computeAxis(s.source, AxisId{static_cast<int>(a)});
    }
    _tickUnclaimed = std::move(_pendingUnclaimed);
    _pendingUnclaimed.clear();
    _tickDevices = std::move(_pendingDevices);
    _pendingDevices.clear();
}

void InputSystem::assign(int slot, Source source) {
    if(slot < 0 || slot >= maxPlayers()) return;
    Slot& s = _slots[slot];
    s.assigned = true;
    s.source = source;
    s.buttons.assign(static_cast<std::size_t>(_actions.buttonCount()), ButtonState{});
    s.axes.assign(static_cast<std::size_t>(_actions.axisCount()), 0.0f);
    // buttons already held count as down but not as a new press
    for(std::size_t b = 0; b < s.buttons.size(); ++b) {
        const bool now = computeButton(source, ButtonId{static_cast<int>(b)});
        s.buttons[b].live = now;
        s.buttons[b].down = now;
    }
}

void InputSystem::release(int slot) {
    if(slot < 0 || slot >= maxPlayers()) return;
    _slots[slot] = Slot{};
}

bool InputSystem::assigned(int slot) const { return validSlot(slot); }

int InputSystem::slotOf(Source source) const {
    for(int i = 0; i < maxPlayers(); ++i)
        if(_slots[i].assigned && _slots[i].source == source) return i;
    return -1;
}

bool InputSystem::down(int slot, ButtonId b) const {
    return validSlot(slot) && b.index >= 0 && b.index < static_cast<int>(_slots[slot].buttons.size()) &&
           _slots[slot].buttons[b.index].down;
}

bool InputSystem::pressed(int slot, ButtonId b) const {
    return validSlot(slot) && b.index >= 0 && b.index < static_cast<int>(_slots[slot].buttons.size()) &&
           _slots[slot].buttons[b.index].pressed;
}

bool InputSystem::released(int slot, ButtonId b) const {
    return validSlot(slot) && b.index >= 0 && b.index < static_cast<int>(_slots[slot].buttons.size()) &&
           _slots[slot].buttons[b.index].released;
}

float InputSystem::axis(int slot, AxisId a) const {
    if(!validSlot(slot) || a.index < 0 || a.index >= static_cast<int>(_slots[slot].axes.size())) return 0.0f;
    return _slots[slot].axes[a.index];
}

} // namespace vibe::game
