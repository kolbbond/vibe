// container for fft

// FFT
// https://www.fftw.org/fftw3_doc/The-1d-Discrete-Fourier-Transform-_0028DFT_0029.html
// the fft computes an array Yk where kth output corresponds to the frequency
// k/n (or k/T where T is total sampling period)
// +/- frequencies are in the first/last half of the array
// frequency -k/n is the same as (n-k)/n
// Pretty sure the frequency bins are from DC -> Nyquist
//                                      or 0 -> sample_rate/2
// the magnitude is sqrt(x^2+y^2) of the complex number
// the phase     is atan2(y,x)   ---
//
//
// the matlab code
// % compute the double and single spectrum
// P2 = abs(Y/numts);
// P1 = P2(:,1:numts/2+1);
// P1(:,2:end-1) = 2*P1(:,2:end-1);
//
// for i=1:3
// subplot(3,1,i);
// plot(0:(sr/numts):(sr/2-sr/numts),P1(i,1:numts/2),...
// 'linestyle','none','marker','X');
// xlim([0 100]);
// end
//

// include header
#include "FFT.hxx"

#include <fftw3.h>
#include <mutex>
#include <unordered_map>

namespace vibe { namespace daw {

// Static FFTW plan cache (keyed by FFT size)
static std::mutex s_fftw_mutex;
static std::unordered_map<int, fftw_plan> s_fftw_plans;
static std::unordered_map<int, double*> s_fftw_in_bufs;
static std::unordered_map<int, fftw_complex*> s_fftw_out_bufs;

static fftw_plan get_or_create_plan(int n, double*& in_buf, fftw_complex*& out_buf) {
	std::lock_guard<std::mutex> lock(s_fftw_mutex);
	auto it = s_fftw_plans.find(n);
	if (it != s_fftw_plans.end()) {
		in_buf = s_fftw_in_bufs[n];
		out_buf = s_fftw_out_bufs[n];
		return it->second;
	}
	// Allocate aligned buffers
	in_buf = static_cast<double*>(fftw_malloc(sizeof(double) * static_cast<size_t>(n)));
	out_buf = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * static_cast<size_t>(n / 2 + 1)));
	// FFTW_MEASURE benchmarks to find optimal algorithm (slow first call, fast thereafter)
	fftw_plan plan = fftw_plan_dft_r2c_1d(n, in_buf, out_buf, FFTW_MEASURE);
	s_fftw_plans[n] = plan;
	s_fftw_in_bufs[n] = in_buf;
	s_fftw_out_bufs[n] = out_buf;
	return plan;
}

// constructor
FFT::FFT() {}

// @hey: remove the audiofile support
FFT::FFT(ShAudioDataPr audio) {
	set_audiodata(audio);
}

// destructor
FFT::~FFT() {}

// factory
ShFFTPr FFT::create() {
	return std::make_shared<FFT>();
}
ShFFTPr FFT::create(ShAudioDataPr audio) {
	return std::make_shared<FFT>(audio);
}

// analyse the audio for fft
void FFT::analyze(ShLogPr lg) {
	lg->msg("%s --- Analyzing Set Audio --- %s \n", "\033[38;2;255;0;200;2m", KNRM);

	assert(_audiodata != NULL);

	arma::Mat<fltp> samples = _audiodata->get_sample_matrix();

	int sample_rate = _audiodata->get_sample_rate();
	int nyquist_number = sample_rate / 2;
	arma::uword numts = samples.n_cols;
	printf("numts: %llu\n", numts);

	lg->msg("FFT samples\n");
	// if given a matrix, returns a fft for each column
	// @hey: add fft based on closest power of 2
	arma::uword num_samples = sample_rate;
	printf("ftt samples: %llu\n", num_samples);
	arma::Mat<arma::cx_double> bins = arma::fft(samples.row(1), num_samples);
	printf("bins(%llu,%llu): \n", bins.n_rows, bins.n_cols);

	// convert bins to double and single spectrum
	// % compute the double and single spectrum
	// P2 = abs(Y/numts); // complex magnitude
	// P1 = P2(:,1:numts/2+1); // first half
	// P1(:,2:end-1) = 2*P1(:,2:end-1); // double
	arma::Row<double> P2 = arma::abs(bins.row(0) / num_samples);
	assert(P2.n_cols == num_samples);
	arma::Row<double> P1 = P2.subvec(0, num_samples / 2);
	P1.subvec(1, P1.n_cols - 2) = 2 * P1.subvec(1, P1.n_cols - 2);

	// sample rate steps
	double dsr = static_cast<double>(sample_rate) / num_samples;
	printf("P1 sum: %0.6f\n", arma::sum(P1));

	// 0:(sr/numts):(sr/2-sr/numts)
	arma::Row<double> freqs = arma::regspace<arma::Row<double>>(0, dsr, sample_rate / 2.0 - dsr);

	//std::cout << "freqs: \n" << freqs << std::endl;
	//std::cout << "bins: \n" << bins << std::endl;

	// need to bin into diff frequencies
	bool use_log = false;
	arma::uword num_bins = 4;
}

// return fft frequencies and magnitudes
// function for object
void FFT::calc_frequencies(arma::Row<fltp>& freqs, arma::Mat<fltp>& mags) {

	assert(_audiodata != NULL);

	arma::Mat<fltp> samples = _audiodata->get_sample_matrix();

	int sample_rate = _audiodata->get_sample_rate();

	// allocate output
	calc_frequencies(samples, sample_rate, 0, samples.n_cols - 1, freqs, mags);
}

void FFT::calc_frequencies(const arma::Mat<fltp>& samples,
	const arma::uword sample_rate,
	const arma::uword fs,
	const arma::uword ls,
	arma::Row<fltp>& freqs,
	arma::Mat<fltp>& mags) {

	// early exit
	if(fs == ls) return;

	arma::uword num_samples = ls - fs;

	assert(samples.n_cols >= num_samples);
	assert(fs >= 0 && ls < samples.n_cols);

	if (_use_fftw_direct) {
		// Direct FFTW path with cached plans (FFTW_MEASURE)
		int n = static_cast<int>(num_samples);
		int n_out = n / 2 + 1;

		double* in_buf = nullptr;
		fftw_complex* out_buf = nullptr;
		fftw_plan plan = get_or_create_plan(n, in_buf, out_buf);

		arma::uword n_rows = samples.n_rows;
		mags.set_size(n_rows, n_out);

		for (arma::uword r = 0; r < n_rows; r++) {
			// Copy row into aligned input buffer
			const fltp* row_ptr = samples.colptr(fs) + r;
			for (int i = 0; i < n; i++) {
				in_buf[i] = *(row_ptr + static_cast<arma::uword>(i) * samples.n_rows);
			}

			// Execute the cached plan with new-array interface
			fftw_execute_dft_r2c(plan, in_buf, out_buf);

			// Extract magnitudes (single-sided spectrum)
			for (int i = 0; i < n_out; i++) {
				double re = out_buf[i][0];
				double im = out_buf[i][1];
				double mag = std::sqrt(re * re + im * im) / n;
				// Double non-DC/non-Nyquist bins for single-sided spectrum
				if (i > 0 && i < n_out - 1) mag *= 2.0;
				mags(r, i) = mag;
			}
		}

		// Frequency axis: one entry per bin (a regspace up to sample_rate/2 can round one short)
		double dsr = static_cast<double>(sample_rate) / n;
		freqs = arma::regspace<arma::Row<double>>(0, n_out - 1) * dsr;
	} else {
		// Original arma::fft path
		arma::Mat<arma::cx_double> fbins = arma::trans(arma::fft(arma::trans(samples.cols(fs, ls)), num_samples));

		// convert to double and single spectrum
		arma::Mat<fltp> P2 = arma::abs(fbins / num_samples);
		assert(P2.n_cols == num_samples);
		arma::Mat<double> P1 = P2.cols(0, num_samples / 2);
		if (P1.n_cols > 2) P1.cols(1, P1.n_cols - 2) = 2 * P1.cols(1, P1.n_cols - 2);

		// sample rate steps
		double dsr = static_cast<double>(sample_rate) / num_samples;
		freqs = arma::regspace<arma::Row<double>>(0, P1.n_cols - 1) * dsr;
		mags = P1;
	}
}

// @hey: add a function to bin frequencies together


// setters
void FFT::set_audiodata(ShAudioDataPr audiodata) {
	_audiodata = audiodata;
}

void FFT::set_audio(AudioFile<double> audio) {
	_audio = audio;
}

void FFT::set_use_fftw_direct(bool enabled) {
	_use_fftw_direct = enabled;
}

// getters
bool FFT::get_use_fftw_direct() const {
	return _use_fftw_direct;
}

// static
void FFT::cleanup_fftw() {
	std::lock_guard<std::mutex> lock(s_fftw_mutex);
	for (auto& [size, plan] : s_fftw_plans) {
		fftw_destroy_plan(plan);
	}
	for (auto& [size, buf] : s_fftw_in_bufs) {
		fftw_free(buf);
	}
	for (auto& [size, buf] : s_fftw_out_bufs) {
		fftw_free(buf);
	}
	s_fftw_plans.clear();
	s_fftw_in_bufs.clear();
	s_fftw_out_bufs.clear();
}

}} // namespace vibe::daw
