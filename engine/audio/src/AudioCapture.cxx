// Real-time audio capture implementation

#include <iostream>
#include <cstring>
#include <algorithm>

#include "AudioCapture.hxx"

namespace vibe {
namespace daw {

// Ring buffer implementation
CaptureRingBuffer::CaptureRingBuffer(size_t capacity)
    : _buffer(capacity), _capacity(capacity) {
}

void CaptureRingBuffer::write(const double* samples, size_t count) {
    std::lock_guard<std::mutex> lock(_mutex);

    for (size_t i = 0; i < count; i++) {
        _buffer[_write_pos % _capacity] = samples[i];
        _write_pos++;

        // If we've wrapped around and caught up to read position, advance read
        if (_write_pos - _read_pos > _capacity) {
            _read_pos = _write_pos - _capacity;
        }
    }
}

size_t CaptureRingBuffer::read(double* samples, size_t count) {
    std::lock_guard<std::mutex> lock(_mutex);

    size_t avail = _write_pos - _read_pos;
    size_t to_read = std::min(count, avail);

    for (size_t i = 0; i < to_read; i++) {
        samples[i] = _buffer[_read_pos % _capacity];
        _read_pos++;
    }

    return to_read;
}

size_t CaptureRingBuffer::peek(double* samples, size_t count) const {
    std::lock_guard<std::mutex> lock(_mutex);

    size_t write_pos = _write_pos.load();
    size_t read_pos = _read_pos.load();
    size_t avail = write_pos - read_pos;
    size_t to_read = std::min(count, avail);

    // Read the most recent samples (from end of buffer, not beginning)
    size_t start_pos = (avail > count) ? (write_pos - count) : read_pos;

    for (size_t i = 0; i < to_read; i++) {
        samples[i] = _buffer[(start_pos + i) % _capacity];
    }

    return to_read;
}

size_t CaptureRingBuffer::available() const {
    return _write_pos - _read_pos;
}

void CaptureRingBuffer::clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _write_pos = 0;
    _read_pos = 0;
}

// AudioCapture implementation
AudioCapture::AudioCapture() : _ring_buffer(48000 * 10) { // 10 seconds buffer
}

AudioCapture::~AudioCapture() {
    stop();
}

ShAudioCapturePr AudioCapture::create() {
    return std::make_shared<AudioCapture>();
}

void AudioCapture::set_libsoundio(ShLibSoundIoPr lsio) {
    _lsio = lsio;
}

bool AudioCapture::start(int device_index) {
    if (_is_running) {
        std::cerr << "AudioCapture: already running\n";
        return false;
    }

    if (!_lsio) {
        std::cerr << "AudioCapture: libsoundio not set\n";
        return false;
    }

    // Get input device
    _device = _lsio->get_input_device_by_index(device_index);
    if (!_device) {
        std::cerr << "AudioCapture: failed to get input device\n";
        return false;
    }

    std::cout << "AudioCapture: using device: " << _device->name << "\n";

    if (_device->probe_error) {
        std::cerr << "AudioCapture: cannot probe device: "
                  << soundio_strerror(_device->probe_error) << "\n";
        soundio_device_unref(_device);
        _device = nullptr;
        return false;
    }

    // Create input stream
    _lsio->create_instream(_instream, _device);
    if (!_instream) {
        std::cerr << "AudioCapture: failed to create input stream\n";
        soundio_device_unref(_device);
        _device = nullptr;
        return false;
    }

    // Configure stream
    _instream->software_latency = 0.05; // 50ms latency
    _instream->read_callback = AudioCapture::read_callback;
    _instream->overflow_callback = LibSoundIo::overflow_callback;

    // Open stream first - this finalizes the format/layout
    if (_lsio->open_instream(_instream) != 0) {
        std::cerr << "AudioCapture: failed to open input stream\n";
        soundio_instream_destroy(_instream);
        _instream = nullptr;
        soundio_device_unref(_device);
        _device = nullptr;
        return false;
    }

    _sample_rate = _instream->sample_rate;
    _channels = _instream->layout.channel_count;

    // Setup capture info AFTER opening (layout is now finalized)
    _capture_info.ring_buffer = &_ring_buffer;
    _capture_info.read_sample = _lsio->_read_sample;
    _capture_info.channels = _channels;
    _capture_info.is_running = &_is_running;
    _instream->userdata = &_capture_info;

    std::cout << "AudioCapture: sample rate = " << _sample_rate
              << ", channels = " << _channels << "\n";

    // Clear the ring buffer
    _ring_buffer.clear();

    // Start the stream
    _is_running = true;
    int err = soundio_instream_start(_instream);
    if (err) {
        std::cerr << "AudioCapture: unable to start stream: "
                  << soundio_strerror(err) << "\n";
        _is_running = false;
        soundio_instream_destroy(_instream);
        _instream = nullptr;
        soundio_device_unref(_device);
        _device = nullptr;
        return false;
    }

    std::cout << "AudioCapture: started\n";
    return true;
}

void AudioCapture::stop() {
    if (!_is_running) return;

    _is_running = false;

    if (_instream) {
        soundio_instream_destroy(_instream);
        _instream = nullptr;
    }

    if (_device) {
        soundio_device_unref(_device);
        _device = nullptr;
    }

    std::cout << "AudioCapture: stopped\n";
}

bool AudioCapture::is_running() const {
    return _is_running;
}

arma::Mat<double> AudioCapture::get_buffer(int num_samples) {
    if (_channels <= 0 || num_samples <= 0) {
        return arma::Mat<double>();
    }

    // Read interleaved samples (channels * num_samples)
    size_t total_samples = static_cast<size_t>(num_samples) * static_cast<size_t>(_channels);
    std::vector<double> raw_samples(total_samples, 0.0);

    size_t available = _ring_buffer.available();
    size_t to_read = std::min(total_samples, available);

    if (to_read > 0) {
        _ring_buffer.read(raw_samples.data(), to_read);
    }

    // Deinterleave into matrix (num_channels x num_samples)
    arma::Mat<double> result(_channels, num_samples, arma::fill::zeros);

    size_t frames_read = to_read / static_cast<size_t>(_channels);
    for (size_t frame = 0; frame < frames_read; frame++) {
        for (int ch = 0; ch < _channels; ch++) {
            result(ch, frame) = raw_samples[frame * static_cast<size_t>(_channels) + static_cast<size_t>(ch)];
        }
    }

    return result;
}

arma::Row<double> AudioCapture::get_buffer_mono(int num_samples) {
    arma::Mat<double> stereo = get_buffer(num_samples);
    if (stereo.n_rows == 0) {
        return arma::Row<double>(num_samples, arma::fill::zeros);
    }

    // Mix down to mono (average of all channels)
    arma::Row<double> mono = arma::mean(stereo, 0);
    return mono;
}

arma::Mat<double> AudioCapture::peek_buffer(int num_samples) const {
    if (_channels <= 0 || num_samples <= 0) {
        return arma::Mat<double>();
    }

    // Peek at interleaved samples (channels * num_samples)
    size_t total_samples = static_cast<size_t>(num_samples) * static_cast<size_t>(_channels);
    std::vector<double> raw_samples(total_samples, 0.0);

    size_t samples_read = _ring_buffer.peek(raw_samples.data(), total_samples);

    // Deinterleave into matrix (num_channels x num_samples)
    arma::Mat<double> result(_channels, num_samples, arma::fill::zeros);

    size_t frames_read = samples_read / static_cast<size_t>(_channels);
    for (size_t frame = 0; frame < frames_read; frame++) {
        for (int ch = 0; ch < _channels; ch++) {
            result(ch, frame) = raw_samples[frame * static_cast<size_t>(_channels) + static_cast<size_t>(ch)];
        }
    }

    return result;
}

int AudioCapture::get_sample_rate() const {
    return _sample_rate;
}

int AudioCapture::get_channels() const {
    return _channels;
}

void AudioCapture::read_callback(struct SoundIoInStream* instream, int frame_count_min, int frame_count_max) {
    (void)frame_count_min;

    CaptureInfo* info = static_cast<CaptureInfo*>(instream->userdata);
    if (!info || !info->is_running->load()) return;

    struct SoundIoChannelArea* areas;
    int frames_left = frame_count_max;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;

        err = soundio_instream_begin_read(instream, &areas, &frame_count);
        if (err) {
            std::cerr << "AudioCapture: read error: " << soundio_strerror(err) << "\n";
            break;
        }

        if (frame_count == 0) break;

        if (!areas) {
            // Hole in buffer - skip these frames
            soundio_instream_end_read(instream);
            frames_left -= frame_count;
            continue;
        }

        // areas is valid - process samples
        {
            // Read samples from device
            int channels = info->channels;
            std::vector<double> samples(static_cast<size_t>(frame_count) * static_cast<size_t>(channels));

            for (int frame = 0; frame < frame_count; frame++) {
                for (int ch = 0; ch < channels; ch++) {
                    const char* ptr = areas[ch].ptr + areas[ch].step * frame;
                    double sample = info->read_sample(ptr);
                    samples[static_cast<size_t>(frame) * static_cast<size_t>(channels) + static_cast<size_t>(ch)] = sample;
                }
            }

            // Write to ring buffer
            info->ring_buffer->write(samples.data(), samples.size());
        }

        err = soundio_instream_end_read(instream);
        if (err) {
            std::cerr << "AudioCapture: end read error: " << soundio_strerror(err) << "\n";
            break;
        }

        frames_left -= frame_count;
    }
}

} // namespace daw
} // namespace vibe
