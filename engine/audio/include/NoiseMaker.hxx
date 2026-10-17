// NoiseMaker.hh

#pragma once

#include "vibe/common/common.hxx"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

#include <soundio/soundio.h>

#include "Pitch.hxx"
#include "AudioFile.hxx"

namespace vibe {namespace daw{
typedef std::shared_ptr<class NoiseMaker> ShNoiseMakerPr;

//typedef int (*audio_callback)(void*);
typedef void (*audio_callback_fn)(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);

class NoiseMaker {

	// properties
protected:
	// this is soundio specific
	// @hey: wrap to support other sound libs
	enum SoundIoBackend backend_ = SoundIoBackendPulseAudio;

	char* device_id_ = NULL;
	bool raw_ = false;
	char* stream_name_ = NULL;
	double latency_ = 0.0;
	int sample_rate_ = 0;
	double freq_ = 60.0;
	double lastfreq_ = 0.0;


public:
	struct SoundIo* soundio_;
	struct SoundIoDevice* device_;
	struct SoundIoOutStream* outstream_;

	AudioFile<double> _audio;

	//void (NoiseMaker::*write_sample_)(char *ptr, double sample);
	void (*write_sample_)(char* ptr, double sample);

	double seconds_offset_ = 0.0;
	int _sample_offset = 0;
	int _last_sample = 0;

	// volatile tells compiler not to optimizer this
	volatile bool want_pause_ = true;

	// methods
public:
	// constructor
	NoiseMaker();

	int generate();
	// destructor
	~NoiseMaker();

	void destroy();

	void play_demo();
	void play_wav();
	void play_note(char*);
	void play_freq(double);

	void play();
	void pause();

    SoundIo* get_soundio();
	SoundIoDevice* get_device();
	double get_freq();
	AudioFile<double> get_audio();
	void set_freq(double);

	void set_audio(AudioFile<double> audio);
	void set_callback(audio_callback_fn callback);

	static void write_sample_s16ne(char* ptr, double sample);
	static void write_sample_s32ne(char* ptr, double sample);
	static void write_sample_float32ne(char* ptr, double sample);
	static void write_sample_float64ne(char* ptr, double sample);
	static void write_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void empty_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void audio_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void wav_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void buffer_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void underflow_callback(struct SoundIoOutStream* outstream);

	// factory
	static ShNoiseMakerPr create();

	static double square(double);

	// statics
	static double note_to_pitch(char*);
};

}} // namespace vibe
