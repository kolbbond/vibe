// SpectralView - Reusable spectrogram/flux/chromagram widget
#include "SpectralView.hxx"
#include "vibe/audio/SpectralAnalysis.hxx"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>
#include <smg/implot.h>
#include <smg/implot3d.h>

namespace vibe { namespace gfx {

SpectralView::SpectralView()
    : spectrogram_data_(spectrogram_history, std::vector<double>(spectrogram_bins, 0.0))
    , prev_spectrum_(static_cast<size_t>(spectrogram_bins), 0.0)
    , flux_history_(256, 0.0)
    , chromagram_(12, 0.0)
    , current_spectrum_(static_cast<size_t>(spectrogram_bins), 0.0)
    , bin_counts_(static_cast<size_t>(spectrogram_bins), 0)
{}

void SpectralView::update(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                           int fft_size, int sample_rate, double current_time_sec) {
    if (mags.is_empty()) return;

    // Rebuild bin mapping if needed
    if (last_fft_size_ != fft_size || last_sample_rate_ != sample_rate) {
        vibe::daw::rebuild_bin_mapping(fft_to_spec_bin_, fft_size, sample_rate, spectrogram_bins);
        last_fft_size_ = fft_size;
        last_sample_rate_ = sample_rate;
    }

    // Compute binned spectrum
    std::fill(current_spectrum_.begin(), current_spectrum_.end(), 0.0);
    std::fill(bin_counts_.begin(), bin_counts_.end(), 0);

    arma::uword max_j = std::min(freqs.n_elem, mags.n_cols);
    for (arma::uword j = 0; j < max_j; j++) {
        int bin = (j < fft_to_spec_bin_.size()) ? fft_to_spec_bin_[j] : -1;
        if (bin >= 0) {
            current_spectrum_[bin] += mags(0, j);
            bin_counts_[bin]++;
        }
    }

    for (int i = 0; i < spectrogram_bins; i++) {
        if (bin_counts_[i] > 0) {
            current_spectrum_[i] /= bin_counts_[i];
        }
    }

    // Update spectrogram circular buffer
    spectrogram_data_[spectrogram_write_idx_] = current_spectrum_;
    spectrogram_write_idx_ = (spectrogram_write_idx_ + 1) % spectrogram_history;

    // Spectral flux
    double flux = vibe::daw::compute_spectral_flux(current_spectrum_, prev_spectrum_);
    prev_spectrum_ = current_spectrum_;

    flux_history_[flux_write_idx_] = flux;
    flux_write_idx_ = (flux_write_idx_ + 1) % static_cast<int>(flux_history_.size());

    // Adaptive threshold for onset detection
    double flux_mean = 0.0;
    for (double f : flux_history_) flux_mean += f;
    flux_mean /= flux_history_.size();
    flux_threshold_ = flux_mean * 1.5;

    // Detect onset
    if (flux > flux_threshold_ && flux > 0.01) {
        if (onset_times_.empty() || (current_time_sec - onset_times_.back()) > 0.05) {
            onset_times_.push_back(current_time_sec);
            while (!onset_times_.empty() && (current_time_sec - onset_times_.front()) > 5.0) {
                onset_times_.erase(onset_times_.begin());
            }
        }
    }

    // Estimate BPM from onsets
    if (onset_times_.size() >= 2) {
        double total_interval = 0.0;
        for (size_t i = 1; i < onset_times_.size(); i++) {
            total_interval += onset_times_[i] - onset_times_[i-1];
        }
        double avg_interval = total_interval / static_cast<double>(onset_times_.size() - 1);
        estimated_bpm_ = static_cast<float>(60.0 / avg_interval);
    }

    // Chromagram
    vibe::daw::compute_chromagram(chromagram_, freqs, mags);
}

// ---------------------------------------------------------------------------
// draw_spectrogram
// ---------------------------------------------------------------------------

void SpectralView::draw_spectrogram() {
    if (!show_spectrogram || !ImGui::CollapsingHeader("Spectrogram", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // View mode: 0=Heatmap, 1=Waterfall, 2=Surface 3D
    static int view_mode = 1;  // Default to waterfall
    const char* view_modes[] = { "Heatmap", "Waterfall", "Surface 3D" };
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("View##spectrogram", &view_mode, view_modes, 3);
    ImGui::SameLine();

    // Colormap selection
    static int colormap_idx = 0;
    const char* colormap_names[] = { "Viridis", "Plasma", "Hot", "Jet", "Spectral", "Greys" };
    ImPlotColormap colormaps[] = { ImPlotColormap_Viridis, ImPlotColormap_Plasma, ImPlotColormap_Hot,
                                    ImPlotColormap_Jet, ImPlotColormap_Spectral, ImPlotColormap_Greys };
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("Colormap##spectrogram", &colormap_idx, colormap_names, 6);
    ImGui::SameLine();

    // Scale controls
    static float scale_min = 0.0f;
    static float scale_max = 0.1f;
    static bool auto_scale = true;
    static float spec_hf_boost = 5.0f;  // HF boost for spectrogram (dB/octave)
    ImGui::Checkbox("Auto Scale", &auto_scale);
    if (!auto_scale) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Min##spec", &scale_min, 0.001f, 0.0f, scale_max - 0.001f, "%.3f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Max##spec", &scale_max, 0.001f, scale_min + 0.001f, 1.0f, "%.3f");
    }
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("HF Boost##spec", &spec_hf_boost, 0.0f, 10.0f, "%.1f dB/oct");

    // Build the heatmap data in correct order (oldest to newest) with HF boost
    static std::vector<double> heatmap_data(spectrogram_history * spectrogram_bins);
    heatmap_data.resize(spectrogram_history * spectrogram_bins);
    double data_min = 1e10, data_max = -1e10;
    double fmin_hf = 20.0, fmax_hf = 20000.0;
    for (int t = 0; t < spectrogram_history; t++) {
        int read_idx = (spectrogram_write_idx_ + t) % spectrogram_history;
        for (int f = 0; f < spectrogram_bins; f++) {
            double val = spectrogram_data_[read_idx][f];

            // Apply HF boost (dB/octave relative to 1kHz)
            if (spec_hf_boost > 0.0f) {
                double freq = fmin_hf * std::pow(fmax_hf / fmin_hf, static_cast<double>(f + 0.5) / spectrogram_bins);
                double ref_freq = 1000.0;
                double octaves = std::log2(freq / ref_freq);
                double gain_db = spec_hf_boost * octaves;
                double gain_linear = std::pow(10.0, gain_db / 20.0);
                val *= gain_linear;
            }

            heatmap_data[f * spectrogram_history + t] = val;
            if (val < data_min) data_min = val;
            if (val > data_max) data_max = val;
        }
    }

    // Auto-scale with smoothing
    if (auto_scale && data_max > data_min) {
        scale_min = 0.0f;
        scale_max = static_cast<float>(data_max * 1.1);  // 10% headroom
    }

    // Push the selected colormap
    ImPlot::PushColormap(colormaps[colormap_idx]);

    if (view_mode == 0) {
        // Heatmap view
        if (ImPlot::BeginPlot("##SpectrogramPlot", ImVec2(-60, 250))) {
            ImPlot::SetupAxes("Time", "Freq (log bins)", ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, spectrogram_bins, ImPlotCond_Always);

            // Custom tick labels for frequency (reversed order for correct display)
            double freq_ticks[] = {0, 16, 32, 48, 64, 80, 96, 112, 127};
            const char* freq_labels[] = {"20k", "12k", "5k", "2k", "800", "315", "125", "50", "20"};
            ImPlot::SetupAxisTicks(ImAxis_Y1, freq_ticks, 9, freq_labels);

            ImPlot::PlotHeatmap("##heatmap", heatmap_data.data(), spectrogram_bins, spectrogram_history,
                scale_min, scale_max, nullptr, ImPlotPoint(0, spectrogram_bins), ImPlotPoint(spectrogram_history, 0));
            ImPlot::EndPlot();
        }

        // Draw colorbar
        ImGui::SameLine();
        ImPlot::ColormapScale("##scale", scale_min, scale_max, ImVec2(50, 250));
    } else if (view_mode == 1) {
        // Waterfall view - stacked lines with offset
        static int num_lines = 32;
        static float y_offset = 0.15f;
        static float x_skew = 0.3f;

        ImGui::SetNextItemWidth(80);
        ImGui::SliderInt("Lines##wf", &num_lines, 8, 64);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("Depth##wf", &y_offset, 0.05f, 0.5f, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("Skew##wf", &x_skew, 0.0f, 1.0f, "%.2f");

        if (ImPlot::BeginPlot("##WaterfallPlot", ImVec2(-1, 300))) {
            ImPlot::SetupAxes("Frequency [Hz]", "Magnitude", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            ImPlot::SetupAxisLimits(ImAxis_X1, 20, 20000, ImPlotCond_Always);

            // Frequency axis for each line
            std::vector<double> freq_axis(spectrogram_bins);
            double fmin = 20.0, fmax = 20000.0;
            for (int f = 0; f < spectrogram_bins; f++) {
                freq_axis[f] = fmin * std::pow(fmax / fmin, static_cast<double>(f + 0.5) / spectrogram_bins);
            }

            // Draw lines from back (oldest) to front (newest)
            int step = std::max(1, spectrogram_history / num_lines);
            for (int line = 0; line < num_lines; line++) {
                int t = line * step;
                int read_idx = (spectrogram_write_idx_ + t) % spectrogram_history;

                std::vector<double> x_vals(spectrogram_bins);
                std::vector<double> y_vals(spectrogram_bins);

                float line_offset = (num_lines - 1 - line) * y_offset;
                float x_shift = (num_lines - 1 - line) * x_skew * 0.1f;

                for (int f = 0; f < spectrogram_bins; f++) {
                    // Apply skew to x (pseudo-perspective)
                    x_vals[f] = freq_axis[f] * (1.0 + x_shift);
                    // Get value and apply HF boost
                    double val = spectrogram_data_[read_idx][f];
                    if (spec_hf_boost > 0.0f) {
                        double ref_freq = 1000.0;
                        double octaves = std::log2(freq_axis[f] / ref_freq);
                        double gain_db = spec_hf_boost * octaves;
                        double gain_linear = std::pow(10.0, gain_db / 20.0);
                        val *= gain_linear;
                    }
                    // Normalize and offset y
                    double norm_val = (scale_max > scale_min) ? (val - scale_min) / (scale_max - scale_min) : 0.0;
                    y_vals[f] = norm_val + line_offset;
                }

                // Color based on time position (older = darker)
                float alpha = 0.3f + 0.7f * static_cast<float>(line) / (num_lines - 1);
                ImVec4 line_color = ImPlot::GetColormapColor(line * 255 / num_lines);
                line_color.w = alpha;

                ImPlot::SetNextLineStyle(line_color, 1.5f);
                char label[24];
                snprintf(label, sizeof(label), "##wfline%d", line);
                ImPlot::PlotLine(label, x_vals.data(), y_vals.data(), spectrogram_bins);

                // Fill under the line for solid appearance
                ImPlot::SetNextFillStyle(ImVec4(line_color.x, line_color.y, line_color.z, alpha * 0.3f));
                snprintf(label, sizeof(label), "##wffill%d", line);
                ImPlot::PlotShaded(label, x_vals.data(), y_vals.data(), spectrogram_bins, static_cast<double>(line_offset));
            }

            ImPlot::EndPlot();
        }
    } else if (view_mode == 2) {
        // Surface 3D view using ImPlot3D
        static bool surface_initialized = false;
        static std::vector<double> surf_x, surf_y, surf_z;

        // Build surface data grid
        int x_count = spectrogram_history;
        int y_count = spectrogram_bins;

        if (!surface_initialized || surf_x.size() != static_cast<size_t>(x_count * y_count)) {
            surf_x.resize(x_count * y_count);
            surf_y.resize(x_count * y_count);
            surf_z.resize(x_count * y_count);
            surface_initialized = true;
        }

        // Frequency axis (log-spaced) with HF boost
        double fmin_s = 20.0, fmax_s = 20000.0;
        for (int t = 0; t < x_count; t++) {
            int read_idx = (spectrogram_write_idx_ + t) % spectrogram_history;
            for (int f = 0; f < y_count; f++) {
                int idx = t * y_count + f;
                surf_x[idx] = static_cast<double>(t);  // Time
                double freq = fmin_s * std::pow(fmax_s / fmin_s, static_cast<double>(f) / y_count);
                surf_y[idx] = freq;  // Frequency (log)
                double val = spectrogram_data_[read_idx][f];
                // Apply HF boost
                if (spec_hf_boost > 0.0f) {
                    double ref_freq = 1000.0;
                    double octaves = std::log2(freq / ref_freq);
                    double gain_db = spec_hf_boost * octaves;
                    double gain_linear = std::pow(10.0, gain_db / 20.0);
                    val *= gain_linear;
                }
                surf_z[idx] = val;  // Magnitude
            }
        }

        // ImPlot3D colormap (use same selection)
        ImPlot3DColormap colormaps3d[] = { ImPlot3DColormap_Viridis, ImPlot3DColormap_Plasma, ImPlot3DColormap_Hot,
                                            ImPlot3DColormap_Jet, ImPlot3DColormap_Spectral, ImPlot3DColormap_Greys };
        ImPlot3D::PushColormap(colormaps3d[colormap_idx]);

        if (ImPlot3D::BeginPlot("##Surface3D", ImVec2(-1, 450))) {
            ImPlot3D::SetupAxes("Time", "Freq (log)", "Magnitude");
            ImPlot3D::PlotSurface("##surface", surf_x.data(), surf_y.data(), surf_z.data(),
                x_count, y_count, scale_min, scale_max);
            ImPlot3D::EndPlot();
        }

        ImPlot3D::PopColormap();
    }

    ImPlot::PopColormap();
}

// ---------------------------------------------------------------------------
// draw_spectral_flux
// ---------------------------------------------------------------------------

void SpectralView::draw_spectral_flux() {
    if (!show_spectral_flux || !ImGui::CollapsingHeader("Spectral Flux (Beats)", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Build flux data in correct order
    static std::vector<double> flux_plot_data(flux_history_.size());
    static std::vector<double> flux_x(flux_history_.size());
    flux_plot_data.resize(flux_history_.size());
    flux_x.resize(flux_history_.size());
    for (size_t i = 0; i < flux_history_.size(); i++) {
        size_t read_idx = (flux_write_idx_ + i) % flux_history_.size();
        flux_plot_data[i] = flux_history_[read_idx];
        flux_x[i] = static_cast<double>(i);
    }

    if (ImPlot::BeginPlot("Spectral Flux", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Time", "Flux", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        // Plot flux line
        ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), 1.5f);
        ImPlot::PlotLine("Flux", flux_x.data(), flux_plot_data.data(), static_cast<int>(flux_history_.size()));

        // Plot threshold line
        ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.3f, 0.3f, 0.8f), 1.0f);
        double thresh_x[2] = {0, static_cast<double>(flux_history_.size())};
        double thresh_y[2] = {flux_threshold_, flux_threshold_};
        ImPlot::PlotLine("Threshold", thresh_x, thresh_y, 2);

        ImPlot::EndPlot();
    }

    // Show recent BPM estimate
    if (onset_times_.size() >= 2) {
        double total_interval = 0.0;
        for (size_t i = 1; i < onset_times_.size(); i++) {
            total_interval += onset_times_[i] - onset_times_[i-1];
        }
        double avg_interval = total_interval / (onset_times_.size() - 1);
        double bpm = 60.0 / avg_interval;
        ImGui::Text("Estimated BPM: %.0f (from %zu onsets)", bpm, onset_times_.size());
    } else {
        ImGui::Text("Detecting beats...");
    }
}

// ---------------------------------------------------------------------------
// draw_chromagram
// ---------------------------------------------------------------------------

void SpectralView::draw_chromagram() {
    if (!show_chromagram || !ImGui::CollapsingHeader("Chromagram (Notes)", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (ImPlot::BeginPlot("Chromagram", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Note", "Energy", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, 11.5, ImPlotCond_Always);
        ImPlot::SetupAxisTicks(ImAxis_X1, 0, 11, 12, note_names_, false);

        // Color by note (C=red, moving through rainbow)
        for (int i = 0; i < 12; i++) {
            float hue = static_cast<float>(i) / 12.0f;
            ImVec4 color;
            ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 0.9f, color.x, color.y, color.z);
            color.w = 0.8f;
            ImPlot::SetNextFillStyle(color);
            double x = static_cast<double>(i);
            double y = chromagram_[i];
            ImPlot::PlotBars("##chroma", &x, &y, 1, 0.8);
        }

        ImPlot::EndPlot();
    }

    // Show strongest note
    int strongest = 0;
    for (int i = 1; i < 12; i++) {
        if (chromagram_[i] > chromagram_[strongest]) strongest = i;
    }
    if (chromagram_[strongest] > 0.1) {
        ImGui::Text("Strongest: %s", note_names_[strongest]);
    }
}

}} // namespace vibe::gfx
