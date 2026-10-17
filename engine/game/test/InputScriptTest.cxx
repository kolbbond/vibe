#include <Corrade/TestSuite/Tester.h>

#include "vibe/game/InputScript.hxx"

using namespace vibe::game;

struct InputScriptTest : Corrade::TestSuite::Tester {
    explicit InputScriptTest();

    void parsesKeyLine();
    void parsesPadButtonAndAxis();
    void skipsCommentsAndBlankLines();
    void rejectsUnknownName();
    void rejectsBadSource();
    void rejectsDecreasingFrames();
    void rejectsMissingField();
};

InputScriptTest::InputScriptTest() {
    addTests({&InputScriptTest::parsesKeyLine,
        &InputScriptTest::parsesPadButtonAndAxis,
        &InputScriptTest::skipsCommentsAndBlankLines,
        &InputScriptTest::rejectsUnknownName,
        &InputScriptTest::rejectsBadSource,
        &InputScriptTest::rejectsDecreasingFrames,
        &InputScriptTest::rejectsMissingField});
}

void InputScriptTest::parsesKeyLine() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(parseInputScript("12 kl key:Space 1", out, error));
    CORRADE_COMPARE(out.size(), 1u);
    CORRADE_COMPARE(out[0].frame, 12);
    CORRADE_VERIFY(out[0].event.source == Source::keyboard(Source::KeyboardLeft));
    CORRADE_VERIFY(out[0].event.control == Control::key(Key::Space));
    CORRADE_COMPARE(out[0].event.value, 1.0f);
}

void InputScriptTest::parsesPadButtonAndAxis() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(parseInputScript("3 pad7 btn:DpadUp 1\n4 pad7 axis:LeftX -0.5\n", out, error));
    CORRADE_COMPARE(out.size(), 2u);
    CORRADE_VERIFY(out[0].event.source == Source::gamepad(7));
    CORRADE_VERIFY(out[0].event.control == Control::pad(PadButton::DpadUp));
    CORRADE_VERIFY(out[1].event.control == Control::axis(PadAxis::LeftX));
    CORRADE_COMPARE(out[1].event.value, -0.5f);
}

void InputScriptTest::skipsCommentsAndBlankLines() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(parseInputScript("# setup\n\n   5 kr key:Return 1  # join\n", out, error));
    CORRADE_COMPARE(out.size(), 1u);
    CORRADE_VERIFY(out[0].event.source == Source::keyboard(Source::KeyboardRight));
}

void InputScriptTest::rejectsUnknownName() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(!parseInputScript("1 kl key:Banana 1", out, error));
    CORRADE_VERIFY(error.find("line 1") != std::string::npos);
}

void InputScriptTest::rejectsBadSource() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(!parseInputScript("1 mouse key:A 1", out, error));
    CORRADE_VERIFY(error.find("line 1") != std::string::npos);
}

void InputScriptTest::rejectsDecreasingFrames() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(!parseInputScript("5 kl key:A 1\n4 kl key:A 0\n", out, error));
    CORRADE_VERIFY(error.find("line 2") != std::string::npos);
}

void InputScriptTest::rejectsMissingField() {
    std::vector<ScriptedInput> out;
    std::string error;
    CORRADE_VERIFY(!parseInputScript("5 kl key:A", out, error));
    CORRADE_VERIFY(error.find("line 1") != std::string::npos);
}

CORRADE_TEST_MAIN(InputScriptTest)
