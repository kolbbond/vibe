#include "vibe/game/InputScript.hxx"

#include <charconv>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <utility>

namespace vibe::game {

namespace {

constexpr std::pair<std::string_view, Key> kKeys[] = {
    {"A", Key::A}, {"D", Key::D}, {"Q", Key::Q}, {"S", Key::S}, {"W", Key::W},
    {"Return", Key::Return}, {"Escape", Key::Escape}, {"Backspace", Key::Backspace}, {"Space", Key::Space},
    {"Right", Key::Right}, {"Left", Key::Left}, {"Down", Key::Down}, {"Up", Key::Up},
    {"LShift", Key::LShift}, {"RShift", Key::RShift},
};

constexpr std::pair<std::string_view, PadButton> kButtons[] = {
    {"A", PadButton::A}, {"B", PadButton::B}, {"X", PadButton::X}, {"Y", PadButton::Y},
    {"Back", PadButton::Back}, {"Guide", PadButton::Guide}, {"Start", PadButton::Start},
    {"LeftStick", PadButton::LeftStick}, {"RightStick", PadButton::RightStick},
    {"LeftShoulder", PadButton::LeftShoulder}, {"RightShoulder", PadButton::RightShoulder},
    {"DpadUp", PadButton::DpadUp}, {"DpadDown", PadButton::DpadDown},
    {"DpadLeft", PadButton::DpadLeft}, {"DpadRight", PadButton::DpadRight},
};

constexpr std::pair<std::string_view, PadAxis> kAxes[] = {
    {"LeftX", PadAxis::LeftX}, {"LeftY", PadAxis::LeftY}, {"RightX", PadAxis::RightX},
    {"RightY", PadAxis::RightY}, {"TriggerLeft", PadAxis::TriggerLeft}, {"TriggerRight", PadAxis::TriggerRight},
};

template<class T, std::size_t N>
std::optional<T> lookup(const std::pair<std::string_view, T> (&table)[N], std::string_view name) {
    for(const auto& [n, v] : table)
        if(n == name) return v;
    return std::nullopt;
}

std::optional<int> parseInt(std::string_view s) {
    int v = 0;
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if(ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

std::optional<Source> parseSource(std::string_view s) {
    if(s == "kl") return Source::keyboard(Source::KeyboardLeft);
    if(s == "kr") return Source::keyboard(Source::KeyboardRight);
    if(s.size() > 3 && s.substr(0, 3) == "pad")
        if(const auto id = parseInt(s.substr(3))) return Source::gamepad(*id);
    return std::nullopt;
}

std::optional<Control> parseControl(std::string_view s) {
    const auto colon = s.find(':');
    if(colon == std::string_view::npos) return std::nullopt;
    const std::string_view kind = s.substr(0, colon);
    const std::string_view name = s.substr(colon + 1);
    if(kind == "key")
        if(const auto k = lookup(kKeys, name)) return Control::key(*k);
    if(kind == "btn")
        if(const auto b = lookup(kButtons, name)) return Control::pad(*b);
    if(kind == "axis")
        if(const auto a = lookup(kAxes, name)) return Control::axis(*a);
    return std::nullopt;
}

} // namespace

bool parseInputScript(std::string_view text, std::vector<ScriptedInput>& out, std::string& error) {
    out.clear();
    std::istringstream lines{std::string{text}};
    std::string line;
    int lineNo = 0;
    int lastFrame = 0;
    const auto fail = [&](const std::string& message) {
        error = "line " + std::to_string(lineNo) + ": " + message;
        return false;
    };

    while(std::getline(lines, line)) {
        ++lineNo;
        if(const auto hash = line.find('#'); hash != std::string::npos) line.resize(hash);
        std::istringstream fields{line};
        std::string frameText, sourceText, controlText, valueText;
        if(!(fields >> frameText)) continue; // blank or comment-only
        if(!(fields >> sourceText >> controlText >> valueText)) return fail("expected: frame source control value");

        const auto frame = parseInt(frameText);
        if(!frame || *frame < 0) return fail("bad frame '" + frameText + "'");
        if(*frame < lastFrame) return fail("frames must not decrease");
        const auto source = parseSource(sourceText);
        if(!source) return fail("bad source '" + sourceText + "' (kl, kr or pad<N>)");
        const auto control = parseControl(controlText);
        if(!control) return fail("bad control '" + controlText + "'");
        char* end = nullptr;
        const float value = std::strtof(valueText.c_str(), &end);
        if(end != valueText.c_str() + valueText.size()) return fail("bad value '" + valueText + "'");

        lastFrame = *frame;
        out.push_back({*frame, InputEvent{*source, *control, value}});
    }
    return true;
}

} // namespace vibe::game
