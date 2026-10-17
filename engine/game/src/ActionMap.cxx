#include "vibe/game/ActionMap.hxx"

#include <utility>

namespace vibe::game {

ActionMap::ButtonBuilder& ActionMap::ButtonBuilder::key(Source::Kind side, Key k) {
    _map->_buttons[_id.index].bindings.push_back({side, Control::key(k), AxisRole::None});
    return *this;
}

ActionMap::ButtonBuilder& ActionMap::ButtonBuilder::pad(PadButton b) {
    _map->_buttons[_id.index].bindings.push_back({Source::Gamepad, Control::pad(b), AxisRole::None});
    return *this;
}

ActionMap::AxisBuilder& ActionMap::AxisBuilder::keys(Source::Kind side, Key negative, Key positive) {
    auto& bindings = _map->_axes[_id.index].bindings;
    bindings.push_back({side, Control::key(negative), AxisRole::Negative});
    bindings.push_back({side, Control::key(positive), AxisRole::Positive});
    return *this;
}

ActionMap::AxisBuilder& ActionMap::AxisBuilder::padAxis(PadAxis a) {
    _map->_axes[_id.index].bindings.push_back({Source::Gamepad, Control::axis(a), AxisRole::Analog});
    return *this;
}

ActionMap::AxisBuilder& ActionMap::AxisBuilder::padButtons(PadButton negative, PadButton positive) {
    auto& bindings = _map->_axes[_id.index].bindings;
    bindings.push_back({Source::Gamepad, Control::pad(negative), AxisRole::Negative});
    bindings.push_back({Source::Gamepad, Control::pad(positive), AxisRole::Positive});
    return *this;
}

ActionMap::ActionMap() {
    // order matters: ids must match Confirm/Back/Start and NavX/NavY
    button("confirm")
        .key(Source::KeyboardLeft, Key::Space)
        .key(Source::KeyboardRight, Key::Return)
        .pad(PadButton::A);
    button("back")
        .key(Source::KeyboardLeft, Key::Q)
        .key(Source::KeyboardRight, Key::Backspace)
        .pad(PadButton::B);
    button("start")
        .key(Source::KeyboardLeft, Key::Escape)
        .key(Source::KeyboardRight, Key::Escape)
        .pad(PadButton::Start);
    axis("navX")
        .keys(Source::KeyboardLeft, Key::A, Key::D)
        .keys(Source::KeyboardRight, Key::Left, Key::Right)
        .padAxis(PadAxis::LeftX)
        .padButtons(PadButton::DpadLeft, PadButton::DpadRight);
    axis("navY")
        .keys(Source::KeyboardLeft, Key::W, Key::S)
        .keys(Source::KeyboardRight, Key::Up, Key::Down)
        .padAxis(PadAxis::LeftY)
        .padButtons(PadButton::DpadUp, PadButton::DpadDown);
}

ActionMap::ButtonBuilder ActionMap::button(std::string name) {
    _buttons.push_back({std::move(name), {}});
    return ButtonBuilder{*this, ButtonId{buttonCount() - 1}};
}

ActionMap::AxisBuilder ActionMap::axis(std::string name) {
    _axes.push_back({std::move(name), {}});
    return AxisBuilder{*this, AxisId{axisCount() - 1}};
}

bool ActionMap::buttonBound(ButtonId id, Source::Kind kind, Control control) const {
    for(const ActionBinding& b : bindings(id))
        if(b.kind == kind && b.control == control) return true;
    return false;
}

AxisRole ActionMap::axisRole(AxisId id, Source::Kind kind, Control control) const {
    for(const ActionBinding& b : bindings(id))
        if(b.kind == kind && b.control == control) return b.role;
    return AxisRole::None;
}

bool ActionMap::usesKey(Source::Kind side, Key k) const {
    const Control control = Control::key(k);
    for(const auto* list : {&_buttons, &_axes})
        for(const Action& action : *list)
            for(const ActionBinding& b : action.bindings)
                if(b.kind == side && b.control == control) return true;
    return false;
}

} // namespace vibe::game
