// Real-time audio capture from input devices
#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <vector>

#include <armadillo>
#include <soundio/soundio.h>

#include "LibSoundIo.hxx"

namespace vibe {
namespace daw {

typedef std::shared_ptr<class AudioCapture> ShAudioCapturePr;

// Ring buffer for storing captured audio samples
class CaptureRingBuffer {
public:
    CaptureRingBuffer(size_t capacity = 48000 * 10); // 10 seconds at 48kHz default

    void write(const double* samples, size_t count);
    size_t read(double* samples, size_t count);
    size_t peek(double* samples, size_t count) const;  // Read without consuming
    size_t available() const;
    void clear();

private:
    std::vector<double> _buffer;
    std::atomic<size_t> _write_pos{0};
    std::atomic<size_t> _read_pos{0};
    size_t _capacity;
    mutable std::mutex _mutex;
};

// Capture info passed to the read callback
struct CaptureInfo {
    CaptureRingBuffer* ring_buffer;
    double (*read_sample)(const char* ptr);
    int channels;
    std::atomic<bool>* is_running;
};

class AudioCapture {
protected:
    ShLibSoundIoPr _lsio;
    SoundIoDevice* _device = nullptr;
    SoundIoInStream* _instream = nullptr;
    CaptureInfo _capture_info;
    mutable CaptureRingBuffer _ring_buffer;
    std::atomic<bool> _is_running{false};
    int _sample_rate = 0;
    int _channels = 0;

public:
    AudioCapture();
    ~AudioCapture();

    static ShAudioCapturePr create();

    // Initialize with a libsoundio instance
    void set_libsoundio(ShLibSoundIoPr lsio);

    // Start capturing from specified device (-1 = default)
    bool start(int device_index = -1);

    // Stop capturing
    void stop();

    // Check if capturing is active
    bool is_running() const;

    // Get the most recent samples as a matrix (num_channels x num_samples)
    // Returns samples in armadillo matrix format matching WavData output
    // NOTE: This CONSUMES samples from the ring buffer
    arma::Mat<double> get_buffer(int num_samples);

    // Get a single-channel buffer (first channel or mono mix)
    // NOTE: This CONSUMES samples from the ring buffer
    arma::Row<double> get_buffer_mono(int num_samples);

    // Peek at the most recent samples WITHOUT consuming them
    // Useful for visualization where you want to see current audio state
    arma::Mat<double> peek_buffer(int num_samples) const;

    // Get the capture sample rate
    int get_sample_rate() const;

    // Get number of channels
    int get_channels() const;

    // Static read callback for libsoundio
    static void read_callback(struct SoundIoInStream* instream, int frame_count_min, int frame_count_max);
};

} // namespace daw
} // namespace vibe
