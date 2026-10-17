#pragma once

#include <memory>
#include <armadillo>

#include "vibe/common/common.hxx"
#include "AudioFile.hxx"

namespace vibe { namespace daw {

typedef std::shared_ptr<class AudioBuffer> ShAudioBufferPr;

class AudioBuffer {

protected:
	//  processed data
	arma::Mat<double> _buffer; // sample matrix (num_channel X num_samples)
	arma::uword _buffer_size; // max num samples to store
	arma::uword _buffer_idx; //  last idx in buffer played
	arma::uword _sample_rate = 44100; // [Hz]
	arma::uword _num_channels = 2;

public:
	// methods
	// constructor
	AudioBuffer();

	// destructor
	virtual ~AudioBuffer();

	// factory
	static ShAudioBufferPr create();

	// getters
	arma::Mat<fltp> get_buffer();
	void add2buffer(const arma::Row<fltp>& samples, arma::uword cidx); // samples and channel idx
	void add2buffer(const arma::Row<fltp>& samples);

	// static
};

}} // namespace vibe::daw
