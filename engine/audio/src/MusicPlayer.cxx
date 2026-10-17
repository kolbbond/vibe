#include "vibe/audio/MusicPlayer.hxx"

#include <cstdio>
#include <filesystem>

#include "vibe/audio/WavData.hxx"

namespace vibe::daw {

MusicPlayer::MusicPlayer() = default;

MusicPlayer::~MusicPlayer() {
    stop();
    if(_lsioReady) _lsio->destroy();
}

bool MusicPlayer::ensureSoundIo() {
    if(_lsioReady) return true;
    _lsio = LibSoundIo::create();
#ifdef _WIN32
    _lsio->set_backend(SoundIoBackendWasapi);
#else
    _lsio->set_backend(SoundIoBackendPulseAudio);
#endif
    if(_lsio->init() != 0) {
        std::fprintf(stderr, "music: no audio output available\n");
        return false;
    }
    _lsioReady = true;
    return true;
}

bool MusicPlayer::play(const std::string& wavPath, double volume, bool loop) {
    stop();

    std::error_code ec;
    if(!std::filesystem::is_regular_file(wavPath, ec)) {
        std::fprintf(stderr, "music: no such file %s\n", wavPath.c_str());
        return false;
    }
    const ShWavDataPr wav = WavData::create();
    if(!wav->load(wavPath)) {
        std::fprintf(stderr, "music: cannot load %s\n", wavPath.c_str());
        return false;
    }
    const arma::Mat<double> samples = wav->get_sample_matrix();
    if(samples.n_rows == 0 || samples.n_cols == 0) return false;
    if(!ensureSoundIo()) return false;

    SoundIoDevice* device = _lsio->get_output_device();
    if(!device) return false;
    _lsio->create_outstream(_stream, device);
    if(!_stream) return false;

    _audio = AudioInfo{_stream->format, samples.row(0) * volume, static_cast<int>(samples.n_cols)};
    _playback = std::make_unique<PlayBackInfo>(PlayBackInfo{&_audio, 0, loop, _lsio->get_soundio(), _lsio->_write_sample, false});
    _stream->userdata = _playback.get();
    _stream->write_callback = WavPlayer::write_callback;
    _stream->underflow_callback = WavPlayer::underflow_callback;
    _stream->software_latency = 0.050;
    _stream->sample_rate = static_cast<int>(wav->get_sample_rate());

    if(_lsio->open_outstream(_stream) != 0) {
        soundio_outstream_destroy(_stream);
        _stream = nullptr;
        _playback.reset();
        return false;
    }

    SoundIoOutStream* stream = _stream;
    SoundIo* soundio = _lsio->get_soundio();
    PlayBackInfo* playback = _playback.get();
    _thread = std::thread{[stream, soundio, playback] {
        if(const int err = soundio_outstream_start(stream)) {
            std::fprintf(stderr, "music: start failed: %s\n", soundio_strerror(err));
            return;
        }
        while(!playback->is_done) soundio_wait_events(soundio);
    }};
    _playing = true;
    return true;
}

void MusicPlayer::stop() {
    if(!_playing) return;
    _playback->is_done = true;
    soundio_wakeup(_lsio->get_soundio());
    if(_thread.joinable()) _thread.join();
    soundio_outstream_destroy(_stream);
    _stream = nullptr;
    _playback.reset();
    _playing = false;
}

} // namespace vibe::daw
