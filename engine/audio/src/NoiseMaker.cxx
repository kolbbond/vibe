
// NoiseMaker.cpp
// ###### -

// include header
#include <soundio/soundio.h>

#include "NoiseMaker.hxx"
#include "AudioBuffer.hxx"

namespace vibe { namespace daw{

// constructor
NoiseMaker::NoiseMaker() {}

// destructor
NoiseMaker::~NoiseMaker() {}

void NoiseMaker::play_demo() {

	// printf("playing demo\n");
	soundio_flush_events(soundio_);
	// printf("flush\n");

	// input checking
	bool flag_input = true;
	if(flag_input) {
		int c = getc(stdin);

		printf("get stdin: %c\n", c);
		if(c == 'p') {
			fprintf(stderr, "pausing result: %s\n", soundio_strerror(soundio_outstream_pause(outstream_, true)));
		} else if(c == 'P') {
			want_pause_ = true;
		} else if(c == 'u') {
			want_pause_ = false;
			fprintf(stderr, "unpausing result: %s\n", soundio_strerror(soundio_outstream_pause(outstream_, false)));
		} else if(c == 'c') {
			fprintf(stderr, "clear buffer result: %s\n", soundio_strerror(soundio_outstream_clear_buffer(outstream_)));
		} else if(c == 'q') {

			// break;
			return;
		} else if(c == '\r' || c == '\n') {
			// newline
			double freq = this->get_freq();
			this->set_freq(freq + Pitch::midi2freq(1));
			want_pause_ = true;
			soundio_strerror(soundio_outstream_pause(outstream_, true));
			soundio_strerror(soundio_outstream_clear_buffer(outstream_));
			outstream_->write_callback(outstream_, 0, 1000);
			want_pause_ = false;
			soundio_strerror(soundio_outstream_pause(outstream_, false));

		} else {
			fprintf(stderr, "Unrecognized command: %c\n", c);
		}
	}
}

void NoiseMaker::play_wav() {

	soundio_flush_events(soundio_);

	if(want_pause_) {
		soundio_strerror(soundio_outstream_pause(outstream_, true));
	} else {
		soundio_strerror(soundio_outstream_pause(outstream_, false));
	}
}

ShNoiseMakerPr NoiseMaker::create() {
	// factory
	return std::make_shared<NoiseMaker>();
}

void NoiseMaker::destroy() {
	// manual destructor
	soundio_outstream_destroy(outstream_);
	soundio_device_unref(device_);
	soundio_destroy(soundio_);
}

void NoiseMaker::play_note(char* note) {
	// play a note
	(void)note;
}

void NoiseMaker::play_freq(double freq) {
	// play a freq
	soundio_flush_events(soundio_);

	// play freq
	this->set_freq(freq + Pitch::midi2freq(1));
	want_pause_ = true;
	soundio_strerror(soundio_outstream_pause(outstream_, true));
	soundio_strerror(soundio_outstream_clear_buffer(outstream_));
	outstream_->write_callback(outstream_, 0, 1000);
	want_pause_ = false;
	soundio_strerror(soundio_outstream_pause(outstream_, false));
}

// setters
void NoiseMaker::set_freq(double freq) {
	freq_ = freq;
}

void NoiseMaker::set_audio(AudioFile<double> audio) {
	_audio = audio;
}

void NoiseMaker::set_callback(audio_callback_fn callback) {
	if (outstream_) {
		outstream_->write_callback = callback;
	}
}

// getters
SoundIo* NoiseMaker::get_soundio() {
	return soundio_;
}
SoundIoDevice* NoiseMaker::get_device() {
	return device_;
}

double NoiseMaker::get_freq() {
	return freq_;
}

AudioFile<double> NoiseMaker::get_audio() {
	return _audio;
}

// static

double NoiseMaker::note_to_pitch(char* note) {
	// static
	(void)note;

	return 444;
}

void NoiseMaker::write_sample_s16ne(char* ptr, double sample) {
	// static
	int16_t* buf = (int16_t*)ptr;
	double range = (double)INT16_MAX - (double)INT16_MIN;
	double val = sample * range / 2.0;
	*buf = val;
}

void NoiseMaker::write_sample_s32ne(char* ptr, double sample) {
	// static
	int32_t* buf = (int32_t*)ptr;
	double range = (double)INT32_MAX - (double)INT32_MIN;
	double val = sample * range / 2.0;
	*buf = val;
}

void NoiseMaker::write_sample_float32ne(char* ptr, double sample) {
	// static
	float* buf = (float*)ptr;
	*buf = sample;
}

void NoiseMaker::write_sample_float64ne(char* ptr, double sample) {
	// static
	double* buf = (double*)ptr;
	*buf = sample;
}

void NoiseMaker::play() {
	want_pause_ = false;
}
void NoiseMaker::pause() {
	want_pause_ = true;
}

/* struct based on this nice stackoverflow
   https://stackoverflow.com/questions/64001616/how-to-play-more-than-one-stream-at-once-using-soundio
struct WAVE_INFO
{
    SoundIoFormat format;
    std::vector<unsigned char*> data;
    int frames; // number of frames in this clip
}

struct PLAYBACK_INFO
{
    const WAVE_INFO* wave_info; // information of sound clip
    int progress; // number of frames already played
}

//  all sounds we want to play
std::vector<WAVE_INFO> waves_;

// when we want to play a sound
SoundIoOutStream* outstream = soundio_outstream_create(sound_device_);
outstream->write_callback = write_callback;
PlayBackInfo* playback_info = new PlayBackInfo({&waves_[index], 0});
outstream->format = playback_info->wave_info->format;
outstream->userdata = playback_info;
soundio_outstream_open(outstream);
soundio_outstream_start(outstream);
std::thread stopper([this, outstream]()
{
    PlayBackInfo* playback_info = (PlayBackInfo*)outstream->userdata;
    while (playback_info->progress != playback_info->wave_info->frames)
    {
        soundio_wait_events(soundio_);
    }
    soundio_outstream_destroy(outstream);
    delete playback_info;
});
stopper.detach();

// in write_callback
PlayBackInfo* playback_info = (PlayBackInfo*)outstream->userdata;
int frames_left = playback_info->audio_info->frames - playback_info->progress;
if (frames_left == 0)
{
    soundio_wakeup(Window::window_->soundio_);
    return;
}
if (frames_left > frame_count_max)
{
    frames_left = frame_count_max;
}
// fill the buffer using
// soundio_outstream_begin_write and
// soundio_outstream_end_write by
// data in playback_info->wave_info.data
// considering playback_info->progress.

// update playback_info->progress based on
// number of frames are written to buffer

// for background music:
if (playback_info->audio_info->frames == playback_info->progress)
{
    // if application has not exited:
    playback_info->progress = 0;
}
*/
/*
void NoiseMaker::buffer_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max) {
	// play a wav file (buffer?)
	(void)frame_count_min;


	// get our data
	AudioBuffer* ab = static_cast<AudioBuffer*>(outstream->userdata);

	struct SoundIoChannelArea* areas;
	int frames_left = frame_count_max;
	int err;

	// get samples to write
	const arma::Mat<fltp>& buffer = ab->get_buffer();

	// get info
	int num_samples = 1;

	// check last sample
	if(nm->_sample_offset > num_samples) {
		nm->_sample_offset = 0;
	}

	int frame_stop = frame_count_max;
	double sample_value = 0.0;

	// debug

	// until we run out of frames
	for(;;) {
		int frame_count = frames_left;
		if((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// early exit
		if(!frame_count) {
			break;
		}

		const struct SoundIoChannelLayout* layout = &outstream->layout;

		// walk over frames
		//std::cout << "frame_count: " << frame_count << std::endl;
		//std::cout << "frame: sample: ";
		for(int frame = 0; frame < frame_count; frame += 1) {

			// write to each channel
			for(int channel = 0; channel < layout->channel_count; channel += 1) {

				// make the sample
				if(frame < frame_stop) {

					int sample_idx = frame + nm->_sample_offset;
					if(sample_idx > num_samples) {
						sample_idx = 0;
						//printf("sample_idx overflow!\n");
					}

					sample_value = samples[channel][sample_idx];

				} else {
					sample_value = 0;
				}

				//std::cout << sample_value << " ";

				nm->write_sample_(areas[channel].ptr, sample_value);
				areas[channel].ptr += areas[channel].step;
			}
		}
		//std::cout << std::endl;

		// change sample offset
		nm->_sample_offset = nm->_sample_offset + frame_count;
		//std::cout << "sample offset: " << nm->_sample_offset << std::endl;

		// end write and error check
		if((err = soundio_outstream_end_write(outstream))) {
			if(err == SoundIoErrorUnderflow) {
				return;
			}
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// decrement
		frames_left -= frame_count;
		if(frames_left <= 0) {
			break;
		}
	}

	soundio_outstream_pause(outstream, nm->want_pause_);
	//printf("end of callback\n");
}
*/
void NoiseMaker::wav_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max) {
	// play a wav file (buffer?)
	(void)frame_count_min;

	//printf("wav callback\n");

	// get our data
	NoiseMaker* nm = static_cast<NoiseMaker*>(outstream->userdata);

	struct SoundIoChannelArea* areas;
	int frames_left = frame_count_max;
	int err;

	// get samples to write
	AudioFile<double> audio = nm->get_audio();

	// get info
	int num_samples = audio.getNumSamplesPerChannel();

	//int num_channels = audio.getNumChannels();
	//bool ismono = audio.isMono();
	//bool isstereo = audio.isStereo();

	// get the samples
	// samples[channel][sampleIndex]
	AudioFile<double>::AudioBuffer samples = audio.samples;

	// check last sample
	if(nm->_sample_offset > num_samples) {
		nm->_sample_offset = 0;
	}

	int frame_stop = frame_count_max;
	double sample_value = 0.0;

	// debug

	// until we run out of frames
	for(;;) {
		int frame_count = frames_left;
		if((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// early exit
		if(!frame_count) {
			break;
		}

		const struct SoundIoChannelLayout* layout = &outstream->layout;

		// walk over frames
		//std::cout << "frame_count: " << frame_count << std::endl;
		//std::cout << "frame: sample: ";
		for(int frame = 0; frame < frame_count; frame += 1) {

			// write to each channel
			for(int channel = 0; channel < layout->channel_count; channel += 1) {

				// make the sample
				if(frame < frame_stop) {

					int sample_idx = frame + nm->_sample_offset;
					if(sample_idx > num_samples) {
						sample_idx = 0;
						//printf("sample_idx overflow!\n");
					}

					sample_value = samples[channel][sample_idx];

				} else {
					sample_value = 0;
				}

				//std::cout << sample_value << " ";

				nm->write_sample_(areas[channel].ptr, sample_value);
				areas[channel].ptr += areas[channel].step;
			}
		}
		//std::cout << std::endl;

		// change sample offset
		nm->_sample_offset = nm->_sample_offset + frame_count;
		//std::cout << "sample offset: " << nm->_sample_offset << std::endl;

		// end write and error check
		if((err = soundio_outstream_end_write(outstream))) {
			if(err == SoundIoErrorUnderflow) {
				return;
			}
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// decrement
		frames_left -= frame_count;
		if(frames_left <= 0) {
			break;
		}
	}

	soundio_outstream_pause(outstream, nm->want_pause_);
	//printf("end of callback\n");
}
void NoiseMaker::empty_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max) {

	printf("empty callback\n");
	(void)outstream;
	(void)frame_count_min;
	(void)frame_count_max;
}

void NoiseMaker::write_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max) {
	// printf("write callback\n");
	//  static
	NoiseMaker* nm = static_cast<NoiseMaker*>(outstream->userdata);
	struct SoundIoChannelArea* areas;
	int frames_left = frame_count_max;
	int err;

	// figure out sample rate
	// write @hey, write only integer number of cycles?
	// throw the rest away?
	double float_sample_rate = outstream->sample_rate;
	double seconds_per_frame = 1.0 / float_sample_rate;
	double freq = nm->freq_;
	double lambda = 1.0 / freq;

	// @hey
	(void)lambda;
	(void)frame_count_min;
	(void)frame_count_max;

	// if same freq, connect the dots
	if(freq == nm->lastfreq_) {

	} else {

		// @hey, we should finish the last freq cycle?
		nm->seconds_offset_ = 0.0;
		nm->lastfreq_ = freq;
	}

	// conversion of freq
	double radians_per_second = freq * 2.0 * PI;
	double sample = 0;
	int frame_stop = frame_count_max;

	// debug
	// printf("frame count min/max: %i/%i\n", frame_count_min, frame_count_max);
	// printf("freq: %0.2f\n", freq);

	// until we run out of frames
	for(;;) {
		int frame_count = frames_left;
		if((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// early exit
		if(!frame_count) {
			break;
		}

		const struct SoundIoChannelLayout* layout = &outstream->layout;

		// walk over frames
		for(int frame = 0; frame < frame_count; frame += 1) {

			// make the sample
			if(frame < frame_stop) {

				//sample = sin((nm->seconds_offset_ + frame * seconds_per_frame) *
				//             radians_per_second);
				sample = square((nm->seconds_offset_ + frame * seconds_per_frame) * radians_per_second);
			} else {
				sample = 0;
			}

			// write to each channel
			for(int channel = 0; channel < layout->channel_count; channel += 1) {
				nm->write_sample_(areas[channel].ptr, sample);
				areas[channel].ptr += areas[channel].step;
			}
		}

		nm->seconds_offset_ = fmod(nm->seconds_offset_ + seconds_per_frame * frame_count, 1.0);

		// end write and error check
		if((err = soundio_outstream_end_write(outstream))) {
			if(err == SoundIoErrorUnderflow) {
				return;
			}
			fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
			exit(1);
		}

		// decrement
		frames_left -= frame_count;
		if(frames_left <= 0) {
			break;
		}
	}

	soundio_outstream_pause(outstream, nm->want_pause_);
	// printf("end of callback\n");
}

void NoiseMaker::underflow_callback(struct SoundIoOutStream* outstream) {
	// static
	printf("underflow_callback\n");
	static int count = 0;

	// unused @hey
	(void)count;
	(void)outstream;
	// fprintf(stderr, "underflow %d\n", count++);
}

double NoiseMaker::square(double rad) {
	// square wave 4 quadrant
	double pi = 3.14;
	double mod = 0;

	// check if rad > 2pi
	if(rad > 2 * pi) {
		mod = ceil(rad / (2 * pi)) - 1;
		rad = rad - mod * 2 * pi;
	}

	// debug
	//printf("rad: %0.2f: ",rad);
	//printf("mod: %0.2f\n",mod);

	// must be a better way here
	if(rad >= 0 && rad <= pi / 2) {
		return 0;
	} else if(rad > pi / 2 && rad <= pi) {
		return 1;
	} else if(rad > pi && rad <= 3 * pi / 2) {
		return 0;
	} else if(rad > 3 * pi / 2 && rad <= 2 * pi) {
		return 1;
	} else {
		printf("bad rad\n");
	}

	return 0.0;
}
}} // namespace vibe
