
// data storing a stream

// include header
#include <iostream>

// use FFTW3?
#define ARMA_USE_FFTW3
#include <armadillo>

#include "LibSoundIo.hxx"
#include "StreamData.hxx"

namespace vibe { namespace daw {

// constructor
StreamData::StreamData() {}

// destructor
StreamData::~StreamData() {}

// factory
ShStreamDataPr StreamData::create() { return std::make_shared<StreamData>(); }

// load wav specific data
// return true is load is success
/*
bool StreamData::load(SoundIo* soundio) {

	// load
	std::cout << "loading: " << fpath;
	if(!_audio.load(fpath)) return false;
	std::cout << " ... LOADED!\n";

	// get info
	_sample_rate = _audio.getSampleRate();
	_bit_depth = _audio.getBitDepth();

	_num_samples = _audio.getNumSamplesPerChannel();
	_length_in_seconds = _audio.getLengthInSeconds();

	_num_channels = _audio.getNumChannels();

	// summary helper
	_audio.printSummary();

	_samples = _audio.samples;

	// pull samples into armadillo
	pull_samples();


	return true;
}
*/

// static
}} // namespace vibe::daw
