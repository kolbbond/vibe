#include <cmath>
#include "vibe/fluid/Preset.hxx"

#include <fstream>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <utility>

#include <json/json.h>

namespace vibe::fluid {

namespace {

class Reader {
public:
    explicit Reader(std::string& error) : _error{error} {}

    bool fail(const std::string& message) {
        _error = message;
        return false;
    }

    // `obj` must be an object whose keys are all in `allowed`
    bool object(const Json::Value& obj, const std::string& path, std::initializer_list<const char*> allowed) {
        if(!obj.isObject()) return fail(path + ": not an object");
        for(const std::string& key : obj.getMemberNames()) {
            bool known = false;
            for(const char* a : allowed) known = known || key == a;
            if(!known) return fail(path + "." + key + ": unknown key");
        }
        return true;
    }

    bool number(const Json::Value& obj, const std::string& path, const char* key, float& out) {
        if(!obj.isMember(key)) return true;
        if(!obj[key].isNumeric() || !std::isfinite(obj[key].asFloat())) return fail(path + "." + key + ": not a finite number");
        out = obj[key].asFloat();
        return true;
    }

    bool integer(const Json::Value& obj, const std::string& path, const char* key, int& out) {
        if(!obj.isMember(key)) return true;
        if(!obj[key].isInt()) return fail(path + "." + key + ": not an integer");
        out = obj[key].asInt();
        return true;
    }

    bool boolean(const Json::Value& obj, const std::string& path, const char* key, bool& out) {
        if(!obj.isMember(key)) return true;
        if(!obj[key].isBool()) return fail(path + "." + key + ": not true/false");
        out = obj[key].asBool();
        return true;
    }

    bool seed(const Json::Value& obj, const std::string& path, const char* key, std::uint64_t& out) {
        if(!obj.isMember(key)) return true;
        if(!obj[key].isUInt64()) return fail(path + "." + key + ": not a non-negative integer");
        out = obj[key].asUInt64();
        return true;
    }

    bool color(const Json::Value& v, const std::string& path, Color3& out) {
        if(!v.isArray() || v.size() != 3 || !v[0].isNumeric() || !v[1].isNumeric() || !v[2].isNumeric())
            return fail(path + ": expected [r, g, b]");
        out = Color3{v[0].asFloat(), v[1].asFloat(), v[2].asFloat()};
        return true;
    }

    bool color(const Json::Value& obj, const std::string& path, const char* key, Color3& out) {
        if(!obj.isMember(key)) return true;
        return color(obj[key], path + "." + key, out);
    }

private:
    std::string& _error;
};

// section `key` of `parent`; missing = empty object, so defaults stay
const Json::Value& sectionOf(const Json::Value& parent, const char* key) {
    static const Json::Value empty{Json::objectValue};
    return parent.isMember(key) ? parent[key] : empty;
}

bool readCommon(Reader& r, const Json::Value& o, const std::string& path, EmitterCommon& c) {
    return r.boolean(o, path, "enabled", c.enabled) && r.number(o, path, "gain", c.gain) &&
           r.number(o, path, "threshold", c.threshold) && r.color(o, path, "color", c.color);
}

Json::Value colorJson(const Color3& c) {
    Json::Value a{Json::arrayValue};
    a.append(c.r());
    a.append(c.g());
    a.append(c.b());
    return a;
}

Json::Value commonJson(const EmitterCommon& c) {
    Json::Value o{Json::objectValue};
    o["enabled"] = c.enabled;
    o["gain"] = c.gain;
    o["threshold"] = c.threshold;
    o["color"] = colorJson(c.color);
    return o;
}

} // namespace

namespace {

// every number a preset can set, bounded so a hand-edited file can't hang the solver
// (iterations), exhaust memory (counts) or blow the field up (gains, strengths)
bool checkRanges(Reader& r, const Preset& p) {
    const auto in = [&](const std::string& path, double v, double lo, double hi) {
        if(v >= lo && v <= hi) return true;
        return r.fail(path + ": must be " + std::to_string(lo) + ".." + std::to_string(hi));
    };
    const auto color = [&](const std::string& path, const Color3& c) {
        return in(path + "[0]", c.r(), 0.0, 10.0) && in(path + "[1]", c.g(), 0.0, 10.0) && in(path + "[2]", c.b(), 0.0, 10.0);
    };
    const auto common = [&](const std::string& path, const EmitterCommon& e) {
        return in(path + ".gain", e.gain, 0.0, 10.0) && in(path + ".threshold", e.threshold, 0.0, 1.0) && color(path + ".color", e.color);
    };
    const FluidParams& f = p.fluid;
    const EmitterParams& e = p.emitters;
    const RenderLook& l = p.look;
    return in("fluid.viscosity", f.viscosity, 0.0, 5.0) && in("fluid.dyeFade", f.dyeFade, 0.0, 1.0) &&
           in("fluid.velocityFade", f.velocityFade, 0.0, 1.0) && in("fluid.vorticity", f.vorticity, 0.0, 200.0) &&
           in("fluid.pressureIterations", f.pressureIterations, 0, 200) && in("look.glow", l.glow, 0.0, 10.0) &&
           in("look.glowLod", l.glowLod, 0, 8) && in("look.exposure", l.exposure, 0.0, 20.0) &&
           in("look.brightness", l.brightness, 0.0, 3.0) && in("look.height", l.height, 0.0, 1.0) &&
           color("look.palette[0]", l.palette[0]) && color("look.palette[1]", l.palette[1]) && color("look.palette[2]", l.palette[2]) &&
           common("emitters.burst", e.burst) && in("emitters.burst.strength", e.burst.strength, 0.0, 20.0) &&
           in("emitters.burst.ringRadius", e.burst.ringRadius, 0.0, 1.0) && in("emitters.burst.splats", e.burst.splats, 1, 64) &&
           in("emitters.burst.refractory", e.burst.refractory, 0.0, 5.0) && common("emitters.vortex", e.vortex) &&
           in("emitters.vortex.strength", e.vortex.strength, 0.0, 20.0) && in("emitters.vortex.radius", e.vortex.radius, 0.0, 1.0) &&
           common("emitters.jets", e.jets) && in("emitters.jets.strength", e.jets.strength, 0.0, 20.0) &&
           in("emitters.jets.radius", e.jets.radius, 0.0, 1.0) && in("emitters.jets.rate", e.jets.rate, 0.0, 10.0) &&
           common("emitters.splash", e.splash) && in("emitters.splash.strength", e.splash.strength, 0.0, 20.0) &&
           in("emitters.splash.refractory", e.splash.refractory, 0.0, 5.0) && common("emitters.sparks", e.sparks) &&
           in("emitters.sparks.count", e.sparks.count, 0, 256) && in("emitters.glow.base", e.glow.base, 0.0, 10.0) &&
           in("emitters.glow.gain", e.glow.gain, 0.0, 10.0) && in("emitters.hue.rate", e.hue.rate, 0.0, 10.0) &&
           in("emitters.hue.minChroma", e.hue.minChroma, 0.0, 1.0) && in("emitters.ambient.strength", e.ambient.strength, 0.0, 20.0) &&
           in("emitters.ambient.speed", e.ambient.speed, 0.0, 10.0) && in("emitters.ambient.points", e.ambient.points, 0, 16) &&
           in("emitters.ambient.radius", e.ambient.radius, 0.0, 1.0) && color("emitters.ambient.color", e.ambient.color) &&
           in("emitters.ambient.dye", e.ambient.dye, 0.0, 10.0) && in("emitters.mix.musicStrength", e.mix.musicStrength, 0.0, 10.0);
}

} // namespace

bool parsePreset(std::string_view json, Preset& out, std::string& error) {
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> parser{builder.newCharReader()};
    Json::Value root;
    std::string parseErrors;
    if(!parser->parse(json.data(), json.data() + json.size(), &root, &parseErrors)) {
        error = "json: " + parseErrors;
        return false;
    }
    Reader r{error};
    if(!r.object(root, "preset", {"name", "grid", "fluid", "look", "emitters"})) return false;

    Preset p;
    if(root.isMember("name")) {
        if(!root["name"].isString()) return r.fail("preset.name: not a string");
        p.name = root["name"].asString();
    }

    const Json::Value& grid = sectionOf(root, "grid");
    if(!r.object(grid, "grid", {"width", "height"}) || !r.integer(grid, "grid", "width", p.width) ||
        !r.integer(grid, "grid", "height", p.height))
        return false;
    if(p.width < 16 || p.height < 16 || p.width > 512 || p.height > 512) return r.fail("grid: width and height must be 16..512");

    const Json::Value& fluid = sectionOf(root, "fluid");
    if(!r.object(fluid, "fluid", {"viscosity", "dyeFade", "velocityFade", "vorticity", "pressureIterations"}) ||
        !r.number(fluid, "fluid", "viscosity", p.fluid.viscosity) || !r.number(fluid, "fluid", "dyeFade", p.fluid.dyeFade) ||
        !r.number(fluid, "fluid", "velocityFade", p.fluid.velocityFade) || !r.number(fluid, "fluid", "vorticity", p.fluid.vorticity) ||
        !r.integer(fluid, "fluid", "pressureIterations", p.fluid.pressureIterations))
        return false;

    const Json::Value& look = sectionOf(root, "look");
    if(!r.object(look, "look", {"colorMode", "glow", "glowLod", "exposure", "brightness", "palette", "display", "height", "autoRotate"}) ||
        !r.number(look, "look", "height", p.look.height) || !r.boolean(look, "look", "autoRotate", p.look.autoRotate) ||
        !r.number(look, "look", "glow", p.look.glow) || !r.number(look, "look", "brightness", p.look.brightness) ||
        !r.integer(look, "look", "glowLod", p.look.glowLod) || !r.number(look, "look", "exposure", p.look.exposure))
        return false;
    if(look.isMember("colorMode")) {
        const std::string mode = look["colorMode"].isString() ? look["colorMode"].asString() : std::string{};
        if(mode == "dye")
            p.look.colorMode = ColorMode::Dye;
        else if(mode == "palette")
            p.look.colorMode = ColorMode::Palette;
        else
            return r.fail("look.colorMode: expected \"dye\" or \"palette\"");
    }
    if(look.isMember("display")) {
        const std::string name = look["display"].isString() ? look["display"].asString() : std::string{};
        if(!parseDisplay(name, p.look.display)) return r.fail("look.display: expected \"flat\", \"terrain\" or \"planet\"");
    }
    if(look.isMember("palette")) {
        const Json::Value& palette = look["palette"];
        if(!palette.isArray() || palette.size() != 3) return r.fail("look.palette: expected 3 colors");
        for(Json::ArrayIndex k = 0; k < 3; ++k)
            if(!r.color(palette[k], "look.palette[" + std::to_string(k) + "]", p.look.palette[k])) return false;
    }

    const Json::Value& em = sectionOf(root, "emitters");
    if(!r.object(em, "emitters", {"burst", "vortex", "jets", "splash", "sparks", "glow", "hue", "ambient", "mix"}))
        return false;

    const Json::Value& burst = sectionOf(em, "burst");
    if(!r.object(burst, "emitters.burst", {"enabled", "gain", "threshold", "color", "strength", "ringRadius", "splats", "refractory"}) ||
        !readCommon(r, burst, "emitters.burst", p.emitters.burst) || !r.number(burst, "emitters.burst", "strength", p.emitters.burst.strength) ||
        !r.number(burst, "emitters.burst", "ringRadius", p.emitters.burst.ringRadius) ||
        !r.integer(burst, "emitters.burst", "splats", p.emitters.burst.splats) ||
        !r.number(burst, "emitters.burst", "refractory", p.emitters.burst.refractory))
        return false;

    const Json::Value& vortex = sectionOf(em, "vortex");
    if(!r.object(vortex, "emitters.vortex", {"enabled", "gain", "threshold", "color", "strength", "radius"}) ||
        !readCommon(r, vortex, "emitters.vortex", p.emitters.vortex) ||
        !r.number(vortex, "emitters.vortex", "strength", p.emitters.vortex.strength) ||
        !r.number(vortex, "emitters.vortex", "radius", p.emitters.vortex.radius))
        return false;

    const Json::Value& jets = sectionOf(em, "jets");
    if(!r.object(jets, "emitters.jets", {"enabled", "gain", "threshold", "color", "strength", "radius", "rate"}) ||
        !readCommon(r, jets, "emitters.jets", p.emitters.jets) || !r.number(jets, "emitters.jets", "strength", p.emitters.jets.strength) ||
        !r.number(jets, "emitters.jets", "radius", p.emitters.jets.radius) || !r.number(jets, "emitters.jets", "rate", p.emitters.jets.rate))
        return false;

    const Json::Value& splash = sectionOf(em, "splash");
    if(!r.object(splash, "emitters.splash", {"enabled", "gain", "threshold", "color", "strength", "seed", "refractory"}) ||
        !readCommon(r, splash, "emitters.splash", p.emitters.splash) ||
        !r.number(splash, "emitters.splash", "strength", p.emitters.splash.strength) ||
        !r.seed(splash, "emitters.splash", "seed", p.emitters.splash.seed) ||
        !r.number(splash, "emitters.splash", "refractory", p.emitters.splash.refractory))
        return false;

    const Json::Value& sparks = sectionOf(em, "sparks");
    if(!r.object(sparks, "emitters.sparks", {"enabled", "gain", "threshold", "color", "count"}) ||
        !readCommon(r, sparks, "emitters.sparks", p.emitters.sparks) || !r.integer(sparks, "emitters.sparks", "count", p.emitters.sparks.count))
        return false;

    const Json::Value& glow = sectionOf(em, "glow");
    if(!r.object(glow, "emitters.glow", {"base", "gain"}) || !r.number(glow, "emitters.glow", "base", p.emitters.glow.base) ||
        !r.number(glow, "emitters.glow", "gain", p.emitters.glow.gain))
        return false;

    const Json::Value& hue = sectionOf(em, "hue");
    if(!r.object(hue, "emitters.hue", {"enabled", "rate", "minChroma"}) || !r.boolean(hue, "emitters.hue", "enabled", p.emitters.hue.enabled) ||
        !r.number(hue, "emitters.hue", "rate", p.emitters.hue.rate) || !r.number(hue, "emitters.hue", "minChroma", p.emitters.hue.minChroma))
        return false;

    const Json::Value& ambient = sectionOf(em, "ambient");
    AmbientParams& a = p.emitters.ambient;
    if(!r.object(ambient, "emitters.ambient", {"enabled", "strength", "speed", "points", "radius", "color", "dye"}) ||
        !r.boolean(ambient, "emitters.ambient", "enabled", a.enabled) || !r.number(ambient, "emitters.ambient", "strength", a.strength) ||
        !r.number(ambient, "emitters.ambient", "speed", a.speed) || !r.integer(ambient, "emitters.ambient", "points", a.points) ||
        !r.number(ambient, "emitters.ambient", "radius", a.radius) || !r.number(ambient, "emitters.ambient", "dye", a.dye))
        return false;
    if(ambient.isMember("color") && !r.color(ambient["color"], "emitters.ambient.color", a.color)) return false;

    const Json::Value& mix = sectionOf(em, "mix");
    if(!r.object(mix, "emitters.mix", {"music", "musicStrength"}) || !r.boolean(mix, "emitters.mix", "music", p.emitters.mix.music) ||
        !r.number(mix, "emitters.mix", "musicStrength", p.emitters.mix.musicStrength))
        return false;
    if(!checkRanges(r, p)) return false;

    out = std::move(p);
    return true;
}

std::string presetToJson(const Preset& p) {
    Json::Value root{Json::objectValue};
    root["name"] = p.name;
    root["grid"]["width"] = p.width;
    root["grid"]["height"] = p.height;
    root["fluid"]["viscosity"] = p.fluid.viscosity;
    root["fluid"]["dyeFade"] = p.fluid.dyeFade;
    root["fluid"]["velocityFade"] = p.fluid.velocityFade;
    root["fluid"]["vorticity"] = p.fluid.vorticity;
    root["fluid"]["pressureIterations"] = p.fluid.pressureIterations;
    root["look"]["colorMode"] = p.look.colorMode == ColorMode::Palette ? "palette" : "dye";
    root["look"]["glow"] = p.look.glow;
    root["look"]["glowLod"] = p.look.glowLod;
    root["look"]["exposure"] = p.look.exposure;
    root["look"]["brightness"] = p.look.brightness;
    root["look"]["display"] = displayName(p.look.display);
    root["look"]["height"] = p.look.height;
    root["look"]["autoRotate"] = p.look.autoRotate;
    Json::Value palette{Json::arrayValue};
    for(const Color3& c : p.look.palette) palette.append(colorJson(c));
    root["look"]["palette"] = palette;

    Json::Value& e = root["emitters"];
    e["burst"] = commonJson(p.emitters.burst);
    e["burst"]["strength"] = p.emitters.burst.strength;
    e["burst"]["ringRadius"] = p.emitters.burst.ringRadius;
    e["burst"]["splats"] = p.emitters.burst.splats;
    e["burst"]["refractory"] = p.emitters.burst.refractory;
    e["vortex"] = commonJson(p.emitters.vortex);
    e["vortex"]["strength"] = p.emitters.vortex.strength;
    e["vortex"]["radius"] = p.emitters.vortex.radius;
    e["jets"] = commonJson(p.emitters.jets);
    e["jets"]["strength"] = p.emitters.jets.strength;
    e["jets"]["radius"] = p.emitters.jets.radius;
    e["jets"]["rate"] = p.emitters.jets.rate;
    e["splash"] = commonJson(p.emitters.splash);
    e["splash"]["strength"] = p.emitters.splash.strength;
    e["splash"]["seed"] = Json::UInt64{p.emitters.splash.seed};
    e["splash"]["refractory"] = p.emitters.splash.refractory;
    e["sparks"] = commonJson(p.emitters.sparks);
    e["sparks"]["count"] = p.emitters.sparks.count;
    e["glow"]["base"] = p.emitters.glow.base;
    e["glow"]["gain"] = p.emitters.glow.gain;
    e["hue"]["enabled"] = p.emitters.hue.enabled;
    e["hue"]["rate"] = p.emitters.hue.rate;
    e["hue"]["minChroma"] = p.emitters.hue.minChroma;
    const AmbientParams& a = p.emitters.ambient;
    e["ambient"]["enabled"] = a.enabled;
    e["ambient"]["strength"] = a.strength;
    e["ambient"]["speed"] = a.speed;
    e["ambient"]["points"] = a.points;
    e["ambient"]["radius"] = a.radius;
    e["ambient"]["color"] = colorJson(a.color);
    e["ambient"]["dye"] = a.dye;
    e["mix"]["music"] = p.emitters.mix.music;
    e["mix"]["musicStrength"] = p.emitters.mix.musicStrength;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    return Json::writeString(writer, root) + "\n";
}

bool loadPreset(const std::string& path, Preset& out, std::string& error) {
    std::ifstream file{path};
    if(!file) {
        error = path + ": cannot open";
        return false;
    }
    std::stringstream text;
    text << file.rdbuf();
    if(!parsePreset(text.str(), out, error)) {
        error = path + ": " + error;
        return false;
    }
    return true;
}

bool savePreset(const std::string& path, const Preset& preset, std::string& error) {
    std::ofstream file{path};
    if(!file) {
        error = path + ": cannot write";
        return false;
    }
    file << presetToJson(preset);
    if(!file) {
        error = path + ": write failed";
        return false;
    }
    return true;
}

} // namespace vibe::fluid
