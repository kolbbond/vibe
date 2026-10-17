// SpectralView - Reusable spectrogram/flux/chromagram widget for ImGui/ImPlot
// Follows the FFTView pattern: internal state + feed method + draw methods
#pragma once

#include <vector>
#include <armadillo>
#include "vibe/common/typedefs.hxx"

namespace vibe { namespace gfx {

class SpectralView {
public:
    SpectralView();

    // Feed FFT results each frame. Internally updates spectrogram, flux, onset
    // detection, and chromagram.
    void update(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                int fft_size, int sample_rate, double current_time_sec);

    // Draw individual sections (call inside your own ImGui window)
    void draw_spectrogram();
    void draw_spectral_flux();
    void draw_chromagram();

    // --- Public settings ---
    bool show_spectrogram = true;
    bool show_spectral_flux = true;
    bool show_chromagram = true;
    int spectrogram_bins = 128;
    int spectrogram_history = 128;

    // --- Read-only accessors ---
    double flux_threshold() const { return flux_threshold_; }
    const std::vector<double>& onset_times() const { return onset_times_; }
    const std::vector<double>& chromagram() const { return chromagram_; }
    float estimated_bpm() const { return estimated_bpm_; }

private:
    // Spectrogram circular buffer
    std::vector<std::vector<double>> spectrogram_data_;
    int spectrogram_write_idx_ = 0;

    // Spectral flux / onset detection
    std::vector<double> prev_spectrum_;
    std::vector<double> flux_history_;
    int flux_write_idx_ = 0;
    double flux_threshold_ = 0.0;
    std::vector<double> onset_times_;

    // Chromagram
    std::vector<double> chromagram_;

    // BPM estimation
    float estimated_bpm_ = 0.0f;
    static constexpr const char* note_names_[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    // Bin mapping cache
    std::vector<int> fft_to_spec_bin_;
    int last_fft_size_ = 0;
    int last_sample_rate_ = 0;

    // Binning work buffers
    std::vector<double> current_spectrum_;
    std::vector<int> bin_counts_;
};

}} // namespace vibe::gfx
