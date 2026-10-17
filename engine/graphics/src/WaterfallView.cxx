// WaterfallView - Scrolling 3D/2D FFT waterfall with OpenGL rendering
#include "WaterfallView.hxx"

#include <cmath>
#include <algorithm>
#include <imgui.h>

#include <Magnum/GL/AbstractFramebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Sampler.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Matrix4.h>
#include <MagnumExternal/OpenGL/GL/flextGL.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace vibe { namespace gfx {

WaterfallView::WaterfallView() {
    ring_buffer_.resize(static_cast<size_t>(history_rows * num_freq_bins), 0.0f);
}

void WaterfallView::set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags) {
    freqs_ = freqs;
    mags_ = mags;
    if (!use_waveform) {
        push_fft_row();
    }
}

void WaterfallView::set_waveform(const arma::Mat<fltp>& mono_buffer, int sample_rate) {
    waveform_ = mono_buffer;
    sample_rate_ = sample_rate;
}

void WaterfallView::set_stereo_waveform(const arma::Mat<fltp>& stereo_buffer, int sample_rate) {
    if (stereo_buffer.n_rows >= 2) {
        stereo_waveform_ = stereo_buffer;
    }
    sample_rate_ = sample_rate;
}

void WaterfallView::set_audio_energy(float sub, float kick, float mid, float snare, float hat) {
    band_energy_[0] = sub;
    band_energy_[1] = kick;
    band_energy_[2] = mid;
    band_energy_[3] = snare;
    band_energy_[4] = hat;
}

void WaterfallView::push_fft_row() {
    if (freqs_.is_empty() || mags_.is_empty()) return;

    auto nb = static_cast<size_t>(num_freq_bins);

    // Resize ring buffer if dimensions changed
    if (last_num_bins_ != num_freq_bins || last_num_rows_ != history_rows) {
        ring_buffer_.assign(static_cast<size_t>(history_rows) * nb, 0.0f);
        ring_idx_ = 0;
        buffer_filled_ = false;
        last_num_bins_ = num_freq_bins;
        last_num_rows_ = history_rows;
        bins_dirty_ = true;
        mesh_dirty_ = true;
    }

    // Rebuild log-spaced bin edges if needed
    if (bins_dirty_) {
        bin_edges_.resize(nb + 1);
        double fmin = 20.0, fmax = 20000.0;
        for (size_t i = 0; i <= nb; i++) {
            bin_edges_[i] = fmin * std::pow(fmax / fmin, static_cast<double>(i) / static_cast<double>(nb));
        }
        bins_dirty_ = false;
    }

    // Aggregate FFT magnitudes into log-spaced bins (peak per bin)
    size_t row_offset = static_cast<size_t>(ring_idx_) * nb;
    for (size_t b = 0; b < nb; b++) {
        double lo = bin_edges_[b];
        double hi = bin_edges_[b + 1];
        float peak = 0.0f;

        for (arma::uword j = 0; j < freqs_.n_elem && j < mags_.n_cols; j++) {
            double f = freqs_(j);
            if (f >= lo && f < hi) {
                float m = static_cast<float>(mags_(0, j));
                if (m > peak) peak = m;
            }
        }

        // HF boost
        if (freq_slope > 0.0f) {
            double center = std::sqrt(lo * hi);
            double octaves = std::log2(center / 1000.0);
            peak *= static_cast<float>(std::pow(10.0, freq_slope * octaves / 20.0));
        }

        // Temporal smoothing
        float prev = ring_buffer_[row_offset + b];
        if (peak > prev) {
            ring_buffer_[row_offset + b] = peak;
        } else {
            ring_buffer_[row_offset + b] = smoothing * prev + (1.0f - smoothing) * peak;
        }
    }

    ring_idx_ = (ring_idx_ + 1) % history_rows;
    if (ring_idx_ == 0) buffer_filled_ = true;
}

void WaterfallView::ensure_initialized() {
    if (initialized_) return;

    shader_ = vibe::WaterfallShader{};
    vertex_buffer_ = GL::Buffer{};
    index_buffer_ = GL::Buffer{};
    line_vertex_buffer_ = GL::Buffer{};
    line_index_buffer_ = GL::Buffer{};
    grid_vertex_buffer_ = GL::Buffer{};
    grid_index_buffer_ = GL::Buffer{};
    sphere_vertex_buffer_ = GL::Buffer{};
    sphere_index_buffer_ = GL::Buffer{};
    ribbon_shader_ = vibe::WaterfallRibbonShader{};
    ribbon_vertex_buffer_ = GL::Buffer{};
    ribbon_index_buffer_ = GL::Buffer{};
    ribbon_line_index_buffer_ = GL::Buffer{};

    create_targets({texture_size, texture_size});

    mesh_dirty_ = true;
    initialized_ = true;
}

// (Re)create the MSAA render target and its mipmapped resolve texture at `size`.
void WaterfallView::create_targets(const Vector2i& size) {
    int samples = std::max(1, std::min(msaa_samples, GL::Renderbuffer::maxSamples()));
    int levels = 1 + static_cast<int>(std::floor(std::log2(static_cast<float>(std::max(size.x(), size.y())))));

    // Resolve target; mipmaps give a clean downsample of the supersampled image
    texture_ = GL::Texture2D{};
    texture_.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
        .setStorage(levels, GL::TextureFormat::RGBA8, size);

    depth_ = GL::Renderbuffer{};
    depth_.setStorage(GL::RenderbufferFormat::DepthComponent24, size);

    fbo_ = GL::Framebuffer{{{}, size}};
    fbo_.attachTexture(GL::Framebuffer::ColorAttachment{0}, texture_, 0)
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, depth_);

    // MSAA render target
    msaa_color_ = GL::Renderbuffer{};
    msaa_color_.setStorageMultisample(samples, GL::RenderbufferFormat::RGBA8, size);

    msaa_depth_ = GL::Renderbuffer{};
    msaa_depth_.setStorageMultisample(samples, GL::RenderbufferFormat::DepthComponent24, size);

    msaa_fbo_ = GL::Framebuffer{{{}, size}};
    msaa_fbo_.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, msaa_color_)
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, msaa_depth_);

    target_size_ = size;
    target_samples_ = msaa_samples;
}

void WaterfallView::update_mesh() {
    int cols = num_freq_bins;
    int rows = history_rows;
    auto nb = static_cast<size_t>(cols);
    auto nr = static_cast<size_t>(rows);

    // Auto-normalization: find running peak across entire buffer
    if (auto_normalize) {
        // Decay peak slowly so it adapts to changing dynamics
        running_peak_ *= 0.999f;
        for (size_t i = 0; i < ring_buffer_.size(); i++) {
            if (ring_buffer_[i] > running_peak_)
                running_peak_ = ring_buffer_[i];
        }
        if (running_peak_ < 0.001f) running_peak_ = 0.001f;
    }

    float norm = auto_normalize ? (1.0f / running_peak_) : 1.0f;

    // Build vertex positions: (col_index, height, row_index)
    vertices_.resize(nb * nr);

    for (size_t r = 0; r < nr; r++) {
        size_t src_row = static_cast<size_t>((ring_idx_ + static_cast<int>(r)) % rows);

        for (size_t b = 0; b < nb; b++) {
            float h = ring_buffer_[src_row * nb + b] * norm;
            vertices_[r * nb + b] = Vector3{
                static_cast<float>(b),
                h,
                static_cast<float>(r)
            };
        }
    }

    // Build index buffer only when grid dimensions change
    if (mesh_dirty_) {
        indices_.clear();
        indices_.reserve((nr - 1) * (nb - 1) * 6);

        for (size_t r = 0; r < nr - 1; r++) {
            for (size_t b = 0; b < nb - 1; b++) {
                auto tl = static_cast<UnsignedInt>(r * nb + b);
                auto tr = static_cast<UnsignedInt>(r * nb + b + 1);
                auto bl = static_cast<UnsignedInt>((r + 1) * nb + b);
                auto br = static_cast<UnsignedInt>((r + 1) * nb + b + 1);

                indices_.push_back(tl);
                indices_.push_back(bl);
                indices_.push_back(tr);

                indices_.push_back(tr);
                indices_.push_back(bl);
                indices_.push_back(br);
            }
        }

        index_buffer_.setData({indices_.data(), indices_.size()});
        mesh_dirty_ = false;
    }

    // Update vertex positions every frame
    vertex_buffer_.setData({vertices_.data(), vertices_.size()});

    // Rebuild mesh object
    mesh_ = GL::Mesh{};
    mesh_.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(static_cast<Int>(indices_.size()))
        .addVertexBuffer(vertex_buffer_, 0, vibe::WaterfallShader::Position{})
        .setIndexBuffer(index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
}

void WaterfallView::update_mesh_2d() {
    size_t num_points;
    std::vector<float> values;

    if (use_waveform) {
        // 2D waveform: read directly from waveform buffer
        if (waveform_.is_empty()) return;
        auto total = static_cast<int>(waveform_.n_cols);
        num_points = static_cast<size_t>(std::min(num_waveform_samples, total));
        if (num_points < 2) return;

        int start = total - static_cast<int>(num_points);
        values.resize(num_points);
        for (size_t i = 0; i < num_points; i++) {
            values[i] = static_cast<float>(waveform_(0, start + static_cast<int>(i)));
        }

        // Auto-normalize from waveform peak
        if (auto_normalize) {
            running_peak_ *= 0.995f;
            for (size_t i = 0; i < num_points; i++) {
                float v = std::abs(values[i]);
                if (v > running_peak_) running_peak_ = v;
            }
            if (running_peak_ < 0.001f) running_peak_ = 0.001f;
        }
    } else {
        // 2D FFT: read newest row from ring buffer
        num_points = static_cast<size_t>(num_freq_bins);
        int rows = history_rows;
        int newest_row = (ring_idx_ - 1 + rows) % rows;
        size_t row_offset = static_cast<size_t>(newest_row) * num_points;

        values.resize(num_points);
        for (size_t i = 0; i < num_points; i++) {
            values[i] = ring_buffer_[row_offset + i];
        }

        // Auto-normalize from ring buffer peak
        if (auto_normalize) {
            running_peak_ *= 0.999f;
            for (size_t i = 0; i < ring_buffer_.size(); i++) {
                if (ring_buffer_[i] > running_peak_)
                    running_peak_ = ring_buffer_[i];
            }
            if (running_peak_ < 0.001f) running_peak_ = 0.001f;
        }
    }

    float norm = auto_normalize ? (1.0f / running_peak_) : 1.0f;

    // Build vertices: for each point, bottom (y=0) + top (y=value)
    std::vector<Vector3> verts;
    verts.reserve(num_points * 2);

    for (size_t b = 0; b < num_points; b++) {
        float h = values[b] * norm;
        float x = static_cast<float>(b);
        verts.push_back(Vector3{x, 0.0f, 0.0f});
        verts.push_back(Vector3{x, h, 0.0f});
    }

    // Build indices for filled quads
    std::vector<UnsignedInt> idx;
    idx.reserve((num_points - 1) * 6);

    for (size_t b = 0; b < num_points - 1; b++) {
        auto bl = static_cast<UnsignedInt>(b * 2);
        auto tl = static_cast<UnsignedInt>(b * 2 + 1);
        auto br = static_cast<UnsignedInt>((b + 1) * 2);
        auto tr = static_cast<UnsignedInt>((b + 1) * 2 + 1);

        idx.push_back(bl);
        idx.push_back(br);
        idx.push_back(tl);

        idx.push_back(tl);
        idx.push_back(br);
        idx.push_back(tr);
    }

    line_vertex_buffer_.setData({verts.data(), verts.size()});
    line_index_buffer_.setData({idx.data(), idx.size()});

    line_mesh_ = GL::Mesh{};
    line_mesh_.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(static_cast<Int>(idx.size()))
        .addVertexBuffer(line_vertex_buffer_, 0, vibe::WaterfallShader::Position{})
        .setIndexBuffer(line_index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
}

void WaterfallView::push_waveform_row() {
    if (waveform_.is_empty()) return;

    int ns = num_waveform_samples;
    int nr = waveform_history_rows;
    auto nb = static_cast<size_t>(ns);

    // Resize if dimensions changed
    if (last_waveform_ns_ != ns || last_waveform_history_rows_ != nr) {
        waveform_ring_.assign(static_cast<size_t>(nr) * nb, 0.0f);
        waveform_ring_idx_ = 0;
        waveform_ring_filled_ = false;
        last_waveform_ns_ = ns;
        last_waveform_history_rows_ = nr;
    }

    size_t row_offset = static_cast<size_t>(waveform_ring_idx_) * nb;

    // Oscilloscope trigger: back off from the newest samples to a rising zero crossing
    // so consecutive rows share a phase instead of jittering
    const int window = (stereo && stereo_waveform_.n_rows >= 2) ? ns / 2 : ns;
    const int delay = trigger ? find_trigger_delay(window) : 0;

    if (stereo && stereo_waveform_.n_rows >= 2) {
        auto stereo_total = static_cast<int>(stereo_waveform_.n_cols);
        int half = ns / 2;
        int start = std::max(0, stereo_total - half - delay);

        for (int i = 0; i < half && (start + i) < stereo_total; i++) {
            // Left channel reversed: center → left edge
            waveform_ring_[row_offset + static_cast<size_t>(half - 1 - i)] =
                static_cast<float>(stereo_waveform_(0, start + i));
            // Right channel: center → right edge
            waveform_ring_[row_offset + static_cast<size_t>(half + i)] =
                static_cast<float>(stereo_waveform_(1, start + i));
        }
    } else {
        auto total = static_cast<int>(waveform_.n_cols);
        int start = std::max(0, total - ns - delay);
        for (int i = 0; i < ns && (start + i) < total; i++) {
            waveform_ring_[row_offset + static_cast<size_t>(i)] =
                static_cast<float>(waveform_(0, start + i));
        }
    }

    waveform_ring_idx_ = (waveform_ring_idx_ + 1) % nr;
    if (waveform_ring_idx_ == 0) waveform_ring_filled_ = true;
}

// Samples to back off from the newest `window` so the window starts on a rising zero
// crossing of the (lightly smoothed) mono signal. 0 if none is found within a search span.
int WaterfallView::find_trigger_delay(int window) const {
    const int total = static_cast<int>(waveform_.n_cols);
    const int span = std::min(4096, total - window - 16);
    if (span <= 0) return 0;
    constexpr int R = 8; // smoothing radius: noise and hats shouldn't fire the trigger
    auto smoothed = [&](int i) {
        double s = 0.0;
        for (int k = -R; k <= R; k++) s += waveform_(0, std::clamp(i + k, 0, total - 1));
        return s;
    };
    const int newest = total - window; // start index with no delay
    double cur = smoothed(newest);
    for (int d = 0; d < span; d++) {
        const int i = newest - d;
        const double prev = smoothed(i - 1);
        if (prev < 0.0 && cur >= 0.0) return d;
        cur = prev;
    }
    return 0;
}

void WaterfallView::update_mesh_waveform_history() {
    push_waveform_row();

    int ns = num_waveform_samples;
    int nr = waveform_history_rows;
    auto nb = static_cast<size_t>(ns);
    auto num_rows = static_cast<size_t>(nr);

    // Auto-normalize from ring buffer
    if (auto_normalize) {
        running_peak_ *= 0.995f;
        for (size_t i = 0; i < waveform_ring_.size(); i++) {
            float v = std::abs(waveform_ring_[i]);
            if (v > running_peak_) running_peak_ = v;
        }
        if (running_peak_ < 0.001f) running_peak_ = 0.001f;
    }
    float norm = auto_normalize ? (1.0f / running_peak_) : 1.0f;

    if (ribbons) {
        // per row: box-filter to ~1 point per pixel, smooth, taper; oldest row first
        ribbon_vertices_.clear();
        ribbon_indices_.clear();
        ribbon_line_indices_.clear();
        const int want = decimate ? ribbon_points() : ns;
        const int half = ns / 2;
        std::vector<float> row, right;
        std::vector<Vector3> pts;
        int row_n = 0;
        for (size_t r = 0; r < num_rows; r++) {
            const float* src = &waveform_ring_[static_cast<size_t>((waveform_ring_idx_ + static_cast<int>(r)) % nr) * nb];
            if (stereo) {
                process_row(src, half, want / 2, row, true, false); // left: outer edge first, center last
                process_row(src + half, ns - half, want / 2, right, false, true); // right: center first
                row.insert(row.end(), right.begin(), right.end());
            } else {
                process_row(src, ns, want, row, true, true);
            }
            row_n = static_cast<int>(row.size());
            const float depth_t = static_cast<float>(r) / static_cast<float>(nr - 1);
            const float scale = depth_t + (1.0f - depth_t) * (1.0f - convergence);
            const float center = static_cast<float>(row_n - 1) * 0.5f;
            pts.resize(row.size());
            for (int j = 0; j < row_n; j++)
                pts[static_cast<size_t>(j)] = Vector3{center + (static_cast<float>(j) - center) * scale,
                                                      row[static_cast<size_t>(j)] * norm, static_cast<float>(r)};
            if (stereo) {
                const int left_n = row_n - static_cast<int>(right.size());
                add_ribbon(pts.data(), left_n, occlude);
                add_ribbon(pts.data() + left_n, row_n - left_n, occlude);
            } else {
                add_ribbon(pts.data(), row_n, occlude);
            }
        }
        ribbon_grid_ = Vector2{static_cast<float>(row_n), static_cast<float>(nr)};
        upload_ribbons();
        return;
    }

    // Build vertices: ns per row, nr rows
    // Oldest row first (far), newest last (close)
    vertices_.resize(nb * num_rows);
    float center_x = static_cast<float>(ns - 1) * 0.5f;

    for (size_t r = 0; r < num_rows; r++) {
        size_t src_row = static_cast<size_t>(
            (waveform_ring_idx_ + static_cast<int>(r)) % nr);

        // Convergence: oldest rows (r=0) narrow toward center,
        // newest rows (r=nr-1) stay full width
        float depth_t = static_cast<float>(r) / static_cast<float>(nr - 1);
        float scale = depth_t + (1.0f - depth_t) * (1.0f - convergence);

        for (size_t i = 0; i < nb; i++) {
            float amp = waveform_ring_[src_row * nb + i] * norm;
            float x = center_x + (static_cast<float>(i) - center_x) * scale;
            vertices_[r * nb + i] = Vector3{
                x,
                amp,
                static_cast<float>(r)
            };
        }
    }

    // Build indices: line segments within each row
    indices_.clear();

    if (stereo) {
        int half = ns / 2;
        indices_.reserve(num_rows * static_cast<size_t>((half - 1 + ns - half - 1) * 2));
        for (size_t r = 0; r < num_rows; r++) {
            auto base = static_cast<UnsignedInt>(r * nb);
            // Left channel line [0..half-1]
            for (int i = 0; i < half - 1; i++) {
                indices_.push_back(base + static_cast<UnsignedInt>(i));
                indices_.push_back(base + static_cast<UnsignedInt>(i + 1));
            }
            // Right channel line [half..ns-1]
            for (int i = half; i < ns - 1; i++) {
                indices_.push_back(base + static_cast<UnsignedInt>(i));
                indices_.push_back(base + static_cast<UnsignedInt>(i + 1));
            }
        }
    } else {
        indices_.reserve(num_rows * static_cast<size_t>((ns - 1) * 2));
        for (size_t r = 0; r < num_rows; r++) {
            auto base = static_cast<UnsignedInt>(r * nb);
            for (int i = 0; i < ns - 1; i++) {
                indices_.push_back(base + static_cast<UnsignedInt>(i));
                indices_.push_back(base + static_cast<UnsignedInt>(i + 1));
            }
        }
    }

    vertex_buffer_.setData({vertices_.data(), vertices_.size()});
    index_buffer_.setData({indices_.data(), indices_.size()});

    mesh_ = GL::Mesh{};
    mesh_.setPrimitive(GL::MeshPrimitive::Lines)
        .setCount(static_cast<Int>(indices_.size()))
        .addVertexBuffer(vertex_buffer_, 0, vibe::WaterfallShader::Position{})
        .setIndexBuffer(index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
}

// ~1 point per output pixel across the full row
int WaterfallView::ribbon_points() const {
    return std::clamp(static_cast<int>(ribbon_viewport_.x() / std::max(ribbon_px_scale_, 1.0f)), 64, 4096);
}

// Box-filter `n` samples down to `out_n` points, smooth, and taper the ends
void WaterfallView::process_row(const float* src, int n, int out_n, std::vector<float>& out, bool taper_head,
                                bool taper_tail) const {
    out_n = std::max(2, std::min(out_n, n));
    out.resize(static_cast<size_t>(out_n));
    for (int j = 0; j < out_n; j++) {
        const int a = static_cast<int>(static_cast<long long>(j) * n / out_n);
        const int b = std::max(a + 1, static_cast<int>(static_cast<long long>(j + 1) * n / out_n));
        float s = 0.0f;
        for (int k = a; k < b; k++) s += src[k];
        out[static_cast<size_t>(j)] = s / static_cast<float>(b - a);
    }
    if (smooth_radius > 0) {
        std::vector<float> tmp(out);
        for (int j = 0; j < out_n; j++) {
            float s = 0.0f;
            for (int k = -smooth_radius; k <= smooth_radius; k++) s += tmp[static_cast<size_t>(std::clamp(j + k, 0, out_n - 1))];
            out[static_cast<size_t>(j)] = s / static_cast<float>(2 * smooth_radius + 1);
        }
    }
    if (taper) { // Tukey window, 10% at each tapered end
        const int k = std::max(1, out_n / 10);
        for (int j = 0; j < k; j++) {
            const float w = 0.5f - 0.5f * std::cos(3.14159265f * static_cast<float>(j) / static_cast<float>(k));
            if (taper_head) out[static_cast<size_t>(j)] *= w;
            if (taper_tail) out[static_cast<size_t>(out_n - 1 - j)] *= w;
        }
    }
}

// Append one polyline: optionally its floor fill first (so it's under the line), then the line quads
void WaterfallView::add_ribbon(const Vector3* pts, int n, bool fill) {
    if (n < 2) return;
    using V = vibe::WaterfallRibbonShader::Vertex;
    if (fill) {
        auto base = static_cast<UnsignedInt>(ribbon_vertices_.size());
        for (int i = 0; i < n; i++) {
            ribbon_vertices_.push_back(V{pts[i], pts[i], pts[i], {0.0f, 1.0f}}); // on the curve
            ribbon_vertices_.push_back(V{pts[i], pts[i], pts[i], {1.0f, 1.0f}}); // on the floor
        }
        for (int i = 0; i < n - 1; i++) {
            const UnsignedInt c0 = base + 2 * i, f0 = c0 + 1, c1 = c0 + 2, f1 = c0 + 3;
            ribbon_indices_.insert(ribbon_indices_.end(), {c0, f0, c1, f0, f1, c1});
        }
    }
    // One quad per segment with that segment's own normal: shared-vertex miter joins
    // collapse to zero width on audio's sharp zig-zags. The shader extends each quad
    // past its ends (start vertex: prev == position, end vertex: next == position).
    for (int i = 0; i < n - 1; i++) {
        const Vector3& a = pts[i];
        const Vector3& b = pts[i + 1];
        auto base = static_cast<UnsignedInt>(ribbon_vertices_.size());
        ribbon_vertices_.push_back(V{a, a, b, {-1.0f, 0.0f}});
        ribbon_vertices_.push_back(V{a, a, b, {1.0f, 0.0f}});
        ribbon_vertices_.push_back(V{b, a, b, {-1.0f, 0.0f}});
        ribbon_vertices_.push_back(V{b, a, b, {1.0f, 0.0f}});
        const std::initializer_list<UnsignedInt> quad{base, base + 1, base + 2, base + 1, base + 3, base + 2};
        ribbon_indices_.insert(ribbon_indices_.end(), quad);
        ribbon_line_indices_.insert(ribbon_line_indices_.end(), quad);
    }
}

void WaterfallView::upload_ribbons() {
    using S = vibe::WaterfallRibbonShader;
    ribbon_vertex_buffer_.setData({ribbon_vertices_.data(), ribbon_vertices_.size()});
    ribbon_index_buffer_.setData({ribbon_indices_.data(), ribbon_indices_.size()});
    ribbon_line_index_buffer_.setData({ribbon_line_indices_.data(), ribbon_line_indices_.size()});

    ribbon_mesh_ = GL::Mesh{};
    ribbon_mesh_.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(static_cast<Int>(ribbon_indices_.size()))
        .addVertexBuffer(ribbon_vertex_buffer_, 0, S::Position{}, S::Prev{}, S::Next{}, S::Side{})
        .setIndexBuffer(ribbon_index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
    ribbon_glow_mesh_ = GL::Mesh{};
    ribbon_glow_mesh_.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(static_cast<Int>(ribbon_line_indices_.size()))
        .addVertexBuffer(ribbon_vertex_buffer_, 0, S::Position{}, S::Prev{}, S::Next{}, S::Side{})
        .setIndexBuffer(ribbon_line_index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
    ribbons_ready_ = true;
}

// Painter's order (oldest row first) with depth writes, so each row's fill hides what's
// behind it; then the halo pass, depth-tested but not written, so halos are occluded too
void WaterfallView::draw_ribbons(const Matrix4& projection, const Matrix4& view, const Vector2& grid_size) {
    // ribbon quads all wind one way and the fluid/game renderers leave culling on
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::LessOrEqual);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One,
                                   GL::Renderer::BlendFunction::OneMinusSourceAlpha); // premultiplied

    ribbon_shader_.setTransformationMatrix(view)
        .setProjectionMatrix(projection)
        .setGridSize(grid_size)
        .setHeightScale(active_height())
        .setViewport(ribbon_viewport_)
        .setLineWidth(line_width * ribbon_px_scale_)
        .setFloor(-active_height() * 1.15f)
        .setColors(wave_color, wave_peak_color)
        .setFillColor({0.0f, 0.0f, 0.0f})
        .setFade(fade)
        .setScanlines(scanlines, time_)
        .setGlow(0.0f)
        .draw(ribbon_mesh_);

    if (glow > 0.0f) {
        GL::Renderer::setDepthMask(false);
        ribbon_shader_.setGlow(glow).draw(ribbon_glow_mesh_);
        GL::Renderer::setDepthMask(true);
    }
    GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::Less);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
}

void WaterfallView::update_grid_mesh(float x_min, float x_max, float z_max) {
    grid_vertices_.clear();
    grid_indices_.clear();

    int nx = grid_lines_x;
    int nz = grid_lines_z;
    float cx = (x_min + x_max) * 0.5f;
    float half_w = (x_max - x_min) * 0.5f;

    // Compute left/right edges at depth t using same formula as waveform
    // t: 0 = far/back (z=0), 1 = near/front (z=z_max)
    auto edges_at = [&](float t) -> std::pair<float, float> {
        float c = convergence * (1.0f - t);
        float l = cx - half_w * (1.0f - c);
        float r = cx + half_w * (1.0f - c);
        return {l, r};
    };

    // Horizontal lines (constant Z, spanning L to R)
    for (int zi = 0; zi <= nz; zi++) {
        float t = static_cast<float>(zi) / static_cast<float>(nz);
        float z = t * z_max;
        auto [l, r] = edges_at(t);

        auto base = static_cast<UnsignedInt>(grid_vertices_.size());
        grid_vertices_.push_back(Vector3{l, 0.0f, z});
        grid_vertices_.push_back(Vector3{r, 0.0f, z});
        grid_indices_.push_back(base);
        grid_indices_.push_back(base + 1);
    }

    // Vertical lines — interpolate between L and R edges at each depth
    int z_steps = nz * 2;
    for (int xi = 0; xi <= nx; xi++) {
        float x_frac = static_cast<float>(xi) / static_cast<float>(nx);

        for (int zi = 0; zi < z_steps; zi++) {
            float t0 = static_cast<float>(zi) / static_cast<float>(z_steps);
            float t1 = static_cast<float>(zi + 1) / static_cast<float>(z_steps);

            auto [l0, r0] = edges_at(t0);
            auto [l1, r1] = edges_at(t1);

            float gx0 = l0 + (r0 - l0) * x_frac;
            float gx1 = l1 + (r1 - l1) * x_frac;

            float z0 = t0 * z_max;
            float z1 = t1 * z_max;

            auto base = static_cast<UnsignedInt>(grid_vertices_.size());
            grid_vertices_.push_back(Vector3{gx0, 0.0f, z0});
            grid_vertices_.push_back(Vector3{gx1, 0.0f, z1});
            grid_indices_.push_back(base);
            grid_indices_.push_back(base + 1);
        }
    }

    if (grid_vertices_.empty()) return;

    grid_vertex_buffer_.setData({grid_vertices_.data(), grid_vertices_.size()});
    grid_index_buffer_.setData({grid_indices_.data(), grid_indices_.size()});

    grid_mesh_ = GL::Mesh{};
    grid_mesh_.setPrimitive(GL::MeshPrimitive::Lines)
        .setCount(static_cast<Int>(grid_indices_.size()))
        .addVertexBuffer(grid_vertex_buffer_, 0, vibe::WaterfallShader::Position{})
        .setIndexBuffer(grid_index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
}

void WaterfallView::update_sphere_mesh() {
    sphere_vertices_.clear();
    sphere_indices_.clear();

    constexpr float PI = 3.14159265f;
    constexpr int SEGMENTS = 128;

    // World-to-input scale factors (shader uses gridSize={2,2}):
    float sx = 1.0f / 4.0f;
    float sy = 1.0f / std::max(height_scale, 0.1f);

    // Concentric rings: one per band, innermost = sub, outermost = hat
    float ring_spacing = sphere_base_radius / static_cast<float>(NUM_BANDS);
    float reactive_scale = sphere_base_radius * 4.0f;

    for (int b = 0; b < NUM_BANDS; b++) {
        band_index_offset_[b] = static_cast<int>(sphere_indices_.size());

        float base_r = ring_spacing * static_cast<float>(b + 1);
        float energy = band_energy_[b];
        float r = base_r + energy * reactive_scale;

        auto base_idx = static_cast<UnsignedInt>(sphere_vertices_.size());

        for (int i = 0; i <= SEGMENTS; i++) {
            float theta = 2.0f * PI * static_cast<float>(i % SEGMENTS)
                          / static_cast<float>(SEGMENTS);
            float px = std::cos(theta) * r;
            float py = std::sin(theta) * r;

            sphere_vertices_.push_back(Vector3{
                0.5f + px * sx, py * sy, 0.0f});

            if (i > 0) {
                sphere_indices_.push_back(base_idx + static_cast<UnsignedInt>(i - 1));
                sphere_indices_.push_back(base_idx + static_cast<UnsignedInt>(i));
            }
        }

        band_index_count_[b] = static_cast<int>(sphere_indices_.size()) - band_index_offset_[b];
    }

    if (sphere_vertices_.empty()) return;

    sphere_vertex_buffer_.setData({sphere_vertices_.data(), sphere_vertices_.size()});
    sphere_index_buffer_.setData({sphere_indices_.data(), sphere_indices_.size()});
}

void WaterfallView::update_mesh_waveform() {
    if (waveform_.is_empty()) return;

    auto total_samples = static_cast<int>(waveform_.n_cols);
    int ns = std::min(num_waveform_samples, total_samples);
    if (ns < 2) return;

    // Auto-normalization: find peak across mono + stereo buffers
    if (auto_normalize) {
        running_peak_ *= 0.995f;
        for (int i = 0; i < total_samples; i++) {
            float v = static_cast<float>(std::abs(waveform_(0, i)));
            if (v > running_peak_) running_peak_ = v;
        }
        // Also check stereo channels
        for (arma::uword ch = 0; ch < stereo_waveform_.n_rows; ch++) {
            for (arma::uword i = 0; i < stereo_waveform_.n_cols; i++) {
                float v = static_cast<float>(std::abs(stereo_waveform_(ch, i)));
                if (v > running_peak_) running_peak_ = v;
            }
        }
        if (running_peak_ < 0.001f) running_peak_ = 0.001f;
    }

    float norm = auto_normalize ? (1.0f / running_peak_) : 1.0f;
    int start = total_samples - ns;

    // In position space: 0.5 maps to world x=0 (center) after shader transform
    // With gridSize.x=2: vCol = pos.x, world_x = (pos.x - 0.5) * 4
    // pos.x=0 → world_x=-2, pos.x=0.5 → world_x=0, pos.x=1 → world_x=2
    float cx = 0.5f;

    if (stereo) {
        // Stereo: two lines tracing center (back) → L/R edges (front)
        bool have_stereo = stereo_waveform_.n_rows >= 2 &&
                           stereo_waveform_.n_cols >= static_cast<arma::uword>(ns);
        int stereo_start = have_stereo
            ? static_cast<int>(stereo_waveform_.n_cols) - ns : 0;

        auto uns = static_cast<size_t>(ns);
        vertices_.resize(uns * 2);

        for (int i = 0; i < ns; i++) {
            float amp_l, amp_r;
            if (have_stereo) {
                amp_l = static_cast<float>(stereo_waveform_(0, stereo_start + i)) * norm;
                amp_r = static_cast<float>(stereo_waveform_(1, stereo_start + i)) * norm;
            } else {
                float amp = static_cast<float>(waveform_(0, start + i)) * norm;
                amp_l = amp;
                amp_r = amp;
            }

            float t = static_cast<float>(i) / static_cast<float>(ns - 1);
            float z = static_cast<float>(i);

            // t=0 is far (z=0), t=1 is near (z=ns-1)
            // convergence controls how much far end pinches to center
            float lc = cx * convergence * (1.0f - t);
            float rc = 1.0f - (1.0f - cx) * convergence * (1.0f - t);

            vertices_[static_cast<size_t>(i)]       = Vector3{lc, amp_l, z};
            vertices_[uns + static_cast<size_t>(i)] = Vector3{rc, amp_r, z};
        }

        indices_.clear();
        indices_.reserve(static_cast<size_t>(ns - 1) * 4);
        for (int i = 0; i < ns - 1; i++) {
            auto ui = static_cast<UnsignedInt>(i);
            auto un = static_cast<UnsignedInt>(ns);
            indices_.push_back(ui);
            indices_.push_back(ui + 1);
            indices_.push_back(un + ui);
            indices_.push_back(un + ui + 1);
        }
    } else {
        // Mono: single line tracing center (back) → right edge (front)
        vertices_.resize(static_cast<size_t>(ns));

        for (int i = 0; i < ns; i++) {
            float amp = static_cast<float>(waveform_(0, start + i)) * norm;
            float t = static_cast<float>(i) / static_cast<float>(ns - 1);
            float z = static_cast<float>(i);
            float rc = 1.0f - (1.0f - cx) * convergence * (1.0f - t);
            vertices_[static_cast<size_t>(i)] = Vector3{rc, amp, z};
        }

        indices_.clear();
        indices_.reserve(static_cast<size_t>(ns - 1) * 2);
        for (int i = 0; i < ns - 1; i++) {
            auto ui = static_cast<UnsignedInt>(i);
            indices_.push_back(ui);
            indices_.push_back(ui + 1);
        }
    }

    if (ribbons) {
        // lines run along the depth here, so no fill; one polyline per channel
        ribbon_vertices_.clear();
        ribbon_indices_.clear();
        ribbon_line_indices_.clear();
        const bool two = vertices_.size() >= static_cast<size_t>(ns) * 2;
        add_ribbon(vertices_.data(), ns, false);
        if (two) add_ribbon(vertices_.data() + ns, ns, false);
        ribbon_grid_ = Vector2{2.0f, static_cast<float>(ns)};
        upload_ribbons();
        return;
    }

    vertex_buffer_.setData({vertices_.data(), vertices_.size()});
    index_buffer_.setData({indices_.data(), indices_.size()});

    mesh_ = GL::Mesh{};
    mesh_.setPrimitive(GL::MeshPrimitive::Lines)
        .setCount(static_cast<Int>(indices_.size()))
        .addVertexBuffer(vertex_buffer_, 0, vibe::WaterfallShader::Position{})
        .setIndexBuffer(index_buffer_, 0, GL::MeshIndexType::UnsignedInt);
}

void WaterfallView::draw_terrain(const Matrix4& projection, const Matrix4& view,
                                 const Vector2& grid_size) {
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    bool waveform_lines = use_waveform && !view_2d;

    if (wireframe) {
        GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Line);
    }
    if (wireframe || waveform_lines) {
        glEnable(GL_LINE_SMOOTH);
        GL::Renderer::setLineWidth(waveform_lines ? 2.0f : 1.5f);
    }

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::One);

    // Draw concentric rings at vanishing point — one per band, each colored
    if (show_sphere && use_waveform && !view_2d && !sphere_indices_.empty()) {
        // Band colors: sub=red, kick=orange, mid=green, snare=blue, hat=purple
        static const Color3 band_colors[NUM_BANDS] = {
            {1.0f, 0.15f, 0.15f},  // sub - red
            {1.0f, 0.5f, 0.1f},    // kick - orange
            {0.1f, 1.0f, 0.3f},    // mid - green
            {0.2f, 0.5f, 1.0f},    // snare - blue
            {0.7f, 0.2f, 1.0f},    // hat - purple
        };

        GL::Renderer::setLineWidth(1.5f);
        shader_
            .setGridSize(Vector2{2.0f, 2.0f})
            .setTransformationMatrix(view)
            .setProjectionMatrix(projection)
            .setTime(time_)
            .setHeightScale(height_scale);

        for (int b = 0; b < NUM_BANDS; b++) {
            if (band_index_count_[b] <= 0) continue;
            float e = band_energy_[b];
            Color3 col = band_colors[b] * (0.4f + std::min(e * 2.0f, 0.6f));
            GL::Mesh ring{};
            ring.setPrimitive(GL::MeshPrimitive::Lines)
                .setCount(band_index_count_[b])
                .addVertexBuffer(sphere_vertex_buffer_, 0,
                                 vibe::WaterfallShader::Position{})
                .setIndexBuffer(sphere_index_buffer_,
                                band_index_offset_[b] * sizeof(UnsignedInt),
                                GL::MeshIndexType::UnsignedInt);
            shader_
                .setColorLow(col)
                .setColorMid(col)
                .setColorHigh(col)
                .draw(ring);
        }
    }

    // Draw grid (behind waveform)
    if (show_grid && use_waveform && !view_2d && !grid_indices_.empty()) {
        Color3 grid_color{0.0f, grid_brightness * 0.5f, grid_brightness};
        GL::Renderer::setLineWidth(1.0f);
        shader_
            .setGridSize(grid_size)
            .setTransformationMatrix(view)
            .setProjectionMatrix(projection)
            .setTime(time_)
            .setHeightScale(1.0f)
            .setColorLow(grid_color)
            .setColorMid(grid_color)
            .setColorHigh(grid_color)
            .draw(grid_mesh_);
    }

    // Draw main mesh
    if (waveform_lines && ribbons_ready_) {
        if (wireframe) GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill); // ribbons are triangles
        draw_ribbons(projection, view, ribbon_grid_);
    } else {
        if (waveform_lines) {
            GL::Renderer::setLineWidth(line_width);
        }

        shader_
            .setGridSize(grid_size)
            .setTransformationMatrix(view)
            .setProjectionMatrix(projection)
            .setTime(time_)
            .setHeightScale(height_scale)
            .setColorLow(color_low)
            .setColorMid(color_mid)
            .setColorHigh(color_high)
            .draw(mesh_);
    }

    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    if (wireframe) {
        GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill);
    }
    if (wireframe || waveform_lines) {
        glDisable(GL_LINE_SMOOTH);
        GL::Renderer::setLineWidth(1.0f);
    }

    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
}

void WaterfallView::draw_2d_chart(const Matrix4& projection, const Matrix4& view) {
    // Filled area (solid triangles)
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::One);

    shader_
        .setTransformationMatrix(view)
        .setProjectionMatrix(projection)
        .setTime(time_)
        .setHeightScale(active_height())
        .setColorLow(color_low)
        .setColorMid(color_mid)
        .setColorHigh(color_high)
        .draw(line_mesh_);

    // Wireframe outline on top with line smoothing
    GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Line);
    glEnable(GL_LINE_SMOOTH);
    GL::Renderer::setLineWidth(1.5f);

    shader_.draw(line_mesh_);

    GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill);
    glDisable(GL_LINE_SMOOTH);
    GL::Renderer::setLineWidth(1.0f);

    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void WaterfallView::render_to_texture() {
    ensure_initialized();

    // Match the panel's shape and render it supersampled
    if (display_size_.x() > 0.0f && display_size_.y() > 0.0f) {
        const ImVec2 fb_scale = ImGui::GetIO().DisplayFramebufferScale;
        const int max_size = std::min(GL::Texture2D::maxSize().x(), GL::Renderbuffer::maxSize());
        Vector2i want{
            std::clamp(static_cast<int>(display_size_.x() * fb_scale.x * supersample), 64, max_size),
            std::clamp(static_cast<int>(display_size_.y() * fb_scale.y * supersample), 64, max_size)};
        if (want != target_size_ || msaa_samples != target_samples_) create_targets(want);
    }
    ribbon_viewport_ = Vector2{target_size_};
    ribbon_px_scale_ = supersample * ImGui::GetIO().DisplayFramebufferScale.x;
    ribbons_ready_ = false;

    // Choose mesh update strategy
    if (view_2d) {
        update_mesh_2d();
    } else if (use_waveform && waveform_history) {
        update_mesh_waveform_history();
        if (show_grid)
            update_grid_mesh(0.0f, static_cast<float>(num_waveform_samples - 1),
                             static_cast<float>(waveform_history_rows - 1));
        if (show_sphere) update_sphere_mesh();
    } else if (use_waveform) {
        update_mesh_waveform();
        if (show_grid)
            update_grid_mesh(0.0f, 1.0f, static_cast<float>(num_waveform_samples - 1));
        if (show_sphere) update_sphere_mesh();
    } else {
        update_mesh();
    }

    const Vector2i size = target_size_;
    const float aspect = static_cast<float>(size.x()) / static_cast<float>(size.y());
    Range2Di rect{{}, size};

    msaa_fbo_.bind();
    msaa_fbo_.setViewport(rect);

    GL::Renderer::enable(GL::Renderer::Feature::Multisampling);
    GL::Renderer::setClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    msaa_fbo_.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    if (view_2d) {
        float margin = 0.1f;
        float y_lo = use_waveform ? -height_scale * (1.0f + margin) : -height_scale * margin;
        float y_hi = height_scale * (1.0f + margin);
        Matrix4 projection = Matrix4::orthographicProjection(
            {-2.0f - margin, y_lo},
            { 2.0f + margin, y_hi},
            -10.0f, 10.0f);
        Matrix4 view = Matrix4{};

        shader_.setGridSize(Vector2{
            static_cast<float>(use_waveform ? num_waveform_samples : num_freq_bins), 1.0f});
        draw_2d_chart(projection, view);
    } else if (use_waveform && waveform_history) {
        Matrix4 projection = Matrix4::perspectiveProjection(50.0_degf, aspect, 0.01f, 20.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 0.5f, 1.5f},
            {0.0f, 0.0f, -1.5f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{static_cast<float>(num_waveform_samples),
                   static_cast<float>(waveform_history_rows)};
        draw_terrain(projection, view, gs);
    } else if (use_waveform) {
        Matrix4 projection = Matrix4::perspectiveProjection(50.0_degf, aspect, 0.01f, 20.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 0.5f, 1.5f},
            {0.0f, 0.0f, -1.5f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{2.0f, static_cast<float>(num_waveform_samples)};
        draw_terrain(projection, view, gs);
    } else {
        Matrix4 projection = Matrix4::perspectiveProjection(45.0_degf, aspect, 0.01f, 50.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 2.5f, 1.5f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{static_cast<float>(num_freq_bins),
                   static_cast<float>(history_rows)};
        draw_terrain(projection, view, gs);
    }

    GL::Renderer::disable(GL::Renderer::Feature::Multisampling);

    // Resolve MSAA
    GL::AbstractFramebuffer::blit(msaa_fbo_, fbo_, rect, GL::FramebufferBlit::Color);
    texture_.generateMipmap();

    GL::defaultFramebuffer.bind();
}

void WaterfallView::draw_background(int viewport_width, int viewport_height) {
    ensure_initialized();
    ribbon_viewport_ = Vector2{static_cast<float>(viewport_width), static_cast<float>(viewport_height)};
    ribbon_px_scale_ = ImGui::GetIO().DisplayFramebufferScale.x;
    ribbons_ready_ = false;

    if (view_2d) {
        update_mesh_2d();
    } else if (use_waveform && waveform_history) {
        update_mesh_waveform_history();
        if (show_grid)
            update_grid_mesh(0.0f, static_cast<float>(num_waveform_samples - 1),
                             static_cast<float>(waveform_history_rows - 1));
        if (show_sphere) update_sphere_mesh();
    } else if (use_waveform) {
        update_mesh_waveform();
        if (show_grid)
            update_grid_mesh(0.0f, 1.0f, static_cast<float>(num_waveform_samples - 1));
        if (show_sphere) update_sphere_mesh();
    } else {
        update_mesh();
    }

    GL::defaultFramebuffer.bind();
    GL::defaultFramebuffer.setViewport({{}, {viewport_width, viewport_height}});

    GL::Renderer::setClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    float aspect = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);

    if (view_2d) {
        float margin = 0.1f;
        float y_lo = use_waveform ? -height_scale * (1.0f + margin) : -height_scale * margin;
        float y_hi = height_scale * (1.0f + margin);
        float h = y_hi - y_lo;
        float w = h * aspect;
        float half_w = std::max(2.0f + margin, w * 0.5f);
        Matrix4 projection = Matrix4::orthographicProjection(
            {-half_w, y_lo},
            { half_w, y_hi},
            -10.0f, 10.0f);
        Matrix4 view = Matrix4{};

        shader_.setGridSize(Vector2{
            static_cast<float>(use_waveform ? num_waveform_samples : num_freq_bins), 1.0f});
        draw_2d_chart(projection, view);
    } else if (use_waveform && waveform_history) {
        Matrix4 projection = Matrix4::perspectiveProjection(50.0_degf, aspect, 0.01f, 20.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 0.5f, 1.5f},
            {0.0f, 0.0f, -1.5f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{static_cast<float>(num_waveform_samples),
                   static_cast<float>(waveform_history_rows)};
        draw_terrain(projection, view, gs);
    } else if (use_waveform) {
        Matrix4 projection = Matrix4::perspectiveProjection(50.0_degf, aspect, 0.01f, 20.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 0.5f, 1.5f},
            {0.0f, 0.0f, -1.5f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{2.0f, static_cast<float>(num_waveform_samples)};
        draw_terrain(projection, view, gs);
    } else {
        Matrix4 projection = Matrix4::perspectiveProjection(45.0_degf, aspect, 0.01f, 50.0f);
        Matrix4 view = Matrix4::lookAt(
            {0.0f, 2.5f, 1.5f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f}
        ).invertedRigid();

        Vector2 gs{static_cast<float>(num_freq_bins),
                   static_cast<float>(history_rows)};
        draw_terrain(projection, view, gs);
    }
}

void WaterfallView::draw_window(const char* title, bool* p_open) {
    time_ += ImGui::GetIO().DeltaTime;

    if (!render_to_background) {
        render_to_texture();
    }

    ImGui::SetNextWindowSize(ImVec2(420, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.85f);
    if (ImGui::Begin(title, p_open)) {
        if (!render_to_background) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float side = std::min(avail.x, avail.y - 120.0f);
            if (side < 40.0f) side = 40.0f;
            display_size_ = Vector2{avail.x, side};

            auto tex_id = static_cast<ImTextureID>(texture_.id());
            ImGui::Image(tex_id, ImVec2{avail.x, side}, ImVec2{0, 1}, ImVec2{1, 0});
        } else {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Rendering to background");
        }

        ImGui::Separator();
        ImGui::Checkbox("Background", &render_to_background);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::SameLine();
        ImGui::Checkbox("2D", &view_2d);
        if (use_waveform && !view_2d) {
            ImGui::SameLine();
            ImGui::Checkbox("Stereo", &stereo);
            ImGui::SameLine();
            ImGui::Checkbox("History", &waveform_history);
        }
        ImGui::Checkbox("Auto Normalize", &auto_normalize);
        if (auto_normalize) {
            ImGui::SameLine();
            ImGui::Text("(peak: %.4f)", static_cast<double>(running_peak_));
        }
        if (use_waveform && waveform_history && !view_2d)
            ImGui::SliderFloat("Height", &history_height, 0.02f, 1.0f, "%.2f"); // rows overlap: much lower
        else
            ImGui::SliderFloat("Height", &height_scale, 0.1f, 5.0f, "%.1f");
        if (!render_to_background) {
            ImGui::SliderFloat("Supersample", &supersample, 1.0f, 3.0f, "%.1fx");
        }
        if (use_waveform) {
            ImGui::SliderInt("Samples", &num_waveform_samples, 64, 8192);
            if (!view_2d) {
                ImGui::SliderFloat("Line Width", &line_width, 0.5f, 5.0f, "%.1f");
            }
            if (waveform_history) {
                ImGui::SliderInt("History Rows", &waveform_history_rows, 16, 256);
            }
            if (!view_2d) {
                ImGui::SliderFloat("Convergence", &convergence, 0.0f, 1.0f, "%.2f");
                ImGui::Checkbox("Grid", &show_grid);
                if (show_grid) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80);
                    ImGui::SliderFloat("Brightness##grid", &grid_brightness, 0.01f, 0.5f, "%.2f");
                }
                ImGui::Checkbox("Sphere", &show_sphere);
                if (show_sphere) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::SliderFloat("Radius##sphere", &sphere_base_radius, 0.05f, 1.0f, "%.2f");
                }

                if (ImGui::CollapsingHeader("Look##waveform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("Ribbons", &ribbons);
                    if (ribbons) {
                        if (waveform_history) {
                            ImGui::SameLine();
                            ImGui::Checkbox("Align rows", &trigger);
                            ImGui::SameLine();
                            ImGui::Checkbox("Occlude", &occlude);
                            ImGui::Checkbox("Decimate", &decimate);
                            ImGui::SameLine();
                            ImGui::Checkbox("Taper", &taper);
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(90);
                            ImGui::SliderInt("Smooth", &smooth_radius, 0, 8);
                        }
                        ImGui::SliderFloat("Glow", &glow, 0.0f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Fade", &fade, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Scanlines", &scanlines, 0.0f, 0.5f, "%.2f");
                        ImGui::ColorEdit3("Line##wavecolor", wave_color.data(), ImGuiColorEditFlags_NoInputs);
                        ImGui::SameLine();
                        ImGui::ColorEdit3("Peak##wavecolor", wave_peak_color.data(), ImGuiColorEditFlags_NoInputs);
                    }
                }
            }
        } else {
            ImGui::SliderFloat("HF Boost", &freq_slope, 0.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("Smoothing", &smoothing, 0.0f, 0.99f, "%.2f");
            ImGui::SliderInt("Freq Bins", &num_freq_bins, 16, 128);
            if (!view_2d) {
                ImGui::SliderInt("History", &history_rows, 16, 256);
            }
        }
    }
    ImGui::End();
}

}} // namespace vibe::gfx
