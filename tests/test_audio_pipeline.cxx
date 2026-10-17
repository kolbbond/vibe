// Tests for spectral analysis helpers (moved to vibe-audio library)
#include "vibe/audio/SpectralAnalysis.hxx"

#include <cassert>
#include <cmath>
#include <iostream>

static void test_compute_levels_silence() {
    arma::Mat<fltp> silence(1, 1024, arma::fill::zeros);
    auto levels = vibe::daw::compute_levels(silence);
    assert(levels.rms_db == -60.0f);
    assert(levels.peak_db == -60.0f);
    std::cout << "  PASS: silence returns -60 dB\n";
}

static void test_compute_levels_signal() {
    arma::Mat<fltp> signal(1, 1024, arma::fill::ones);
    auto levels = vibe::daw::compute_levels(signal);
    // RMS of all-ones should be 0 dB
    assert(std::abs(levels.rms_db - 0.0f) < 0.1f);
    assert(std::abs(levels.peak_db - 0.0f) < 0.1f);
    std::cout << "  PASS: all-ones signal returns ~0 dB\n";
}

static void test_spectral_flux_identical() {
    std::vector<double> a = {0.1, 0.2, 0.3, 0.4};
    double flux = vibe::daw::compute_spectral_flux(a, a);
    assert(flux == 0.0);
    std::cout << "  PASS: identical frames give zero flux\n";
}

static void test_spectral_flux_increase() {
    std::vector<double> prev = {0.1, 0.2, 0.3};
    std::vector<double> curr = {0.5, 0.2, 0.6};
    double flux = vibe::daw::compute_spectral_flux(curr, prev);
    // Only positive differences: (0.5-0.1) + (0.6-0.3) = 0.4 + 0.3 = 0.7
    assert(std::abs(flux - 0.7) < 1e-10);
    std::cout << "  PASS: increase gives positive flux\n";
}

static void test_spectral_flux_decrease() {
    std::vector<double> prev = {0.5, 0.5, 0.5};
    std::vector<double> curr = {0.1, 0.1, 0.1};
    double flux = vibe::daw::compute_spectral_flux(curr, prev);
    // All decreases, flux should be 0 (only counts positive diffs)
    assert(flux == 0.0);
    std::cout << "  PASS: decrease gives zero flux\n";
}

static void test_chromagram_a440() {
    // Create a single-frequency signal at 440 Hz (A4)
    int n = 4096;
    arma::Row<fltp> freqs(n);
    arma::Mat<fltp> mags(1, n, arma::fill::zeros);

    double sample_rate = 44100.0;
    for (int i = 0; i < n; i++) {
        freqs(i) = static_cast<double>(i) * sample_rate / n;
    }

    // Put energy only at 440 Hz bin
    int bin_440 = static_cast<int>(440.0 * n / sample_rate);
    mags(0, bin_440) = 1.0;

    std::vector<double> chromagram;
    vibe::daw::compute_chromagram(chromagram, freqs, mags);

    assert(chromagram.size() == 12);
    // A is note index 9 (C=0, C#=1, ..., A=9)
    int strongest = 0;
    for (int i = 1; i < 12; i++) {
        if (chromagram[i] > chromagram[strongest]) strongest = i;
    }
    assert(strongest == 9);
    std::cout << "  PASS: 440 Hz maps to A (index 9)\n";
}

static void test_rebuild_bin_mapping() {
    std::vector<int> mapping;
    vibe::daw::rebuild_bin_mapping(mapping, 1024, 44100, 64);

    assert(mapping.size() == 1024);

    // Bin 0 (DC) should be -1 (below 20 Hz)
    assert(mapping[0] == -1);

    // High bins above Nyquist range should be -1
    assert(mapping[1023] == -1);

    // A bin at ~440 Hz should map to a valid spectrogram bin
    int bin_440 = static_cast<int>(440.0 * 1024 / 44100);
    assert(mapping[bin_440] >= 0 && mapping[bin_440] < 64);

    std::cout << "  PASS: bin mapping has valid indices\n";
}

int main() {
    std::cout << "SpectralAnalysis tests:\n";

    test_compute_levels_silence();
    test_compute_levels_signal();
    test_spectral_flux_identical();
    test_spectral_flux_increase();
    test_spectral_flux_decrease();
    test_chromagram_a440();
    test_rebuild_bin_mapping();

    std::cout << "All SpectralAnalysis tests passed.\n";
    return 0;
}
