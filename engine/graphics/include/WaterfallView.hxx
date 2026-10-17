// WaterfallView - Scrolling 3D/2D waveform waterfall rendered via OpenGL
#pragma once

#include <vector>
#include <armadillo>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

#include "WaterfallRibbonShader.hxx"
#include "WaterfallShader.hxx"
#include "typedefs.hxx"

namespace vibe { namespace gfx {

class WaterfallView {
public:
    WaterfallView();

    // Feed FFT data each frame
    void set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags);

    // Feed raw waveform for time-domain waterfall (mono or stereo)
    void set_waveform(const arma::Mat<fltp>& mono_buffer, int sample_rate);

    // Feed stereo waveform (2 rows: left/right channels)
    void set_stereo_waveform(const arma::Mat<fltp>& stereo_buffer, int sample_rate);

    // Feed audio energy per band for concentric ring reactivity (0-1 range)
    void set_audio_energy(float sub, float kick, float mid, float snare, float hat);

    // Draw as standalone ImGui window (renders to FBO, shows as Image)
    void draw_window(const char* title = "Waterfall", bool* p_open = nullptr);

    // Draw to default framebuffer (background mode)
    void draw_background(int viewport_width, int viewport_height);

    // Settings
    int num_freq_bins = 64;
    int history_rows = 96;
    int num_waveform_samples = 2048;  // samples to show in waveform ribbon
    float height_scale = 0.6f;
    float history_height = 0.18f;     // waveform history rows: low enough that ridges overlap a few rows
    float freq_slope = 3.0f;
    float smoothing = 0.4f;
    float line_width = 2.0f;          // GL line width for waveform lines
    bool use_waveform = false;
    bool wireframe = true;
    bool render_to_background = false;
    bool view_2d = false;             // 2D line chart mode vs 3D terrain
    bool stereo = true;               // stereo: L/R channels split left/right
    bool waveform_history = false;    // scrolling history of past waveforms
    int waveform_history_rows = 64;   // number of past waveform snapshots
    float convergence = 0.9f;         // 0=parallel rows, 1=vanishing point
    bool show_grid = false;           // perspective grid behind waveform
    int grid_lines_x = 12;           // vertical grid lines
    int grid_lines_z = 16;           // horizontal grid lines
    float grid_brightness = 0.15f;   // grid color intensity
    bool show_sphere = false;         // wireframe sphere at vanishing point
    float sphere_base_radius = 0.3f; // sphere radius in world units
    bool auto_normalize = true;       // auto-scale magnitudes to 0-1
    // waveform look (3D waveform modes)
    bool ribbons = true;              // real-width anti-aliased lines (off: plain GL lines)
    bool trigger = true;              // history: start each row on a rising zero crossing
    bool decimate = true;             // history: ~1 point per output pixel (box-filtered)
    int smooth_radius = 1;            // history: extra moving average, in points
    bool taper = true;                // history: fade each row's ends to zero
    bool occlude = true;              // history: black fill under each row hides the rows behind
    float glow = 0.2f;                // halo strength around ribbons
    float fade = 0.7f;                // distance fade toward the oldest rows
    float scanlines = 0.0f;           // old scanline shimmer, 0 = off
    Magnum::Color3 wave_color{0.0f, 0.6f, 0.8f};       // line base color
    Magnum::Color3 wave_peak_color{0.75f, 0.95f, 1.0f}; // color at full amplitude

    int texture_size = 2048;          // initial render size, before the panel size is known
    int msaa_samples = 16;            // clamped to the GPU's max
    float supersample = 2.0f;         // render at panel size x this, downsampled via mipmaps

    // Colors
    Magnum::Color3 color_low{0.0f, 0.05f, 0.15f};   // dark blue (silence)
    Magnum::Color3 color_mid{0.0f, 0.6f, 0.8f};     // cyan (moderate)
    Magnum::Color3 color_high{1.0f, 0.3f, 0.8f};    // hot pink (loud)

private:
    void push_fft_row();
    void push_waveform_row();
    void ensure_initialized();
    void update_mesh();
    void update_mesh_2d();
    void update_mesh_waveform();
    void update_mesh_waveform_history();
    void update_grid_mesh(float x_min, float x_max, float z_max);
    void update_sphere_mesh();
    void draw_terrain(const Magnum::Matrix4& projection, const Magnum::Matrix4& view,
                      const Magnum::Vector2& grid_size);
    void draw_2d_chart(const Magnum::Matrix4& projection, const Magnum::Matrix4& view);
    void render_to_texture();
    void create_targets(const Magnum::Vector2i& size);

    // ribbons
    int find_trigger_delay(int window) const;
    void process_row(const float* src, int n, int out_n, std::vector<float>& out, bool taper_head, bool taper_tail) const;
    void add_ribbon(const Magnum::Vector3* pts, int n, bool fill);
    void upload_ribbons();
    void draw_ribbons(const Magnum::Matrix4& projection, const Magnum::Matrix4& view, const Magnum::Vector2& grid_size);
    int ribbon_points() const; // decimated points per history row
    float active_height() const { return use_waveform && waveform_history ? history_height : height_scale; }

    // FFT input
    arma::Row<fltp> freqs_;
    arma::Mat<fltp> mags_;
    arma::Mat<fltp> waveform_;
    arma::Mat<fltp> stereo_waveform_;  // 2 rows: L/R channels
    int sample_rate_ = 44100;

    // Ring buffer: history_rows x num_freq_bins
    std::vector<float> ring_buffer_;
    int ring_idx_ = 0;
    bool buffer_filled_ = false;

    // Log frequency bin edges
    std::vector<double> bin_edges_;
    bool bins_dirty_ = true;
    int last_num_bins_ = 0;
    int last_num_rows_ = 0;

    // Waveform history ring buffer
    std::vector<float> waveform_ring_;
    int waveform_ring_idx_ = 0;
    bool waveform_ring_filled_ = false;
    int last_waveform_ns_ = 0;
    int last_waveform_history_rows_ = 0;

    // Auto-normalization
    float running_peak_ = 0.001f;     // avoid div-by-zero

    // GL resources
    bool initialized_ = false;
    float time_ = 0.0f;
    vibe::WaterfallShader shader_{Magnum::NoCreate};
    Magnum::GL::Mesh mesh_{Magnum::NoCreate};
    Magnum::GL::Buffer vertex_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer index_buffer_{Magnum::NoCreate};

    // 2D mode: line mesh (current row as line strip + filled area)
    Magnum::GL::Mesh line_mesh_{Magnum::NoCreate};
    Magnum::GL::Buffer line_vertex_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer line_index_buffer_{Magnum::NoCreate};

    // Grid mesh
    Magnum::GL::Mesh grid_mesh_{Magnum::NoCreate};
    Magnum::GL::Buffer grid_vertex_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer grid_index_buffer_{Magnum::NoCreate};
    std::vector<Magnum::Vector3> grid_vertices_;
    std::vector<Magnum::UnsignedInt> grid_indices_;

    // Concentric ring meshes (one per band)
    static constexpr int NUM_BANDS = 5;
    float band_energy_[NUM_BANDS] = {}; // sub, kick, mid, snare, hat
    Magnum::GL::Buffer sphere_vertex_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer sphere_index_buffer_{Magnum::NoCreate};
    std::vector<Magnum::Vector3> sphere_vertices_;
    std::vector<Magnum::UnsignedInt> sphere_indices_;
    int band_index_offset_[NUM_BANDS] = {};  // byte offset into index buffer
    int band_index_count_[NUM_BANDS] = {};   // number of indices per band

    // Ribbon geometry: fills and lines in back-to-front order; glow reuses the line quads
    vibe::WaterfallRibbonShader ribbon_shader_{Magnum::NoCreate};
    Magnum::GL::Buffer ribbon_vertex_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer ribbon_index_buffer_{Magnum::NoCreate};
    Magnum::GL::Buffer ribbon_line_index_buffer_{Magnum::NoCreate};
    Magnum::GL::Mesh ribbon_mesh_{Magnum::NoCreate};
    Magnum::GL::Mesh ribbon_glow_mesh_{Magnum::NoCreate};
    std::vector<vibe::WaterfallRibbonShader::Vertex> ribbon_vertices_;
    std::vector<Magnum::UnsignedInt> ribbon_indices_;
    std::vector<Magnum::UnsignedInt> ribbon_line_indices_;
    Magnum::Vector2 ribbon_grid_{2.0f, 2.0f};      // grid size the ribbon points are in
    Magnum::Vector2 ribbon_viewport_{1024.0f, 1024.0f}; // px of the target being drawn
    float ribbon_px_scale_ = 1.0f;                 // supersample factor of that target
    bool ribbons_ready_ = false;                   // ribbon geometry built this frame

    // FBO for texture rendering
    Magnum::Vector2i target_size_{0, 0};   // current render target size
    Magnum::Vector2 display_size_{0.0f, 0.0f}; // panel size last frame (ImGui px)
    int target_samples_ = 0;
    Magnum::GL::Texture2D texture_{Magnum::NoCreate};
    Magnum::GL::Renderbuffer depth_{Magnum::NoCreate};
    Magnum::GL::Framebuffer fbo_{Magnum::NoCreate};

    // MSAA
    Magnum::GL::Renderbuffer msaa_color_{Magnum::NoCreate};
    Magnum::GL::Renderbuffer msaa_depth_{Magnum::NoCreate};
    Magnum::GL::Framebuffer msaa_fbo_{Magnum::NoCreate};

    // Vertex data (updated each frame)
    std::vector<Magnum::Vector3> vertices_;
    std::vector<Magnum::UnsignedInt> indices_;
    bool mesh_dirty_ = true;
};

}} // namespace vibe::gfx
