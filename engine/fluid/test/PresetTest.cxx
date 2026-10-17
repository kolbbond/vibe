#include <cstdio>
#include <string>

#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "vibe/fluid/Preset.hxx"

using namespace vibe::fluid;

struct PresetTest : Corrade::TestSuite::Tester {
    explicit PresetTest();

    void defaultsRoundTrip();
    void changedValuesRoundTrip();
    void emptyObjectKeepsDefaults();
    void unknownKeyNamesPath();
    void unknownSectionRejected();
    void wrongTypeNamesPath();
    void badColorModeRejected();
    void gridOutOfRangeRejected();
    void outOfRangeRejected();
    void newFieldsRoundTrip();
    void displayRoundTrip();
    void oldPresetDefaultsToFlat();
    void saveAndLoadFile();
    void missingFileNamesPath();
    void shippedPresetsParse();
};

PresetTest::PresetTest() {
    addTests({&PresetTest::defaultsRoundTrip,
        &PresetTest::changedValuesRoundTrip,
        &PresetTest::emptyObjectKeepsDefaults,
        &PresetTest::unknownKeyNamesPath,
        &PresetTest::unknownSectionRejected,
        &PresetTest::wrongTypeNamesPath,
        &PresetTest::badColorModeRejected,
        &PresetTest::gridOutOfRangeRejected,
        &PresetTest::outOfRangeRejected,
        &PresetTest::newFieldsRoundTrip,
        &PresetTest::displayRoundTrip,
        &PresetTest::oldPresetDefaultsToFlat,
        &PresetTest::saveAndLoadFile,
        &PresetTest::missingFileNamesPath,
        &PresetTest::shippedPresetsParse});
}

void PresetTest::defaultsRoundTrip() {
    const Preset p;
    const std::string json = presetToJson(p);
    Preset q;
    std::string error;
    CORRADE_VERIFY(parsePreset(json, q, error));
    CORRADE_COMPARE(presetToJson(q), json);
    CORRADE_COMPARE(q.width, 192);
    CORRADE_COMPARE(q.height, 108);
}

void PresetTest::changedValuesRoundTrip() {
    Preset p;
    p.name = "test";
    p.width = 128;
    p.height = 72;
    p.fluid.vorticity = 25.5f;
    p.look.colorMode = ColorMode::Palette;
    p.look.palette[1] = Color3{0.1f, 0.2f, 0.3f};
    p.emitters.splash.seed = 42;
    p.emitters.jets.rate = 0.9f;
    p.emitters.sparks.enabled = false;
    Preset q;
    std::string error;
    CORRADE_VERIFY(parsePreset(presetToJson(p), q, error));
    CORRADE_COMPARE(q.name, "test");
    CORRADE_COMPARE(q.width, 128);
    CORRADE_COMPARE(q.fluid.vorticity, 25.5f);
    CORRADE_VERIFY(q.look.colorMode == ColorMode::Palette);
    CORRADE_COMPARE(q.look.palette[1], (Color3{0.1f, 0.2f, 0.3f}));
    CORRADE_COMPARE(q.emitters.splash.seed, 42u);
    CORRADE_COMPARE(q.emitters.jets.rate, 0.9f);
    CORRADE_VERIFY(!q.emitters.sparks.enabled);
}

void PresetTest::emptyObjectKeepsDefaults() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(parsePreset("{}", q, error));
    CORRADE_COMPARE(presetToJson(q), presetToJson(Preset{}));
}

void PresetTest::unknownKeyNamesPath() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!parsePreset(R"({"emitters": {"burst": {"gian": 1}}})", q, error));
    CORRADE_VERIFY(error.find("emitters.burst.gian") != std::string::npos);
}

void PresetTest::unknownSectionRejected() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!parsePreset(R"({"grids": {}})", q, error));
    CORRADE_VERIFY(error.find("preset.grids") != std::string::npos);
}

void PresetTest::wrongTypeNamesPath() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!parsePreset(R"({"fluid": {"vorticity": "high"}})", q, error));
    CORRADE_VERIFY(error.find("fluid.vorticity") != std::string::npos);
}

void PresetTest::badColorModeRejected() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!parsePreset(R"({"look": {"colorMode": "rainbow"}})", q, error));
    CORRADE_VERIFY(error.find("look.colorMode") != std::string::npos);
}

void PresetTest::gridOutOfRangeRejected() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!parsePreset(R"({"grid": {"width": 4096, "height": 4096}})", q, error));
    CORRADE_VERIFY(error.find("grid") != std::string::npos);
    CORRADE_COMPARE(q.width, 192); // untouched on failure
}

void PresetTest::saveAndLoadFile() {
    Preset p;
    p.name = "saved";
    p.look.glow = 1.25f;
    const std::string path = "/tmp/vibe-fluid-preset-test.json";
    std::string error;
    CORRADE_VERIFY(savePreset(path, p, error));
    Preset q;
    CORRADE_VERIFY(loadPreset(path, q, error));
    CORRADE_COMPARE(presetToJson(q), presetToJson(p));
    std::remove(path.c_str());
}

void PresetTest::missingFileNamesPath() {
    Preset q;
    std::string error;
    CORRADE_VERIFY(!loadPreset("/nonexistent/lava.json", q, error));
    CORRADE_VERIFY(error.find("/nonexistent/lava.json") != std::string::npos);
}

void PresetTest::shippedPresetsParse() {
    for(const char* name : {"neon", "lava", "aurora", "giant", "ridge"}) {
        Preset p;
        std::string error;
        CORRADE_ITERATION(name);
        CORRADE_VERIFY(loadPreset(std::string{VVAVVIZ_PRESETS_DIR} + "/" + name + ".json", p, error));
        CORRADE_COMPARE(error, "");
        CORRADE_COMPARE(p.name, name);
    }
}

// review finding: out-of-range numbers must be rejected, not hang, exhaust memory or crash
void PresetTest::outOfRangeRejected() {
    const std::pair<const char*, const char*> cases[] = {
        {R"({"fluid": {"pressureIterations": 100000}})", "fluid.pressureIterations"},
        {R"({"fluid": {"viscosity": -0.25}})", "fluid.viscosity"},
        {R"({"fluid": {"dyeFade": 2.0}})", "fluid.dyeFade"},
        {R"({"emitters": {"sparks": {"count": 100000}}})", "emitters.sparks.count"},
        {R"({"emitters": {"burst": {"splats": 1000000}}})", "emitters.burst.splats"},
        {R"({"emitters": {"burst": {"gain": 1e39}}})", "emitters.burst.gain"},
        {R"({"emitters": {"vortex": {"strength": 1e6}}})", "emitters.vortex.strength"},
        {R"({"look": {"glowLod": 40}})", "look.glowLod"},
    };
    for(const auto& [json, path] : cases) {
        Preset p;
        std::string error;
        CORRADE_ITERATION(path);
        CORRADE_VERIFY(!parsePreset(json, p, error));
        CORRADE_VERIFY(error.find(path) != std::string::npos);
    }
}

// brightness, music mix and ambient are saved and loaded like every other look setting
void PresetTest::newFieldsRoundTrip() {
    Preset p;
    p.look.brightness = 1.7f;
    p.emitters.mix.music = false;
    p.emitters.mix.musicStrength = 2.5f;
    p.emitters.ambient.enabled = true;
    p.emitters.ambient.strength = 0.9f;
    p.emitters.ambient.points = 5;
    Preset q;
    std::string error;
    CORRADE_VERIFY(parsePreset(presetToJson(p), q, error));
    CORRADE_COMPARE(q.look.brightness, 1.7f);
    CORRADE_VERIFY(!q.emitters.mix.music);
    CORRADE_COMPARE(q.emitters.mix.musicStrength, 2.5f);
    CORRADE_COMPARE(q.emitters.ambient.strength, 0.9f);
    CORRADE_COMPARE(q.emitters.ambient.points, 5);
    CORRADE_VERIFY(!parsePreset(R"({"look": {"brightness": 9}})", q, error));
    CORRADE_VERIFY(error.find("look.brightness") != std::string::npos);
    CORRADE_VERIFY(!parsePreset(R"({"emitters": {"ambient": {"points": 500}}})", q, error));
    CORRADE_VERIFY(error.find("emitters.ambient.points") != std::string::npos);
}

void PresetTest::displayRoundTrip() {
    Preset p;
    p.look.display = Display::Planet;
    p.look.height = 0.8f;
    p.look.autoRotate = false;
    Preset q;
    std::string error;
    CORRADE_VERIFY(parsePreset(presetToJson(p), q, error));
    CORRADE_VERIFY(q.look.display == Display::Planet);
    CORRADE_COMPARE(q.look.height, 0.8f);
    CORRADE_VERIFY(!q.look.autoRotate);
    CORRADE_VERIFY(!parsePreset(R"({"look": {"display": "cube"}})", q, error));
    CORRADE_VERIFY(error.find("look.display") != std::string::npos);
    CORRADE_VERIFY(!parsePreset(R"({"look": {"height": 2}})", q, error));
    CORRADE_VERIFY(error.find("look.height") != std::string::npos);
}

// presets saved before the 3D modes (no display key) load flat
void PresetTest::oldPresetDefaultsToFlat() {
    Preset q;
    q.look.display = Display::Terrain;
    std::string error;
    CORRADE_VERIFY(parsePreset(R"({"look": {"glow": 1.0}})", q, error));
    CORRADE_VERIFY(q.look.display == Display::Flat);
}

CORRADE_TEST_MAIN(PresetTest)
