#pragma once

#include <memory>
#include <string>
#include <thread>

#include "vibe/audio/LibSoundIo.hxx"
#include "vibe/audio/WavPlayer.hxx"

namespace vibe::daw {

// looping background music from a WAV file (mono: first channel) on the default output device
class MusicPlayer {
public:
    MusicPlayer();
    ~MusicPlayer();
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;

    // false (and silence) when the file can't be loaded or there is no audio output
    bool play(const std::string& wavPath, double volume = 0.6, bool loop = true);
    void stop();
    bool playing() const { return _playing; }

private:
    bool ensureSoundIo();

    ShLibSoundIoPr _lsio;
    bool _lsioReady = false;
    SoundIoOutStream* _stream = nullptr;
    AudioInfo _audio{};
    std::unique_ptr<PlayBackInfo> _playback;
    std::thread _thread;
    bool _playing = false;
};

} // namespace vibe::daw
