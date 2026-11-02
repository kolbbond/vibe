// Tests for FFT::calc_frequencies: the frequency axis must match the magnitude bins for every length
#include "vibe/audio/FFT.hxx"

#include <cassert>
#include <cmath>
#include <iostream>

// one bin per magnitude column, bin k at k * sample_rate / n (n = ls - fs samples analyzed)
static int check_axis(vibe::daw::FFT& fft, arma::uword n, arma::uword sample_rate) {
    arma::Mat<fltp> samples(2, n + 1, arma::fill::randu);
    arma::Row<fltp> freqs;
    arma::Mat<fltp> mags;
    fft.calc_frequencies(samples, sample_rate, 0, n, freqs, mags);

    if (freqs.n_elem != mags.n_cols || mags.n_rows != 2) {
        std::cerr << "  FAIL: n=" << n << " sr=" << sample_rate << ": " << freqs.n_elem << " freqs vs "
                  << mags.n_cols << " mag bins\n";
        return 1;
    }
    const double dsr = static_cast<double>(sample_rate) / static_cast<double>(n);
    for (arma::uword k = 0; k < freqs.n_elem; k++) {
        if (std::abs(freqs(k) - static_cast<double>(k) * dsr) > 1e-6 * static_cast<double>(sample_rate)) {
            std::cerr << "  FAIL: n=" << n << " sr=" << sample_rate << ": freqs(" << k << ")=" << freqs(k) << "\n";
            return 1;
        }
    }
    return 0;
}

// a file's tail (or a short capture) hands the FFT an arbitrary length, not just the power-of-two buffer size
static void test_axis_matches_bins(bool fftw_direct) {
    auto fft = vibe::daw::FFT::create();
    fft->set_use_fftw_direct(fftw_direct);
    int failures = 0;
    for (arma::uword sample_rate : {44100u, 48000u, 96000u}) {
        // every short length (each new length costs the fftw path a FFTW_MEASURE plan), then the real buffer sizes
        for (arma::uword n = 2; n <= 256 && failures < 5; n++) {
            failures += check_axis(*fft, n, sample_rate);
        }
        for (arma::uword n : {1023u, 1024u, 2047u, 2048u, 4095u, 4096u, 8191u}) {
            failures += check_axis(*fft, n, sample_rate);
        }
    }
    assert(failures == 0);
    std::cout << "  PASS: freqs match mag bins for n = 2..256 and buffer sizes (" << (fftw_direct ? "fftw" : "arma") << ")\n";
}

static void test_peak_at_tone() {
    auto fft = vibe::daw::FFT::create();
    const arma::uword n = 4800;
    const double sample_rate = 48000.0;
    arma::Mat<fltp> samples(1, n + 1);
    for (arma::uword i = 0; i <= n; i++) {
        samples(0, i) = std::sin(2.0 * M_PI * 1000.0 * static_cast<double>(i) / sample_rate);
    }
    arma::Row<fltp> freqs;
    arma::Mat<fltp> mags;
    fft->calc_frequencies(samples, 48000, 0, n, freqs, mags);
    assert(std::abs(freqs(mags.row(0).index_max()) - 1000.0) < 1e-6);
    std::cout << "  PASS: 1 kHz tone peaks at the 1 kHz bin\n";
}

int main() {
    std::cout << "FFT tests:\n";
    test_axis_matches_bins(true);
    test_axis_matches_bins(false);
    test_peak_at_tone();
    std::cout << "All FFT tests passed.\n";
    return 0;
}
