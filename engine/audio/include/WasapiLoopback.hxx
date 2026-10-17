// WASAPI Loopback capture for Windows
// Captures audio from output devices (what's playing through speakers/headphones)
#pragma once

#ifdef _WIN32

#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <string>

#include <armadillo>

// Forward declarations for Windows types
struct IAudioClient;
struct IAudioCaptureClient;
struct IMMDevice;
struct IMMDeviceEnumerator;

namespace vibe {
namespace daw {

// Forward declare the ring buffer from AudioCapture
class CaptureRingBuffer;

typedef std::shared_ptr<class WasapiLoopback> ShWasapiLoopbackPr;

// Output device info for loopback
struct LoopbackDeviceInfo {
    int index;
    std::string id;
    std::string name;
    bool is_default;
};

class WasapiLoopback {
protected:
    IMMDeviceEnumerator* _enumerator = nullptr;
    IMMDevice* _device = nullptr;
    IAudioClient* _audio_client = nullptr;
    IAudioCaptureClient* _capture_client = nullptr;

    std::unique_ptr<CaptureRingBuffer> _ring_buffer;
    std::atomic<bool> _is_running{false};
    std::thread _capture_thread;

    int _sample_rate = 0;
    int _channels = 0;
    int _bits_per_sample = 0;

    bool _com_initialized = false;

public:
    WasapiLoopback();
    ~WasapiLoopback();

    static ShWasapiLoopbackPr create();

    // List available output devices for loopback
    std::vector<LoopbackDeviceInfo> list_devices();

    // Start loopback capture from specified output device (-1 = default)
    bool start(int device_index = -1);

    // Start loopback capture from device ID string
    bool start_by_id(const std::string& device_id);

    // Start loopback capture by matching device name
    bool start_by_name(const std::string& device_name);

    // Stop capturing
    void stop();

    // Check if capturing is active
    bool is_running() const;

    // Get the most recent samples as a matrix (num_channels x num_samples)
    arma::Mat<double> get_buffer(int num_samples);

    // Get a single-channel buffer (first channel or mono mix)
    arma::Row<double> get_buffer_mono(int num_samples);

    // Peek at the most recent samples WITHOUT consuming them
    arma::Mat<double> peek_buffer(int num_samples) const;

    // Get the capture sample rate
    int get_sample_rate() const;

    // Get number of channels
    int get_channels() const;

private:
    void capture_thread_func();
    bool initialize_com();
    void cleanup();
};

} // namespace daw
} // namespace vibe

#endif // _WIN32
