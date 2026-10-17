#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include "error.hxx"
#include <armadillo>
#include <thread>

#include "Log.hxx"
#include "error.hxx"
#include "WavData.hxx"
#include "WavPlayer.hxx"
#include "LibSoundIo.hxx"

typedef double fltp;

#include <tclap/CmdLine.h>
int main(int argc, char** argv) {
	// create the sounds

	// logger
	vibe::ShLogPr lg = vibe::Log::create();

	// use tclap to load wav file
	// COMMAND LINE INPUT
	// create tclap object
	TCLAP::CmdLine cmd("Wav Player", ' ');

	// filename input argument
	TCLAP::UnlabeledValueArg<std::string> preset_name("wf", "Wav File", false, "None", "string", cmd);

	// parse the command line arguments
	cmd.parse(argc, argv);
	lg->msg("Wav File: %s%s%s\n", KCYN, preset_name.getValue().c_str(), KNRM);

	// create libsoundio
	vibe::daw::ShLibSoundIoPr lsio = vibe::daw::LibSoundIo::create();
    lsio->set_backend(SoundIoBackendPulseAudio);
	lsio->init();

	// create some outstreams
	SoundIo* soundio = lsio->get_soundio();
	SoundIoOutStream* outstream;
	SoundIoDevice* device = lsio->get_device();
	lsio->create_outstream(outstream, device);

	// audio file
	std::string fpath = std::getenv("VIBE_PATH");
	fpath = preset_name.getValue();
	vibe::ShWavDataPr wd = vibe::WavData::create();
	if(!wd->load(fpath)) assert(0);
	arma::Mat<fltp> samples = wd->get_sample_matrix();

	//  all sounds we want to play
	std::vector<vibe::daw::AudioInfo> waves;
	waves.push_back({outstream->format, samples.row(0), static_cast<int>(samples.n_cols)});

	vibe::daw::PlayBackInfo* playback_info =
		new vibe::daw::PlayBackInfo({&waves[0], 0, true, soundio, lsio->_write_sample});

	// create outstreams
	char* stream_name=NULL;
	fltp latency = 0.05;
    fltp sample_rate = wd->get_sample_rate();
	lsio->create_outstream(outstream, device);
	outstream->userdata = playback_info;
	outstream->write_callback = vibe::daw::WavPlayer::write_callback;
	outstream->underflow_callback =vibe::daw::WavPlayer::underflow_callback ;
	outstream->name = stream_name;
	outstream->software_latency = latency;
	outstream->sample_rate = sample_rate;
    int err;
	if((err = soundio_outstream_open(outstream))) {
		fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
		return 1;
	}

	fprintf(stderr, "Software latency: %f\n", outstream->software_latency);
	if(outstream->layout_error)
		fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

	// play the sound
	// run the stream in a thread?
	bool parallel = true;
	std::thread stopper;
	if(parallel) {
		stopper = std::thread([&]() {
			vibe::daw::PlayBackInfo* pi = (vibe::daw::PlayBackInfo*)outstream->userdata;
			if((err = soundio_outstream_start(outstream))) {
				fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
				return 1;
			}

			// check if samples are over
			while(pi->progress < pi->audio_info->frames) {
				soundio_wait_events(lsio->get_soundio());
			}

			soundio_outstream_destroy(outstream);
			delete pi;

			// have to return success?
			return 0;
		});
		//		printf("detach thread\n");
		stopper.detach();

	} else {
		// serial
		// check if samples are over
		//		printf("serial playback\n");
		if((err = soundio_outstream_start(outstream))) {
			fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
			return 1;
		}
		while(playback_info->progress < playback_info->audio_info->frames) {
			soundio_wait_events(soundio);
		}

		soundio_outstream_destroy(outstream);
	}

	lg->msg("%sPlay Started: %s\n", KGRN, KNRM);

	// game loop
	bool done = false;
	while(!done) {
		//
	}

	// cleanup
	//soundio_outstream_destroy(outstream_);
	soundio_device_unref(lsio->get_device());
	soundio_destroy(lsio->get_soundio());

	// success?
	return 0;
}
