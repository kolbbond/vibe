// fft container
#pragma once

#include <memory>

#include "vibe/common/typedefs.hxx"
#include "vibe/common/Log.hxx"

#include "AudioData.hxx"

namespace vibe { namespace daw {

//typedef std::shared_ptr<class FFT> ShFFTPr;
using ShFFTPr = std::shared_ptr<class FFT>;

class FFT {

protected:
	// properties
	ShAudioDataPr _audiodata;
	AudioFile<double> _audio;
	bool _use_fftw_direct = true;

public:
	// methods
	// constructor
	FFT();
	FFT(ShAudioDataPr);

	// destructor
	~FFT();

	// factory
	static ShFFTPr create();
	static ShFFTPr create(ShAudioDataPr);

	void analyze(ShLogPr lg = vibe::NullLog::create());

	// setters
	void set_audiodata(ShAudioDataPr);
	void set_audio(AudioFile<double>);
	void set_use_fftw_direct(bool enabled);

	// getters
	bool get_use_fftw_direct() const;

	void calc_frequencies(arma::Row<fltp>& freqs, arma::Mat<fltp>& mags);

	// static
	void calc_frequencies(const arma::Mat<fltp>& samples,
		const arma::uword sample_rate,
		const arma::uword fs,
		const arma::uword ls,
		arma::Row<fltp>& freqs,
		arma::Mat<fltp>& mags);

	// Cleanup cached FFTW plans (call at program exit if desired)
	static void cleanup_fftw();
};

}} // namespace vibe::daw
