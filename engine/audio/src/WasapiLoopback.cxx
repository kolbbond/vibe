// WASAPI Loopback capture implementation for Windows
#ifdef _WIN32

#include "WasapiLoopback.hxx"
#include "AudioCapture.hxx" // For CaptureRingBuffer

#include <iostream>
#include <algorithm>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

// Link required libraries
#pragma comment(lib, "ole32.lib")

namespace vibe {
namespace daw {

// GUIDs
static const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
static const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
static const IID IID_IAudioClient = __uuidof(IAudioClient);
static const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

WasapiLoopback::WasapiLoopback()
    : _ring_buffer(std::make_unique<CaptureRingBuffer>(48000 * 10)) { // 10 seconds buffer
}

WasapiLoopback::~WasapiLoopback() {
    stop();
    cleanup();
}

ShWasapiLoopbackPr WasapiLoopback::create() {
    return std::make_shared<WasapiLoopback>();
}

bool WasapiLoopback::initialize_com() {
    if (_com_initialized) return true;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "WasapiLoopback: CoInitializeEx failed: 0x" << std::hex << hr << std::dec << "\n";
        return false;
    }
    _com_initialized = true;
    return true;
}

void WasapiLoopback::cleanup() {
    if (_capture_client) {
        _capture_client->Release();
        _capture_client = nullptr;
    }
    if (_audio_client) {
        _audio_client->Release();
        _audio_client = nullptr;
    }
    if (_device) {
        _device->Release();
        _device = nullptr;
    }
    if (_enumerator) {
        _enumerator->Release();
        _enumerator = nullptr;
    }
    if (_com_initialized) {
        CoUninitialize();
        _com_initialized = false;
    }
}

std::vector<LoopbackDeviceInfo> WasapiLoopback::list_devices() {
    std::vector<LoopbackDeviceInfo> devices;

    if (!initialize_com()) return devices;

    HRESULT hr;

    // Create device enumerator
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                          IID_IMMDeviceEnumerator, (void**)&enumerator);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to create device enumerator\n";
        return devices;
    }

    // Get default device ID for comparison
    IMMDevice* defaultDevice = nullptr;
    LPWSTR defaultId = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (SUCCEEDED(hr) && defaultDevice) {
        defaultDevice->GetId(&defaultId);
        defaultDevice->Release();
    }

    // Enumerate render (output) devices
    IMMDeviceCollection* collection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr)) {
        enumerator->Release();
        if (defaultId) CoTaskMemFree(defaultId);
        return devices;
    }

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; i++) {
        IMMDevice* device = nullptr;
        hr = collection->Item(i, &device);
        if (FAILED(hr)) continue;

        LoopbackDeviceInfo info;
        info.index = static_cast<int>(i);

        // Get device ID
        LPWSTR deviceId = nullptr;
        hr = device->GetId(&deviceId);
        if (SUCCEEDED(hr) && deviceId) {
            // Convert wide string to narrow
            int len = WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, nullptr, 0, nullptr, nullptr);
            std::string id(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, &id[0], len, nullptr, nullptr);
            info.id = id;

            // Check if default
            info.is_default = (defaultId && wcscmp(deviceId, defaultId) == 0);

            CoTaskMemFree(deviceId);
        }

        // Get device name from properties
        IPropertyStore* props = nullptr;
        hr = device->OpenPropertyStore(STGM_READ, &props);
        if (SUCCEEDED(hr)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);
            hr = props->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr) && varName.vt == VT_LPWSTR) {
                int len = WideCharToMultiByte(CP_UTF8, 0, varName.pwszVal, -1, nullptr, 0, nullptr, nullptr);
                std::string name(len - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, varName.pwszVal, -1, &name[0], len, nullptr, nullptr);
                info.name = name;
            }
            PropVariantClear(&varName);
            props->Release();
        }

        devices.push_back(info);
        device->Release();
    }

    collection->Release();
    enumerator->Release();
    if (defaultId) CoTaskMemFree(defaultId);

    return devices;
}

bool WasapiLoopback::start(int device_index) {
    if (_is_running) {
        std::cerr << "WasapiLoopback: already running\n";
        return false;
    }

    auto devices = list_devices();
    if (devices.empty()) {
        std::cerr << "WasapiLoopback: no output devices found\n";
        return false;
    }

    std::string target_id;
    if (device_index < 0) {
        // Use default device
        for (const auto& dev : devices) {
            if (dev.is_default) {
                target_id = dev.id;
                std::cout << "WasapiLoopback: using default device: " << dev.name << "\n";
                break;
            }
        }
        if (target_id.empty() && !devices.empty()) {
            target_id = devices[0].id;
            std::cout << "WasapiLoopback: using first device: " << devices[0].name << "\n";
        }
    } else if (device_index < static_cast<int>(devices.size())) {
        target_id = devices[device_index].id;
        std::cout << "WasapiLoopback: using device: " << devices[device_index].name << "\n";
    } else {
        std::cerr << "WasapiLoopback: invalid device index\n";
        return false;
    }

    return start_by_id(target_id);
}

bool WasapiLoopback::start_by_id(const std::string& device_id) {
    if (_is_running) {
        std::cerr << "WasapiLoopback: already running\n";
        return false;
    }

    if (!initialize_com()) return false;

    HRESULT hr;

    // Create device enumerator
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                          IID_IMMDeviceEnumerator, (void**)&_enumerator);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to create device enumerator\n";
        cleanup();
        return false;
    }

    // Convert device ID to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, device_id.c_str(), -1, nullptr, 0);
    std::wstring wid(wlen - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, device_id.c_str(), -1, &wid[0], wlen);

    // Get device by ID
    hr = _enumerator->GetDevice(wid.c_str(), &_device);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to get device\n";
        cleanup();
        return false;
    }

    // Activate audio client
    hr = _device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&_audio_client);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to activate audio client\n";
        cleanup();
        return false;
    }

    // Get mix format
    WAVEFORMATEX* pwfx = nullptr;
    hr = _audio_client->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to get mix format\n";
        cleanup();
        return false;
    }

    _sample_rate = pwfx->nSamplesPerSec;
    _channels = pwfx->nChannels;
    _bits_per_sample = pwfx->wBitsPerSample;

    std::cout << "WasapiLoopback: format - " << _sample_rate << " Hz, "
              << _channels << " channels, " << _bits_per_sample << " bits\n";

    // Initialize audio client in loopback mode
    // AUDCLNT_STREAMFLAGS_LOOPBACK is the key flag here
    REFERENCE_TIME hnsRequestedDuration = 10000000; // 1 second buffer
    hr = _audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        hnsRequestedDuration,
        0,
        pwfx,
        nullptr);

    CoTaskMemFree(pwfx);

    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to initialize audio client: 0x" << std::hex << hr << std::dec << "\n";
        cleanup();
        return false;
    }

    // Get capture client
    hr = _audio_client->GetService(IID_IAudioCaptureClient, (void**)&_capture_client);
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to get capture client\n";
        cleanup();
        return false;
    }

    // Clear the ring buffer
    _ring_buffer->clear();

    // Start the audio client
    hr = _audio_client->Start();
    if (FAILED(hr)) {
        std::cerr << "WasapiLoopback: Failed to start audio client\n";
        cleanup();
        return false;
    }

    // Start capture thread
    _is_running = true;
    _capture_thread = std::thread(&WasapiLoopback::capture_thread_func, this);

    std::cout << "WasapiLoopback: started\n";
    return true;
}

bool WasapiLoopback::start_by_name(const std::string& device_name) {
    if (_is_running) {
        std::cerr << "WasapiLoopback: already running\n";
        return false;
    }

    auto devices = list_devices();
    if (devices.empty()) {
        std::cerr << "WasapiLoopback: no output devices found\n";
        return false;
    }

    // Find device by name (partial match)
    std::string target_id;
    for (const auto& dev : devices) {
        if (dev.name.find(device_name) != std::string::npos ||
            device_name.find(dev.name) != std::string::npos) {
            target_id = dev.id;
            std::cout << "WasapiLoopback: matched device by name: " << dev.name << "\n";
            break;
        }
    }

    if (target_id.empty()) {
        // Try exact match
        for (const auto& dev : devices) {
            if (dev.name == device_name) {
                target_id = dev.id;
                std::cout << "WasapiLoopback: exact match device: " << dev.name << "\n";
                break;
            }
        }
    }

    if (target_id.empty()) {
        std::cerr << "WasapiLoopback: device not found: " << device_name << "\n";
        std::cerr << "Available devices:\n";
        for (const auto& dev : devices) {
            std::cerr << "  - " << dev.name << "\n";
        }
        return false;
    }

    return start_by_id(target_id);
}

void WasapiLoopback::stop() {
    if (!_is_running) return;

    _is_running = false;

    if (_capture_thread.joinable()) {
        _capture_thread.join();
    }

    if (_audio_client) {
        _audio_client->Stop();
    }

    cleanup();

    std::cout << "WasapiLoopback: stopped\n";
}

bool WasapiLoopback::is_running() const {
    return _is_running;
}

void WasapiLoopback::capture_thread_func() {
    // Re-initialize COM for this thread
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool thread_com_init = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

    while (_is_running) {
        UINT32 packetLength = 0;
        hr = _capture_client->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            std::cerr << "WasapiLoopback: GetNextPacketSize failed\n";
            break;
        }

        while (packetLength > 0 && _is_running) {
            BYTE* pData = nullptr;
            UINT32 numFramesAvailable = 0;
            DWORD flags = 0;

            hr = _capture_client->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
            if (FAILED(hr)) {
                std::cerr << "WasapiLoopback: GetBuffer failed\n";
                break;
            }

            if (numFramesAvailable > 0) {
                // Convert samples to double and write to ring buffer
                std::vector<double> samples(numFramesAvailable * _channels);

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    // Silent buffer - write zeros
                    std::fill(samples.begin(), samples.end(), 0.0);
                } else if (pData) {
                    // Convert based on format
                    if (_bits_per_sample == 32) {
                        // Assume float32
                        const float* floatData = reinterpret_cast<const float*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable * _channels; i++) {
                            samples[i] = static_cast<double>(floatData[i]);
                        }
                    } else if (_bits_per_sample == 16) {
                        // 16-bit signed int
                        const int16_t* intData = reinterpret_cast<const int16_t*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable * _channels; i++) {
                            samples[i] = static_cast<double>(intData[i]) / 32768.0;
                        }
                    } else if (_bits_per_sample == 24) {
                        // 24-bit packed (3 bytes per sample)
                        const uint8_t* byteData = pData;
                        for (UINT32 i = 0; i < numFramesAvailable * _channels; i++) {
                            int32_t sample = (byteData[i * 3 + 2] << 16) |
                                            (byteData[i * 3 + 1] << 8) |
                                            (byteData[i * 3 + 0]);
                            // Sign extend
                            if (sample & 0x800000) sample |= 0xFF000000;
                            samples[i] = static_cast<double>(sample) / 8388608.0;
                        }
                    }
                }

                _ring_buffer->write(samples.data(), samples.size());
            }

            hr = _capture_client->ReleaseBuffer(numFramesAvailable);
            if (FAILED(hr)) {
                std::cerr << "WasapiLoopback: ReleaseBuffer failed\n";
                break;
            }

            hr = _capture_client->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }

        // Sleep a bit to avoid busy-waiting
        Sleep(5);
    }

    if (thread_com_init) {
        CoUninitialize();
    }
}

arma::Mat<double> WasapiLoopback::get_buffer(int num_samples) {
    if (_channels <= 0 || num_samples <= 0) {
        return arma::Mat<double>();
    }

    size_t total_samples = static_cast<size_t>(num_samples) * static_cast<size_t>(_channels);
    std::vector<double> raw_samples(total_samples, 0.0);

    size_t available = _ring_buffer->available();
    size_t to_read = std::min(total_samples, available);

    if (to_read > 0) {
        _ring_buffer->read(raw_samples.data(), to_read);
    }

    // Deinterleave into matrix (num_channels x num_samples)
    arma::Mat<double> result(_channels, num_samples, arma::fill::zeros);

    size_t frames_read = to_read / static_cast<size_t>(_channels);
    for (size_t frame = 0; frame < frames_read; frame++) {
        for (int ch = 0; ch < _channels; ch++) {
            result(ch, frame) = raw_samples[frame * _channels + ch];
        }
    }

    return result;
}

arma::Row<double> WasapiLoopback::get_buffer_mono(int num_samples) {
    arma::Mat<double> stereo = get_buffer(num_samples);
    if (stereo.n_rows == 0) {
        return arma::Row<double>(num_samples, arma::fill::zeros);
    }

    // Mix down to mono (average of all channels)
    arma::Row<double> mono = arma::mean(stereo, 0);
    return mono;
}

arma::Mat<double> WasapiLoopback::peek_buffer(int num_samples) const {
    if (_channels <= 0 || num_samples <= 0) {
        return arma::Mat<double>();
    }

    size_t total_samples = static_cast<size_t>(num_samples) * static_cast<size_t>(_channels);
    std::vector<double> raw_samples(total_samples, 0.0);

    size_t samples_read = _ring_buffer->peek(raw_samples.data(), total_samples);

    // Deinterleave into matrix (num_channels x num_samples)
    arma::Mat<double> result(_channels, num_samples, arma::fill::zeros);

    size_t frames_read = samples_read / static_cast<size_t>(_channels);
    for (size_t frame = 0; frame < frames_read; frame++) {
        for (int ch = 0; ch < _channels; ch++) {
            result(ch, frame) = raw_samples[frame * _channels + ch];
        }
    }

    return result;
}

int WasapiLoopback::get_sample_rate() const {
    return _sample_rate;
}

int WasapiLoopback::get_channels() const {
    return _channels;
}

} // namespace daw
} // namespace vibe

#endif // _WIN32
