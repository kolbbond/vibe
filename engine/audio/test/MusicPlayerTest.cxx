#include <Corrade/TestSuite/Tester.h>

#include "vibe/audio/MusicPlayer.hxx"

using vibe::daw::MusicPlayer;

struct MusicPlayerTest : Corrade::TestSuite::Tester {
    explicit MusicPlayerTest();

    void missingFileStaysSilent();
    void directoryIsNotATrack();
    void stopWithoutPlayIsSafe();
    void destroysCleanlyWithoutEverPlaying();
};

MusicPlayerTest::MusicPlayerTest() {
    addTests({&MusicPlayerTest::missingFileStaysSilent,
        &MusicPlayerTest::directoryIsNotATrack,
        &MusicPlayerTest::stopWithoutPlayIsSafe,
        &MusicPlayerTest::destroysCleanlyWithoutEverPlaying});
}

void MusicPlayerTest::missingFileStaysSilent() {
    MusicPlayer music;
    CORRADE_VERIFY(!music.play("/nonexistent/track.wav"));
    CORRADE_VERIFY(!music.playing());
}

void MusicPlayerTest::directoryIsNotATrack() {
    MusicPlayer music;
    CORRADE_VERIFY(!music.play("/"));
    CORRADE_VERIFY(!music.playing());
}

void MusicPlayerTest::stopWithoutPlayIsSafe() {
    MusicPlayer music;
    music.stop();
    music.stop();
    CORRADE_VERIFY(!music.playing());
}

void MusicPlayerTest::destroysCleanlyWithoutEverPlaying() {
    { MusicPlayer music; }
    CORRADE_VERIFY(true);
}

CORRADE_TEST_MAIN(MusicPlayerTest)
