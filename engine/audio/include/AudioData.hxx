#pragma once

#include <memory>
#include <armadillo>

#include "vibe/common/common.hxx"
#include "AudioFile.hxx"

namespace vibe {

typedef std::shared_ptr<class AudioData> ShAudioDataPr;

class AudioData {

protected:
	// properties
	int _bit_depth, _num_samples, _num_channels;
	int _sample_rate;
	double _length_in_seconds;

	// audiofile object
	AudioFile<double> _audio;

	// samples[channel][sampleIndex]
	AudioFile<double>::AudioBuffer _samples;

	//  processed data
	arma::Row<double> _tvec; // time vector (1 X num_samples)
	arma::Mat<double> _smat; // sample matrix (num_channel X num_samples)


	// fft ?


public:
	// methods
	// constructor
	//AudioData();

	// destructor
	virtual ~AudioData();

	//
	void pull_samples();

	// factory
	static ShAudioDataPr create();

	// setters
	void set_data(arma::Row<fltp> tvec, arma::Mat<fltp> smat);
	void set_data(arma::Row<fltp> tvec, arma::Mat<fltp> smat, arma::uword sample_rate);

	// getters
	AudioFile<double> get_audio();
	arma::Row<double> get_time_vector();
	arma::Mat<double> get_sample_matrix();
	arma::uword get_sample_rate();

	// static
};

} // namespace vibe
