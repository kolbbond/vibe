# vibe-audio unit tests (no audio device needed)
find_package(Corrade REQUIRED TestSuite)

corrade_add_test(AudioMusicPlayerTest MusicPlayerTest.cxx LIBRARIES vibeaudio)
