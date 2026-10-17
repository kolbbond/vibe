#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "vibe/game/GameList.hxx"

using namespace vibe::game;

struct GameListTest : Corrade::TestSuite::Tester {
    explicit GameListTest();

    void parsesEntries();
    void titleDefaultsToNameAndArgsAreOptional();
    void missingNameIsRejected();
    void missingBinaryIsRejected();
    void badArgsAreRejected();
    void malformedJsonReportsError();
    void notAnArrayIsRejected();
};

GameListTest::GameListTest() {
    addTests({&GameListTest::parsesEntries,
        &GameListTest::titleDefaultsToNameAndArgsAreOptional,
        &GameListTest::missingNameIsRejected,
        &GameListTest::missingBinaryIsRejected,
        &GameListTest::badArgsAreRejected,
        &GameListTest::malformedJsonReportsError,
        &GameListTest::notAnArrayIsRejected});
}

void GameListTest::parsesEntries() {
    const char* json = R"([
        {"name": "alpha", "title": "Alpha", "players": "2-4",
         "description": "last cycle riding wins", "args": ["--mute", "a b"],
         "binary": "/b/alpha"},
        {"name": "beta", "title": "Beta", "binary": "/b/beta"}
    ])";
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(loadGameList(json, games, error));
    CORRADE_COMPARE(games.size(), 2u);
    CORRADE_COMPARE(games[0].name, "alpha");
    CORRADE_COMPARE(games[0].title, "Alpha");
    CORRADE_COMPARE(games[0].players, "2-4");
    CORRADE_COMPARE(games[0].description, "last cycle riding wins");
    CORRADE_COMPARE(games[0].binary, "/b/alpha");
    CORRADE_COMPARE(games[0].args.size(), 2u);
    CORRADE_COMPARE(games[0].args[1], "a b");
    CORRADE_COMPARE(games[1].name, "beta");
}

void GameListTest::titleDefaultsToNameAndArgsAreOptional() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(loadGameList(R"([{"name": "gamma", "binary": "/b/gamma"}])", games, error));
    CORRADE_COMPARE(games[0].title, "gamma");
    CORRADE_VERIFY(games[0].args.empty());
}

void GameListTest::missingNameIsRejected() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(!loadGameList(R"([{"name": "a", "binary": "/a"}, {"binary": "/b"}])", games, error));
    CORRADE_VERIFY(error.find("entry 1") != std::string::npos);
    CORRADE_VERIFY(error.find("name") != std::string::npos);
    CORRADE_VERIFY(games.empty());
}

void GameListTest::missingBinaryIsRejected() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(!loadGameList(R"([{"name": "a"}])", games, error));
    CORRADE_VERIFY(error.find("entry 0") != std::string::npos);
    CORRADE_VERIFY(error.find("binary") != std::string::npos);
}

void GameListTest::badArgsAreRejected() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(!loadGameList(R"([{"name": "a", "binary": "/a", "args": "--mute"}])", games, error));
    CORRADE_VERIFY(error.find("args") != std::string::npos);
}

void GameListTest::malformedJsonReportsError() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(!loadGameList("[{\"name\": ", games, error));
    CORRADE_VERIFY(!error.empty());
}

void GameListTest::notAnArrayIsRejected() {
    std::vector<GameEntry> games;
    std::string error;
    CORRADE_VERIFY(!loadGameList(R"({"name": "a", "binary": "/a"})", games, error));
    CORRADE_VERIFY(error.find("array") != std::string::npos);
}

CORRADE_TEST_MAIN(GameListTest)
