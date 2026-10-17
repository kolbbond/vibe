// SpectralAnalysis - Pure spectral analysis helpers
#include "SpectralAnalysis.hxx"

#include <algorithm>
#include <cmath>

namespace vibe { namespace daw {

Levels compute_levels(const arma::Mat<fltp>& buffer) {
    float rms = 0.0f;
    float peak = 0.0f;
    if (!buffer.is_empty()) {
        for (arma::uword i = 0; i < buffer.n_elem; i++) {
            float val = std::abs(static_cast<float>(buffer(i)));
            rms += val * val;
            if (val > peak) peak = val;
        }
        rms = std::sqrt(rms / buffer.n_elem);
    }

    float rms_db = (rms > 1e-10f) ? 20.0f * std::log10(rms) : -60.0f;
    float peak_db = (peak > 1e-10f) ? 20.0f * std::log10(peak) : -60.0f;

    rms_db = std::clamp(rms_db, -60.0f, 0.0f);
    peak_db = std::clamp(peak_db, -60.0f, 0.0f);

    return {rms_db, peak_db};
}

void rebuild_bin_mapping(std::vector<int>& mapping, int fft_size,
                         int sample_rate, int num_bins) {
    mapping.resize(fft_size);

    double fmin = 20.0, fmax = 20000.0;
    double log_ratio = std::log(fmax / fmin);

    for (int j = 0; j < fft_size; j++) {
        double freq = static_cast<double>(j) * sample_rate / fft_size;
        if (freq < fmin || freq >= fmax) {
            mapping[j] = -1;
        } else {
            int bin = static_cast<int>(std::log(freq / fmin) / log_ratio * num_bins);
            mapping[j] = std::clamp(bin, 0, num_bins - 1);
        }
    }
}

double compute_spectral_flux(const std::vector<double>& current,
                             const std::vector<double>& previous) {
    double flux = 0.0;
    size_t n = std::min(current.size(), previous.size());
    for (size_t i = 0; i < n; i++) {
        double diff = current[i] - previous[i];
        if (diff > 0) flux += diff;
    }
    return flux;
}

void compute_chromagram(std::vector<double>& chromagram,
                        const arma::Row<fltp>& freqs,
                        const arma::Mat<fltp>& mags) {
    chromagram.resize(12);
    std::fill(chromagram.begin(), chromagram.end(), 0.0);

    for (arma::uword j = 0; j < freqs.n_elem && j < mags.n_cols; j++) {
        double freq = freqs(j);
        if (freq > 20.0 && freq < 5000.0) {
            double midi = 69.0 + 12.0 * std::log2(freq / 440.0);
            int chroma = static_cast<int>(std::round(midi)) % 12;
            if (chroma < 0) chroma += 12;
            chromagram[chroma] += mags(0, j);
        }
    }

    double chroma_max = *std::max_element(chromagram.begin(), chromagram.end());
    if (chroma_max > 0.0) {
        for (double& c : chromagram) c /= chroma_max;
    }
}

void extract_bands(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                   float& bass, float& mids, float& highs) {
    bass = mids = highs = 0.0f;
    int bc = 0, mc = 0, hc = 0;
    arma::uword n = std::min(freqs.n_elem, mags.n_cols);
    for (arma::uword i = 0; i < n; i++) {
        double f = freqs(i), m = mags(0, i);
        if (f >= 20.0 && f < 250.0)          { bass += static_cast<float>(m); bc++; }
        else if (f >= 250.0 && f < 4000.0)   { mids += static_cast<float>(m); mc++; }
        else if (f >= 4000.0 && f <= 20000.0) { highs += static_cast<float>(m); hc++; }
    }
    if (bc > 0) bass /= static_cast<float>(bc);
    if (mc > 0) mids /= static_cast<float>(mc);
    if (hc > 0) highs /= static_cast<float>(hc);
}

void extract_5bands(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                    float& sub, float& kick, float& mid, float& snare, float& hat) {
    sub = kick = mid = snare = hat = 0.0f;
    int sc = 0, kc = 0, mc = 0, snc = 0, hc = 0;
    arma::uword n = std::min(freqs.n_elem, mags.n_cols);
    for (arma::uword i = 0; i < n; i++) {
        double f = freqs(i), m = mags(0, i);
        if (f >= 20.0 && f < 60.0)           { sub   += static_cast<float>(m); sc++;  }
        else if (f >= 60.0 && f < 250.0)      { kick  += static_cast<float>(m); kc++;  }
        else if (f >= 250.0 && f < 2000.0)    { mid   += static_cast<float>(m); mc++;  }
        else if (f >= 2000.0 && f < 8000.0)   { snare += static_cast<float>(m); snc++; }
        else if (f >= 8000.0 && f <= 20000.0) { hat   += static_cast<float>(m); hc++;  }
    }
    if (sc  > 0) sub   /= static_cast<float>(sc);
    if (kc  > 0) kick  /= static_cast<float>(kc);
    if (mc  > 0) mid   /= static_cast<float>(mc);
    if (snc > 0) snare /= static_cast<float>(snc);
    if (hc  > 0) hat   /= static_cast<float>(hc);
}

void extract_5bands_peak(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags,
                         float& sub, float& kick, float& mid, float& snare, float& hat) {
    sub = kick = mid = snare = hat = 0.0f;
    arma::uword n = std::min(freqs.n_elem, mags.n_cols);
    for (arma::uword i = 0; i < n; i++) {
        double f = freqs(i);
        auto m = static_cast<float>(mags(0, i));
        if      (f >= 20.0 && f < 60.0)       { if (m > sub)   sub   = m; }
        else if (f >= 60.0 && f < 250.0)       { if (m > kick)  kick  = m; }
        else if (f >= 250.0 && f < 2000.0)     { if (m > mid)   mid   = m; }
        else if (f >= 2000.0 && f < 8000.0)    { if (m > snare) snare = m; }
        else if (f >= 8000.0 && f <= 20000.0)  { if (m > hat)   hat   = m; }
    }
}

}} // namespace vibe::daw
