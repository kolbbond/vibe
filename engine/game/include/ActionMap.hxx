#pragma once

#include <string>
#include <vector>

#include "vibe/game/Input.hxx"

namespace vibe::game {

struct ButtonId {
    int index = -1;
};

struct AxisId {
    int index = -1;
};

// how a control feeds an axis
enum class AxisRole : std::uint8_t { None, Negative, Positive, Analog };

struct ActionBinding {
    Source::Kind kind;
    Control control;
    AxisRole role;
};

// game-declared named actions with default bindings per source kind.
// the built-in menu actions (Confirm, Back, Start, NavX, NavY) always exist.
class ActionMap {
public:
    static constexpr ButtonId Confirm{0};
    static constexpr ButtonId Back{1};
    static constexpr ButtonId Start{2};
    static constexpr AxisId NavX{0};
    static constexpr AxisId NavY{1};

    class ButtonBuilder {
    public:
        ButtonBuilder& key(Source::Kind side, Key k);
        ButtonBuilder& pad(PadButton b);
        operator ButtonId() const { return _id; }

    private:
        friend class ActionMap;
        ButtonBuilder(ActionMap& map, ButtonId id) : _map{&map}, _id{id} {}

        ActionMap* _map;
        ButtonId _id;
    };

    class AxisBuilder {
    public:
        AxisBuilder& keys(Source::Kind side, Key negative, Key positive);
        AxisBuilder& padAxis(PadAxis a);
        AxisBuilder& padButtons(PadButton negative, PadButton positive);
        operator AxisId() const { return _id; }

    private:
        friend class ActionMap;
        AxisBuilder(ActionMap& map, AxisId id) : _map{&map}, _id{id} {}

        ActionMap* _map;
        AxisId _id;
    };

    ActionMap();

    ButtonBuilder button(std::string name);
    AxisBuilder axis(std::string name);

    int buttonCount() const { return static_cast<int>(_buttons.size()); }
    int axisCount() const { return static_cast<int>(_axes.size()); }
    const std::string& buttonName(ButtonId id) const { return _buttons[id.index].name; }
    const std::string& axisName(AxisId id) const { return _axes[id.index].name; }
    const std::vector<ActionBinding>& bindings(ButtonId id) const { return _buttons[id.index].bindings; }
    const std::vector<ActionBinding>& bindings(AxisId id) const { return _axes[id.index].bindings; }

    // does `control` on a source of kind `kind` drive button `id`?
    bool buttonBound(ButtonId id, Source::Kind kind, Control control) const;
    // how `control` on a source of kind `kind` feeds axis `id`
    AxisRole axisRole(AxisId id, Source::Kind kind, Control control) const;
    // is key `k` bound to any action for keyboard side `side`? (SdlInput routing)
    bool usesKey(Source::Kind side, Key k) const;

private:
    struct Action {
        std::string name;
        std::vector<ActionBinding> bindings;
    };

    std::vector<Action> _buttons;
    std::vector<Action> _axes;
};

} // namespace vibe::game
