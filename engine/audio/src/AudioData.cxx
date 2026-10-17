// virtual
// data storing a wav file

// include header
#include <iostream>

// use FFTW3?
#define ARMA_USE_FFTW3
#include <armadillo>

#include "AudioData.hxx"

namespace vibe {

//AudioData::AudioData() {}
AudioData::~AudioData() {}

// factory
ShAudioDataPr AudioData::create() {
	return std::make_shared<AudioData>();
}

// audio data
void AudioData::set_data(arma::Row<fltp> tvec, arma::Mat<fltp> smat) {
	// assumes input times give sample rate

    // num samples
    arma::uword num_samples = tvec.n_elem;

    // check derivative
    arma::Row<fltp> dt = arma::diff(tvec);

    // find num samples in each second
    arma::uword sample_rate = 44100;

    // find sample rate
    set_data(tvec,smat,sample_rate);
}

void AudioData::set_data(arma::Row<fltp> tvec, arma::Mat<fltp> smat, arma::uword sample_rate) {
	// check
	assert(tvec.n_elem == smat.n_cols);

	_tvec = tvec;
	_smat = smat;
	_sample_rate = sample_rate;
}

arma::Row<double> AudioData::get_time_vector() {
	return _tvec;
}

arma::Mat<double> AudioData::get_sample_matrix() {
	return _smat;
}

arma::uword AudioData::get_sample_rate(){
    return _sample_rate;
}

// pull samples out of audiofile
void AudioData::pull_samples() {

	// allocate
	_tvec.set_size(_num_samples);
	_smat.set_size(_num_channels, _num_samples);

	//std::cout << "fill sample vectors\n";

	// walk the samples
	//printf("i,j...: ");
	for(int i = 0; i < _num_samples; i++) {

		_tvec(i) = i;

		// walk over channels
		for(int j = 0; j < _num_channels; j++) {
			_smat(j, i) = _samples[j][i];
		}
	}

	//printf("_tvec(%llu,%llu)\n", _tvec.n_rows, _tvec.n_cols);
	//printf("_smat(%llu,%llu)\n", _smat.n_rows, _smat.n_cols);
}
// setters

// getters

AudioFile<double> AudioData::get_audio() {
	return _audio;
}

// static
} // namespace vibe
