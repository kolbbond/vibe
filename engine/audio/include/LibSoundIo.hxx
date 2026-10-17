// wrappers to handle libsoundio
#pragma once

#include <memory>
#include <cstdio>
#include <vector>
#include <string>

#include <armadillo>

#include <soundio/soundio.h>
#include "AudioLibrary.hxx"

namespace vibe::daw {

using ShLibSoundIoPr = std::shared_ptr<class LibSoundIo>;

// Device type enum
enum class DeviceType { INPUT, OUTPUT };

// Device information structure
struct AudioDeviceInfo {
    DeviceType type;
    int index;
    std::string id;
    std::string name;
    bool is_raw;
    bool is_default;
    int sample_rate_min;
    int sample_rate_max;
};

class LibSoundIo: public AudioLibrary {

    using audio_callback = void (*)(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
    using input_callback = void (*)(struct SoundIoInStream* instream, int frame_count_min, int frame_count_max);

public:
protected:
    // SoundIoBackendNone
    // SoundIoBackendPulseAudio
    // SoundIoBackendPulseAlsa
    // SoundIoBackendPulseJack
    // SoundIoBackendPulse
    enum SoundIoBackend _backend = SoundIoBackendJack;

    char* _device_id = NULL;
    bool _raw = false;
    char* _stream_name = NULL;
    double _latency = 0.0;
    int _sample_rate = 0;

    struct SoundIo* _soundio;
    struct SoundIoDevice* _device;
    struct SoundIoOutStream* _outstream;
    struct SoundIoInStream* _instream;
    struct SoundIoDevice* _input_device;

public:
    void (*_write_sample)(char* ptr, double sample);
    double (*_read_sample)(const char* ptr);

    LibSoundIo();
    static ShLibSoundIoPr create();

    int init(); // initialize soundio and device
    void destroy(); // destroy soundio and device
    void create_outstream(SoundIoOutStream*& outstream, SoundIoDevice*& device);
    bool valid_outstream(SoundIoOutStream*& outstream);

    SoundIo* get_soundio();
    SoundIoDevice* get_device();
    SoundIoDevice* get_output_device();
    void get_input_devices();
    void get_input_devices(SoundIo* soundio);
    static SoundIoDevice* get_input_device(SoundIo* soundio);

    void set_backend(SoundIoBackend backend);
    int open_outstream(SoundIoOutStream*& outstream);

    // Input stream support
    std::vector<AudioDeviceInfo> list_input_devices();
    std::vector<AudioDeviceInfo> list_output_devices();
    std::vector<AudioDeviceInfo> list_all_devices();
    SoundIoDevice* get_input_device_by_index(int index);
    void create_instream(SoundIoInStream*& instream, SoundIoDevice*& device);
    bool valid_instream(SoundIoInStream*& instream);
    int open_instream(SoundIoInStream*& instream);

    // Sample write functions (output)
    static void write_sample_s16ne(char* ptr, double sample);
    static void write_sample_s32ne(char* ptr, double sample);
    static void write_sample_float32ne(char* ptr, double sample);
    static void write_sample_float64ne(char* ptr, double sample);

    // Sample read functions (input)
    static double read_sample_s16ne(const char* ptr);
    static double read_sample_s32ne(const char* ptr);
    static double read_sample_float32ne(const char* ptr);
    static double read_sample_float64ne(const char* ptr);

    static void empty_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
    static void underflow_callback(struct SoundIoOutStream* outstream);
    static void overflow_callback(struct SoundIoInStream* instream);

    static void write_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
};


} // namespace vibe::daw
