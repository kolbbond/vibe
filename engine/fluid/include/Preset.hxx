#pragma once

#include <string>
#include <string_view>

#include "vibe/fluid/Emitters.hxx"
#include "vibe/fluid/FluidSolver.hxx"
#include "vibe/fluid/Look.hxx"

namespace vibe::fluid {

// a complete plasma look: grid, fluid settings, render look and every emitter
struct Preset {
    std::string name = "default";
    int width = 192;
    int height = 108;
    FluidParams fluid;
    RenderLook look;
    EmitterParams emitters = defaultEmitterParams();
};

// strict: unknown keys are errors that name the path ("emitters.burst.gian: unknown key");
// missing keys keep their defaults; on failure `out` is untouched
bool parsePreset(std::string_view json, Preset& out, std::string& error);
std::string presetToJson(const Preset& preset);
bool loadPreset(const std::string& path, Preset& out, std::string& error);
bool savePreset(const std::string& path, const Preset& preset, std::string& error);

} // namespace vibe::fluid
