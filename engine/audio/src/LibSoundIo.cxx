// libsoundio functions

#include <cstring>


#include "vibe/common/Extra.hxx"
#include "LibSoundIo.hxx"

namespace vibe { namespace daw {

LibSoundIo::LibSoundIo() {
	//
}

ShLibSoundIoPr LibSoundIo::create() { return std::make_shared<LibSoundIo>(); }


int LibSoundIo::init() {

	// start the soundio
	_soundio = soundio_create();
	if(!_soundio) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	// setup backend
	int err = (_backend == SoundIoBackendNone) ? soundio_connect(_soundio) : soundio_connect_backend(_soundio, _backend);

	if(err) {
		fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
		return 1;
	}

	fprintf(stderr, "Backend: %s\n", soundio_backend_name(_soundio->current_backend));

	soundio_flush_events(_soundio);

	// check devices
	int selected_device_index = -1;
	if(_device_id) {
		int device_count = soundio_output_device_count(_soundio);
		for(int i = 0; i < device_count; i += 1) {
			_device = soundio_get_output_device(_soundio, i);
			bool select_this_one = strcmp(_device->id, _device_id) == 0 && _device->is_raw == _raw;
			soundio_device_unref(_device);
			if(select_this_one) {
				selected_device_index = i;
				// break;
				return 0;
			}
		}
	} else {
		selected_device_index = soundio_default_output_device_index(_soundio);
	}

	if(selected_device_index < 0) {
		fprintf(stderr, "Output device not found\n");
		return 1;
	}

	// get the device
	_device = soundio_get_output_device(_soundio, selected_device_index);
	if(!_device) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	fprintf(stderr, "Output device: %s\n", _device->name);
	if(_device->probe_error) {
		fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(_device->probe_error));
		return 1;
	}


	return 0; // success (callers before MusicPlayer ignored the result)
}

void LibSoundIo::get_input_devices() { return get_input_devices(_soundio); }
void LibSoundIo::get_input_devices(SoundIo* soundio) {
	// allocate
	char* in_device_id = NULL;
	bool in_raw = false;

	// from libsoundio microphone example
	int default_in_device_index = soundio_default_input_device_index(soundio);
	if(default_in_device_index < 0) Extra::panic("no input device found");

	int in_device_index = default_in_device_index;
	if(in_device_id) {
		bool found = false;
		for(int i = 0; i < soundio_input_device_count(soundio); i += 1) {
			struct SoundIoDevice* device = soundio_get_input_device(soundio, i);
			if(device->is_raw == in_raw && strcmp(device->id, in_device_id) == 0) {
				in_device_index = i;
				found = true;
				soundio_device_unref(device);
				break;
			}
			soundio_device_unref(device);
		}
		if(!found) Extra::panic("invalid input device id: %s", in_device_id);
	}
}

void LibSoundIo::destroy() {
	// manual destructor
	soundio_device_unref(_device);
	soundio_destroy(_soundio);
}

SoundIo* LibSoundIo::get_soundio() { return _soundio; }
SoundIoDevice* LibSoundIo::get_output_device() { return _device; }

void LibSoundIo::set_backend(SoundIoBackend backend) { _backend = backend; }

// create an outstream
void LibSoundIo::create_outstream(SoundIoOutStream*& outstream, SoundIoDevice*& device) {
	// we will want streams for each sound

	//SoundIoOutStream* outstream;
	outstream = soundio_outstream_create(device);
	if(!outstream) {
		fprintf(stderr, "out of memory\n");
		//return;
	}

	// get outstream format and write sample
	if(soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
		outstream->format = SoundIoFormatFloat32NE;
		_write_sample = write_sample_float32ne;
	} else if(soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
		outstream->format = SoundIoFormatFloat64NE;
		_write_sample = write_sample_float64ne;
	} else if(soundio_device_supports_format(device, SoundIoFormatS32NE)) {
		outstream->format = SoundIoFormatS32NE;
		_write_sample = write_sample_s32ne;
	} else if(soundio_device_supports_format(device, SoundIoFormatS16NE)) {
		outstream->format = SoundIoFormatS16NE;
		_write_sample = write_sample_s16ne;
	} else {
		fprintf(stderr, "No suitable device format available.\n");
		//return;
	}
	//return outstream;
}
bool LibSoundIo::valid_outstream(SoundIoOutStream*& outstream) {
	// check relevant parts of our outstream
	if(outstream->underflow_callback == NULL) return false;
	if(outstream->write_callback == NULL) return false;

	return true;
}

int LibSoundIo::open_outstream(SoundIoOutStream*& outstream) {
	int err;
	if((err = soundio_outstream_open(outstream))) {
		fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
		return 1;
	}

	fprintf(stderr, "Software latency: %f\n", outstream->software_latency);
	if(outstream->layout_error) fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

	return 0;
}


// static definitions
void LibSoundIo::write_sample_s16ne(char* ptr, double sample) {
	// static
	int16_t* buf = (int16_t*)ptr;
	double range = (double)INT16_MAX - (double)INT16_MIN;
	double val = sample * range / 2.0;
	*buf = val;
}

void LibSoundIo::write_sample_s32ne(char* ptr, double sample) {
	// static
	int32_t* buf = (int32_t*)ptr;
	double range = (double)INT32_MAX - (double)INT32_MIN;
	double val = sample * range / 2.0;
	*buf = val;
}

void LibSoundIo::write_sample_float32ne(char* ptr, double sample) {
	// static
	float* buf = (float*)ptr;
	*buf = sample;
}

void LibSoundIo::write_sample_float64ne(char* ptr, double sample) {
	// static
	double* buf = (double*)ptr;
	*buf = sample;
}

// Read sample functions (for input streams)
double LibSoundIo::read_sample_s16ne(const char* ptr) {
	const int16_t* buf = reinterpret_cast<const int16_t*>(ptr);
	return static_cast<double>(*buf) / static_cast<double>(INT16_MAX);
}

double LibSoundIo::read_sample_s32ne(const char* ptr) {
	const int32_t* buf = reinterpret_cast<const int32_t*>(ptr);
	return static_cast<double>(*buf) / static_cast<double>(INT32_MAX);
}

double LibSoundIo::read_sample_float32ne(const char* ptr) {
	const float* buf = reinterpret_cast<const float*>(ptr);
	return static_cast<double>(*buf);
}

double LibSoundIo::read_sample_float64ne(const char* ptr) {
	const double* buf = reinterpret_cast<const double*>(ptr);
	return *buf;
}

// List all available input devices
std::vector<AudioDeviceInfo> LibSoundIo::list_input_devices() {
	std::vector<AudioDeviceInfo> devices;

	// Check if soundio is properly initialized with a backend
	if(!_soundio || _soundio->current_backend == SoundIoBackendNone) {
		return devices;  // Return empty list if not connected
	}

	soundio_flush_events(_soundio);

	int default_idx = soundio_default_input_device_index(_soundio);
	int count = soundio_input_device_count(_soundio);

	for (int i = 0; i < count; i++) {
		SoundIoDevice* device = soundio_get_input_device(_soundio, i);
		if (device) {
			AudioDeviceInfo info;
			info.type = DeviceType::INPUT;
			info.index = i;
			info.id = device->id ? device->id : "";
			info.name = device->name ? device->name : "";
			info.is_raw = device->is_raw;
			info.is_default = (i == default_idx);

			// Get sample rate range
			if (device->sample_rate_count > 0) {
				info.sample_rate_min = device->sample_rates[0].min;
				info.sample_rate_max = device->sample_rates[device->sample_rate_count - 1].max;
			} else {
				info.sample_rate_min = 0;
				info.sample_rate_max = 0;
			}

			devices.push_back(info);
			soundio_device_unref(device);
		}
	}

	return devices;
}

// List all available output devices
std::vector<AudioDeviceInfo> LibSoundIo::list_output_devices() {
	std::vector<AudioDeviceInfo> devices;

	// Check if soundio is properly initialized with a backend
	if(!_soundio || _soundio->current_backend == SoundIoBackendNone) {
		return devices;  // Return empty list if not connected
	}

	soundio_flush_events(_soundio);

	int default_idx = soundio_default_output_device_index(_soundio);
	int count = soundio_output_device_count(_soundio);

	for (int i = 0; i < count; i++) {
		SoundIoDevice* device = soundio_get_output_device(_soundio, i);
		if (device) {
			AudioDeviceInfo info;
			info.type = DeviceType::OUTPUT;
			info.index = i;
			info.id = device->id ? device->id : "";
			info.name = device->name ? device->name : "";
			info.is_raw = device->is_raw;
			info.is_default = (i == default_idx);

			// Get sample rate range
			if (device->sample_rate_count > 0) {
				info.sample_rate_min = device->sample_rates[0].min;
				info.sample_rate_max = device->sample_rates[device->sample_rate_count - 1].max;
			} else {
				info.sample_rate_min = 0;
				info.sample_rate_max = 0;
			}

			devices.push_back(info);
			soundio_device_unref(device);
		}
	}

	return devices;
}

// List all devices (input and output combined)
std::vector<AudioDeviceInfo> LibSoundIo::list_all_devices() {
	std::vector<AudioDeviceInfo> all_devices;

	auto inputs = list_input_devices();
	auto outputs = list_output_devices();

	all_devices.reserve(inputs.size() + outputs.size());
	all_devices.insert(all_devices.end(), inputs.begin(), inputs.end());
	all_devices.insert(all_devices.end(), outputs.begin(), outputs.end());

	return all_devices;
}

SoundIoDevice* LibSoundIo::get_input_device_by_index(int index) {
	// Check if soundio is properly initialized with a backend
	if(!_soundio || _soundio->current_backend == SoundIoBackendNone) {
		fprintf(stderr, "Audio backend not connected\n");
		return nullptr;
	}

	soundio_flush_events(_soundio);

	if (index < 0) {
		// Use default device
		index = soundio_default_input_device_index(_soundio);
	}

	if (index < 0 || index >= soundio_input_device_count(_soundio)) {
		fprintf(stderr, "Invalid input device index: %d\n", index);
		return nullptr;
	}

	return soundio_get_input_device(_soundio, index);
}

void LibSoundIo::create_instream(SoundIoInStream*& instream, SoundIoDevice*& device) {
	instream = soundio_instream_create(device);
	if (!instream) {
		fprintf(stderr, "out of memory creating instream\n");
		return;
	}

	// Select format and read function
	if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
		instream->format = SoundIoFormatFloat32NE;
		_read_sample = read_sample_float32ne;
	} else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
		instream->format = SoundIoFormatFloat64NE;
		_read_sample = read_sample_float64ne;
	} else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {
		instream->format = SoundIoFormatS32NE;
		_read_sample = read_sample_s32ne;
	} else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
		instream->format = SoundIoFormatS16NE;
		_read_sample = read_sample_s16ne;
	} else {
		fprintf(stderr, "No suitable input device format available.\n");
	}
}

bool LibSoundIo::valid_instream(SoundIoInStream*& instream) {
	if (instream == nullptr) return false;
	if (instream->read_callback == nullptr) return false;
	return true;
}

int LibSoundIo::open_instream(SoundIoInStream*& instream) {
	int err;
	if ((err = soundio_instream_open(instream))) {
		fprintf(stderr, "unable to open input device: %s\n", soundio_strerror(err));
		return 1;
	}

	fprintf(stderr, "Input software latency: %f\n", instream->software_latency);
	if (instream->layout_error) {
		fprintf(stderr, "unable to set input channel layout: %s\n", soundio_strerror(instream->layout_error));
	}

	return 0;
}

void LibSoundIo::overflow_callback(struct SoundIoInStream* instream) {
	(void)instream;
	fprintf(stderr, "input overflow!\n");
}

}} // namespace vibe::daw
