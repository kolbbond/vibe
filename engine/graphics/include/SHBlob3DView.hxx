// SHBlob3DView - 3D spherical harmonics blob rendered to FBO, displayed as ImGui::Image
#pragma once

#include <vector>
#include <armadillo>

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

#include "SHBlobShader.hxx"
#include "typedefs.hxx"

namespace vibe { namespace gfx {

class SHBlob3DView {
public:
    SHBlob3DView();

    // Feed 5-band audio data each frame (for color/fallback)
    void set_audio(float sub, float kick, float mid, float snare, float hat);

    // Feed raw FFT data for radial displacement on sphere surface
    void set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags);

    // Feed estimated BPM from onset detection
    void set_bpm(float bpm);

    // FFT settings
    int num_fft_bins = 64;       // number of log-spaced bins on sphere

    // Draw as a standalone ImGui window (renders to FBO then shows as Image)
    void draw_window(const char* title = "SH Blob 3D", bool* p_open = nullptr);

    // Draw directly to default framebuffer (background mode)
    void draw_background(int viewport_width, int viewport_height);

    // Settings
    bool render_to_background = false;  // false = FBO→ImGui::Image, true = default framebuffer
    bool wireframe = true;
    float gain = 30.0f;               // amplify audio bands for visible displacement
    float rotation_speed = 0.3f;
    float snap_resolution = 160.0f;
    float rim_power = 2.0f;
    Magnum::Color3 base_color{0.0f, 0.8f, 0.9f};
    Magnum::Color3 rim_color{0.6f, 0.2f, 1.0f};
    int texture_size = 1024;

    // BPM sync
    bool bpm_sync = false;
    float beats_per_rotation = 4.0f;  // one full rotation per bar (4/4 time)

private:
    void ensure_initialized();
    void render_to_texture();
    void draw_blob(const Magnum::Matrix4& projection, const Magnum::Matrix4& view);

    bool initialized_ = false;
    float time_ = 0.0f;

    // Audio bands (smoothed externally)
    float sub_ = 0.0f, kick_ = 0.0f, mid_ = 0.0f, snare_ = 0.0f, hat_ = 0.0f;

    // FFT bins for radial displacement
    std::vector<float> fft_bins_;
    std::vector<double> bin_edges_;
    int last_num_bins_ = 0;

    // BPM
    float bpm_ = 0.0f;

    // GL resources
    vibe::SHBlobShader shader_{Magnum::NoCreate};
    Magnum::GL::Mesh mesh_{Magnum::NoCreate};
    Magnum::GL::Texture2D texture_{Magnum::NoCreate};
    Magnum::GL::Renderbuffer depth_{Magnum::NoCreate};
    Magnum::GL::Framebuffer fbo_{Magnum::NoCreate};

    // MSAA resolve resources
    Magnum::GL::Renderbuffer msaa_color_{Magnum::NoCreate};
    Magnum::GL::Renderbuffer msaa_depth_{Magnum::NoCreate};
    Magnum::GL::Framebuffer msaa_fbo_{Magnum::NoCreate};
};

}} // namespace vibe::gfx
