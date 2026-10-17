// FFTView - Reusable FFT visualization widget for ImGui/ImPlot
// Provides waveform, classic FFT, detailed FFT, spectrum analyzer, and band energy display
#pragma once

#include <vector>
#include <armadillo>
#include "smg/implot.h"
#include "typedefs.hxx"

namespace vibe { namespace gfx {

class FFTView {
public:
    FFTView();

    // Feed FFT data each frame
    void set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags);

    // Feed raw waveform for time-domain display
    void set_waveform(const arma::Mat<fltp>& mono_buffer, int sample_rate);

    // Feed pre-computed band energies
    void set_band_energies(float bass, float mids, float highs);

    // Draw as a standalone ImGui window (with Begin/End)
    void draw_window(const char* title = "FFT Debug", bool* p_open = nullptr);

    // Draw individual sections (call inside your own ImGui window)
    void draw_waveform();
    void draw_classic_fft();
    void draw_detailed_fft();
    void draw_spectrum_analyzer();
    void draw_band_energy();

    // Settings
    int num_bands = 32;
    int viz_style = 0;    // 0=Bars, 1=Line, 2=Filled, 3=Stems, 4=Scatter
    bool show_peaks = true;
    float freq_slope = 5.0f;
    float smoothing = 0.7f;
    bool use_classic_view = false;
    float time_window_ms = 300.0f;
    bool center_at_now = true;

private:
    void update_spectrum_bands();
    void update_waveform_display();
    static ImVec4 bar_color(int idx, int total);

    // FFT data
    arma::Row<fltp> freqs_;
    arma::Mat<fltp> mags_;
    int sample_rate_ = 44100;

    // Waveform (raw buffer + display)
    arma::Mat<fltp> mono_buf_;
    std::vector<double> wf_data_;
    std::vector<double> wf_time_;
    int wf_samples_ = 0;

    // Band energies
    float bass_ = 0, mids_ = 0, highs_ = 0;

    // Spectrum analyzer state
    std::vector<double> band_mags_;
    std::vector<double> band_peaks_;
    std::vector<double> band_centers_;
};

}} // namespace vibe::gfx
