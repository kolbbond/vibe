// WavPlayer to load and play wav files

// include header
#include <cassert>
#include "vibe/common/error.hxx"
#include "WavPlayer.hxx"

namespace vibe { namespace daw {

// constructor
WavPlayer::WavPlayer() {}

// destructor
WavPlayer::~WavPlayer() {}

// factory
ShWavPlayerPr WavPlayer::create() { return std::make_shared<WavPlayer>(); }

// setters

// getters

// static
// callback
// @hey: optimize this, we get underflow callbacks on wsl
//       actually it might be reasonably well optimal
void WavPlayer::write_callback(struct SoundIoOutStream* outstream, int frame_count_min, int frame_count_max) {

	// in write_callback
	PlayBackInfo* playback_info = (PlayBackInfo*)outstream->userdata;
	const AudioInfo* audio_info = static_cast<const AudioInfo*>(playback_info->audio_info);

	// early exit
	if(playback_info->is_done) {
		return;
	}

	// check info?
	assert(!audio_info->data.is_empty());

	int frame_start = playback_info->progress;
	int frame_stop = frame_count_max;
	int frames_left = playback_info->audio_info->frames - playback_info->progress;
	int err;

	// check frames left
	bool wakeup = false;
	if(frames_left == 0) {
		try {
			wakeup = true;
			playback_info->is_done = true;
		} catch(...) {}
		return;
	} else if(frames_left < 0) {
		return;
	} else if(frames_left > frame_count_max) {
		frames_left = frame_count_max;
	} else if(frames_left < frame_count_min) {
		frames_left = frame_count_min;
	}

	int frame_count = frames_left;

	// start writing
	struct SoundIoChannelArea* areas;
	if(wakeup) {
		int frame_count_wakeup = 1;
		err = soundio_outstream_begin_write(outstream, &areas, &frame_count_wakeup);
		soundio_outstream_end_write(outstream);
		return;
	} else if((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
		fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
		exit(1);
	}

	const struct SoundIoChannelLayout* layout = &outstream->layout;

	// walk over frames
	for(int frame = 0; frame < frame_count; frame += 1) {

		double sample = 0;
		int idx = frame_start + frame;
		// while this frame is less than sound buffer length
		if(idx < static_cast<int>(audio_info->data.n_elem)) {

			// make the sample
			if(frame < frame_stop) { sample = audio_info->data(idx); }

		} else {
			// this is the restart
			// @hey: make this a flag
			if(playback_info->loop) {
				frame_start = 0;
				frame_count = frame_count - frame;
				playback_info->progress = 0;
			} else {
				playback_info->is_done = true;
				// end write and error check
				if((err = soundio_outstream_end_write(outstream))) {
					if(err == SoundIoErrorUnderflow) { return; }
					fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
					exit(1);
				}
				return;
			}


			std::cout << std::endl;
		}

		// write to each channel
		for(int channel = 0; channel < layout->channel_count; channel += 1) {
			playback_info->write_sample(areas[channel].ptr, sample);
			areas[channel].ptr += areas[channel].step;
		}
	}

	// end write and error check
	if((err = soundio_outstream_end_write(outstream))) {
		if(err == SoundIoErrorUnderflow) { return; }
		fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
		exit(1);
	}

	// update playback_info->progress based on
	// number of frames are written to buffer
	// updated progress
	playback_info->progress += frame_count;

	// for background music:
	if(playback_info->progress >= playback_info->audio_info->frames) {
		// if application has not exited:
		if(playback_info->loop) {
			playback_info->progress = 0;
		} else {
			playback_info->progress = playback_info->audio_info->frames;
			playback_info->is_done = true;
		}
	}
}

void WavPlayer::underflow_callback(struct SoundIoOutStream* outstream) {
	(void)outstream;
	static int count = 0;
	fprintf(stderr, "underflow %d\n", count++);
}

}} // namespace vibe::daw
