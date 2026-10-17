// WavPlayer

#pragma once

#include <memory>
#include <armadillo>
//#include "SDL_audio.h"
#include "LibSoundIo.hxx"

namespace vibe { namespace daw {

// struct based on this nice stackoverflow
//  https://stackoverflow.com/questions/64001616/how-to-play-more-than-one-stream-at-once-using-soundio
struct AudioInfo {
	SoundIoFormat format;
	arma::Row<double> data;
	int frames; // number of frames in this clip
};

struct PlayBackInfo {
	const AudioInfo* audio_info; // information of sound clip
	int progress; // number of frames already played
	bool loop; // loop the audio ( for music i.e. )
	SoundIo* soundio; // otherwise how do we get soundio pointer? @FIX:
	void (*write_sample)(char* ptr, double sample);
    bool is_done;
};

typedef std::shared_ptr<class WavPlayer> ShWavPlayerPr;

class WavPlayer {

protected:
	// properties


public:
	// methods
	// constructor
	WavPlayer();

	// destructor
	~WavPlayer();

	// factory
	static ShWavPlayerPr create();

	void load_wav(std::string fpath);

	// setters

	// getters

	// static
	static void write_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max);
	static void underflow_callback(struct SoundIoOutStream* outstream);
};

}} // namespace vibe::daw
