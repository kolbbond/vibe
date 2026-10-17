// SpectralAnalysis - Pure spectral analysis helpers (no GUI dependency)
#pragma once

#include <vector>
#include <armadillo>
#include "vibe/common/typedefs.hxx"

namespace vibe { namespace daw {

struct Levels { float rms_db; float peak_db; };

// Compute RMS (dB) and peak (dB) levels from an audio buffer.
// Returns {rms_db, peak_db} clamped to [-60, 0].
Levels compute_levels(const arma::Mat<fltp>& buffer);

// Rebuild the FFT-bin-to-spectrogram-bin mapping table.
void rebuild_bin_mapping(std::vector<int>& mapping, int fft_size,
                         int sample_rate, int num_bins);

// Compute spectral flux (sum of positive differences between current and
// previous spectra). Returns the flux value.
double compute_spectral_flux(const std::vector<double>& current,
                             const std::vector<double>& previous);

// Compute chromagram (12 semitones) from FFT data. Writes normalized
// values into `chromagram` (size 12).
void compute_chromagram(std::vector<double>& chromagram,
                        const arma::Row<fltp>& freqs,
                        const arma::Mat<fltp>& mags);

// Extract 3-band energy (bass/mids/highs) from FFT frequency/magnitude data.
void extract_bands(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                   float& bass, float& mids, float& highs);

// Extract 5-band energy (sub/kick/mid/snare/hat) from FFT frequency/magnitude data.
// Uses average magnitude per band.
void extract_5bands(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                    float& sub, float& kick, float& mid, float& snare, float& hat);

// Extract 5-band peak magnitude (max per band). More responsive for visualization.
void extract_5bands_peak(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                         float& sub, float& kick, float& mid, float& snare, float& hat);

}} // namespace vibe::daw
