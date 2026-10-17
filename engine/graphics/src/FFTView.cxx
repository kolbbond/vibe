// FFTView - Reusable FFT visualization widget
#include "FFTView.hxx"
#include "Plotter.hxx"

#include <cmath>
#include <algorithm>
#include <cstdio>

namespace vibe { namespace gfx {

FFTView::FFTView() {
    band_mags_.resize(static_cast<size_t>(num_bands), 0.0);
    band_peaks_.resize(static_cast<size_t>(num_bands), 0.0);
    band_centers_.resize(static_cast<size_t>(num_bands), 0.0);
}

void FFTView::set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags) {
    freqs_ = freqs;
    mags_ = mags;
}

void FFTView::set_waveform(const arma::Mat<fltp>& mono_buffer, int sample_rate) {
    sample_rate_ = sample_rate;
    mono_buf_ = mono_buffer;
    update_waveform_display();
}

void FFTView::update_waveform_display() {
    if (mono_buf_.is_empty()) {
        wf_samples_ = 0;
        return;
    }

    // Window the buffer to time_window_ms
    int total_available = static_cast<int>(mono_buf_.n_cols);
    int window_samples = static_cast<int>(time_window_ms * 0.001 * sample_rate_);
    window_samples = std::min(window_samples, total_available);
    window_samples = std::max(window_samples, 1);

    // Take the last window_samples from the buffer (most recent data)
    int start = total_available - window_samples;

    int step = std::max(1, window_samples / 1024);
    int n = window_samples / step;
    wf_data_.resize(static_cast<size_t>(n));
    wf_time_.resize(static_cast<size_t>(n));
    double ms_per_sample = 1000.0 / sample_rate_;
    double window_ms_actual = window_samples * ms_per_sample;
    for (int i = 0; i < n; i++) {
        auto src_idx = static_cast<arma::uword>(start + i * step);
        wf_data_[static_cast<size_t>(i)] = static_cast<double>(mono_buf_(0, src_idx));
        if (center_at_now) {
            wf_time_[static_cast<size_t>(i)] = -window_ms_actual + i * step * ms_per_sample;
        } else {
            wf_time_[static_cast<size_t>(i)] = i * step * ms_per_sample;
        }
    }
    wf_samples_ = n;
}

void FFTView::set_band_energies(float bass, float mids, float highs) {
    bass_ = bass;
    mids_ = mids;
    highs_ = highs;
}

ImVec4 FFTView::bar_color(int idx, int total) {
    float t = static_cast<float>(idx) / static_cast<float>(total - 1);
    float r = std::max(0.0f, 1.0f - 2.0f * t);
    float g = t < 0.5f ? 2.0f * t : 2.0f * (1.0f - t);
    float b = std::max(0.0f, 2.0f * t - 1.0f);
    return ImVec4(r, g, b, 0.8f);
}

void FFTView::update_spectrum_bands() {
    auto nb = static_cast<size_t>(num_bands);
    if (band_mags_.size() != nb) {
        band_mags_.resize(nb, 0.0);
        band_peaks_.resize(nb, 0.0);
        band_centers_.resize(nb, 0.0);
    }

    if (freqs_.is_empty() || mags_.is_empty()) return;

    double fmin = 20.0, fmax = 20000.0;
    for (int i = 0; i < num_bands; i++) {
        auto idx = static_cast<size_t>(i);
        double lo = fmin * std::pow(fmax / fmin, static_cast<double>(i) / num_bands);
        double hi = fmin * std::pow(fmax / fmin, static_cast<double>(i + 1) / num_bands);
        band_centers_[idx] = std::sqrt(lo * hi);

        double sum = 0.0;
        int count = 0;
        for (arma::uword j = 0; j < freqs_.n_elem && j < mags_.n_cols; j++) {
            if (freqs_(j) >= lo && freqs_(j) < hi) {
                sum += mags_(0, j);
                count++;
            }
        }
        double new_val = count > 0 ? sum / count : 0.0;

        if (freq_slope > 0.0f) {
            double octaves = std::log2(band_centers_[idx] / 1000.0);
            new_val *= std::pow(10.0, freq_slope * octaves / 20.0);
        }

        if (new_val > band_mags_[idx]) {
            band_mags_[idx] = new_val;
        } else {
            double s = static_cast<double>(smoothing);
            band_mags_[idx] = s * band_mags_[idx] + (1.0 - s) * new_val;
        }

        if (band_mags_[idx] > band_peaks_[idx])
            band_peaks_[idx] = band_mags_[idx];
        else
            band_peaks_[idx] *= 0.98;
    }
}

void FFTView::draw_window(const char* title, bool* p_open) {
    ImGui::SetNextWindowPos(ImVec2(400, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.8f);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    draw_waveform();
    if (use_classic_view)
        draw_classic_fft();
    else
        draw_detailed_fft();
    draw_spectrum_analyzer();
    draw_band_energy();

    ImGui::End();
}

void FFTView::draw_waveform() {
    if (!ImGui::CollapsingHeader("Waveform", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::SliderFloat("Time Window (ms)", &time_window_ms, 5.0f, 1000.0f, "%.0f ms");
    ImGui::SameLine();
    ImGui::Checkbox("Center at Now", &center_at_now);

    if (wf_samples_ <= 0) return;

    if (ImPlot::BeginPlot("##Waveform", ImVec2(-1, 160))) {
        const char* x_label = center_at_now ? "time [ms] (0 = now)" : "time [ms]";
        ImPlot::SetupAxes(x_label, "amplitude",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("audio", wf_time_.data(), wf_data_.data(), wf_samples_);
        ImPlot::EndPlot();
    }
}

void FFTView::draw_classic_fft() {
    if (!ImGui::CollapsingHeader("FFT (Classic)", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (freqs_.is_empty() || mags_.is_empty()) return;

    if (ImPlot::BeginPlot("##ClassicFFT", ImVec2(-1, 250))) {
        ImPlot::SetupAxes("frequency (log scale)", "magnitude",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        fltp Afreq = 440.0;
        fltp fmin = 1.0;
        fltp fmax = sample_rate_ / 2.0;
        fltp bmin = std::log(fmin / Afreq);
        fltp bmax = std::log(fmax / Afreq);

        arma::Row<fltp> log_freqs = (arma::log(freqs_ / Afreq) - bmin) / (bmax - bmin);

        Plotter::plot_line("FFT", arma::trans(log_freqs), arma::trans(mags_));
        ImPlot::EndPlot();
    }
}

void FFTView::draw_detailed_fft() {
    if (!ImGui::CollapsingHeader("FFT (Detailed)", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (freqs_.is_empty() || mags_.is_empty()) return;

    ImGui::Checkbox("Classic View", &use_classic_view);

    if (ImPlot::BeginPlot("##DetailedFFT", ImVec2(-1, 250))) {
        ImPlot::SetupAxes("frequency [Hz]", "magnitude",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        double fmin = 20.0;
        double fmax_d = freqs_(freqs_.n_elem - 1);
        ImPlot::SetupAxisLimits(ImAxis_X1, fmin, fmax_d, ImPlotCond_Always);

        // Frequency band overlays
        ImPlotRect limits = ImPlot::GetPlotLimits();
        double ymax = limits.Y.Max;

        struct FreqBand { double lo; double hi; ImVec4 color; const char* name; };
        FreqBand bands[] = {
            {20,    60,    ImVec4(0.8f, 0.2f, 0.2f, 0.15f), "Sub"},
            {60,    250,   ImVec4(0.8f, 0.5f, 0.2f, 0.15f), "Bass"},
            {250,   500,   ImVec4(0.8f, 0.8f, 0.2f, 0.15f), "Low-Mid"},
            {500,   2000,  ImVec4(0.2f, 0.8f, 0.2f, 0.15f), "Mid"},
            {2000,  4000,  ImVec4(0.2f, 0.8f, 0.8f, 0.15f), "Up-Mid"},
            {4000,  6000,  ImVec4(0.2f, 0.2f, 0.8f, 0.15f), "Presence"},
            {6000,  20000, ImVec4(0.6f, 0.2f, 0.8f, 0.15f), "Brill"},
        };
        for (const auto& band : bands) {
            double xs[2] = {band.lo, band.hi};
            double ys[2] = {ymax, ymax};
            ImPlot::SetNextFillStyle(band.color);
            ImPlot::PlotShaded(band.name, xs, ys, 2, 0);
        }

        Plotter::plot_line("FFT", arma::trans(freqs_), arma::trans(mags_));
        ImPlot::EndPlot();
    }
}

void FFTView::draw_spectrum_analyzer() {
    if (!ImGui::CollapsingHeader("Spectrum Analyzer", ImGuiTreeNodeFlags_DefaultOpen)) return;

    const char* viz_styles[] = {"Bars", "Line", "Filled", "Stems", "Scatter"};
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("Style", &viz_style, viz_styles, 5);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderInt("Bands", &num_bands, 8, 128);
    ImGui::SameLine();
    ImGui::Checkbox("Peaks", &show_peaks);
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("HF Boost (dB/oct)", &freq_slope, 0.0f, 10.0f, "%.1f");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Boost high frequencies to normalize loudness.\n"
                          "0 = flat, 3 = pink noise, 5 = default, 10 = max");
    }

    update_spectrum_bands();

    if (!ImPlot::BeginPlot("##Spectrum", ImVec2(-1, 220))) return;

    ImPlot::SetupAxes("Hz", "mag", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
    ImPlot::SetupAxisLimits(ImAxis_X1, 20.0, 20000.0, ImPlotCond_Always);

    switch (viz_style) {
    case 0: // Bars
        for (int i = 0; i < num_bands; i++) {
            ImPlot::PushStyleColor(ImPlotCol_Fill, bar_color(i, num_bands));
            double x = band_centers_[static_cast<size_t>(i)];
            double y = band_mags_[static_cast<size_t>(i)];
            char label[24]; snprintf(label, sizeof(label), "##bar%d", i);
            ImPlot::PlotBars(label, &x, &y, 1, 0.9, ImPlotBarsFlags_None);
            ImPlot::PopStyleColor();
        }
        break;
    case 1: // Line
        ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), 2.0f);
        ImPlot::PlotLine("Spectrum", band_centers_.data(), band_mags_.data(), num_bands);
        break;
    case 2: // Filled
        ImPlot::SetNextFillStyle(ImVec4(0.2f, 0.6f, 0.9f, 0.5f));
        ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), 1.5f);
        ImPlot::PlotShaded("Spectrum", band_centers_.data(), band_mags_.data(), num_bands, 0);
        ImPlot::PlotLine("##line", band_centers_.data(), band_mags_.data(), num_bands);
        break;
    case 3: // Stems
        for (int i = 0; i < num_bands; i++) {
            auto idx = static_cast<size_t>(i);
            ImPlot::SetNextLineStyle(bar_color(i, num_bands), 2.0f);
            double xs[2] = {band_centers_[idx], band_centers_[idx]};
            double ys[2] = {0.0, band_mags_[idx]};
            char label[24]; snprintf(label, sizeof(label), "##stem%d", i);
            ImPlot::PlotLine(label, xs, ys, 2);
        }
        for (int i = 0; i < num_bands; i++) {
            auto idx = static_cast<size_t>(i);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5, bar_color(i, num_bands), 1);
            double x = band_centers_[idx], y = band_mags_[idx];
            char label[24]; snprintf(label, sizeof(label), "##dot%d", i);
            ImPlot::PlotScatter(label, &x, &y, 1);
        }
        break;
    case 4: // Scatter
        for (int i = 0; i < num_bands; i++) {
            auto idx = static_cast<size_t>(i);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8, bar_color(i, num_bands), 1);
            double x = band_centers_[idx], y = band_mags_[idx];
            char label[24]; snprintf(label, sizeof(label), "##scat%d", i);
            ImPlot::PlotScatter(label, &x, &y, 1);
        }
        break;
    }

    // Peak indicators
    if (show_peaks) {
        double fmin = 20.0, fmax = 20000.0;
        for (int i = 0; i < num_bands; i++) {
            auto idx = static_cast<size_t>(i);
            ImPlot::SetNextLineStyle(bar_color(i, num_bands), 2.0f);
            double lo = fmin * std::pow(fmax / fmin, static_cast<double>(i) / num_bands);
            double hi = fmin * std::pow(fmax / fmin, static_cast<double>(i + 1) / num_bands);
            double xs[2] = {lo * 1.05, hi * 0.95};
            double ys[2] = {band_peaks_[idx], band_peaks_[idx]};
            char label[24]; snprintf(label, sizeof(label), "##peak%d", i);
            ImPlot::PlotLine(label, xs, ys, 2);
        }
    }
    ImPlot::EndPlot();
}

void FFTView::draw_band_energy() {
    if (!ImGui::CollapsingHeader("Band Energy", ImGuiTreeNodeFlags_DefaultOpen)) return;

    float max_e = std::max({bass_, mids_, highs_, 0.001f});
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::ProgressBar(bass_ / max_e, ImVec2(-1, 0), "Bass");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
    ImGui::ProgressBar(mids_ / max_e, ImVec2(-1, 0), "Mids");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.3f, 1.0f, 1.0f));
    ImGui::ProgressBar(highs_ / max_e, ImVec2(-1, 0), "Highs");
    ImGui::PopStyleColor();
}

}} // namespace vibe::gfx
