// data storing a wav file

// include header
#include <iostream>

// use FFTW3?
#define ARMA_USE_FFTW3
#include <armadillo>

#include "WavData.hxx"

namespace vibe { namespace daw {

// constructor
WavData::WavData() {}

// destructor
WavData::~WavData() {}

// factory
ShWavDataPr WavData::create() { return std::make_shared<WavData>(); }

// load wav specific data
// return true is load is success
bool WavData::load(std::string fpath) {

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

// static
}} // namespace vibe::daw
