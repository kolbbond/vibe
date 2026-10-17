#pragma once

#include <array>
#include <string_view>

#include "vibe/fluid/FluidSolver.hxx"

namespace vibe::fluid {

enum class ColorMode { Dye, Palette };

// how the field is shown: flat image, lit heightfield, or wrapped on a sphere
enum class Display { Flat, Terrain, Planet };

inline const char* displayName(Display d) {
    switch(d) {
    case Display::Terrain: return "terrain";
    case Display::Planet: return "planet";
    default: return "flat";
    }
}

inline bool parseDisplay(std::string_view name, Display& out) {
    for(Display d : {Display::Flat, Display::Terrain, Display::Planet})
        if(name == displayName(d)) {
            out = d;
            return true;
        }
    return false;
}

// how a renderer shows the dye field (preset-tunable)
struct RenderLook {
    ColorMode colorMode = ColorMode::Dye;
    float glow = 0.8f;     // mix of the blurred (glow) sample
    int glowLod = 3;       // mip level used for the glow blur
    float exposure = 1.5f; // tonemap strength: 1 - exp(-exposure * c)
    float brightness = 1.0f; // final multiplier after the tonemap
    Display display = Display::Flat;
    float height = 0.5f;     // terrain/planet relief, 0..1
    bool autoRotate = true;  // slow camera orbit in the 3D modes
    std::array<Color3, 3> palette{{Color3{0.5f, 0.0f, 0.6f}, Color3{0.0f, 0.8f, 1.0f}, Color3{1.0f, 1.0f, 1.0f}}};
};

} // namespace vibe::fluid
