// SHBlob3DView - 3D SH blob rendered to FBO, displayed as ImGui::Image
#include "SHBlob3DView.hxx"

#include <algorithm>
#include <cmath>
#include <imgui.h>

#include <Corrade/Containers/ArrayView.h>

#include <Magnum/GL/AbstractFramebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Matrix4.h>
#include <MagnumExternal/OpenGL/GL/flextGL.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace vibe { namespace gfx {

SHBlob3DView::SHBlob3DView() {}

void SHBlob3DView::set_audio(float sub, float kick, float mid, float snare, float hat) {
    sub_ = sub;
    kick_ = kick;
    mid_ = mid;
    snare_ = snare;
    hat_ = hat;
}

void SHBlob3DView::set_fft_data(const arma::Row<fltp>& freqs, const arma::Mat<fltp>& mags) {
    if (freqs.is_empty() || mags.is_empty()) return;

    auto nb = static_cast<size_t>(num_fft_bins);
    nb = std::min(nb, static_cast<size_t>(vibe::SHBlobShader::MaxFFTBins));

    // Rebuild log-spaced bin edges if bin count changed
    if (last_num_bins_ != static_cast<int>(nb)) {
        bin_edges_.resize(nb + 1);
        double fmin = 20.0, fmax = 20000.0;
        for (size_t i = 0; i <= nb; i++) {
            bin_edges_[i] = fmin * std::pow(fmax / fmin, static_cast<double>(i) / static_cast<double>(nb));
        }
        fft_bins_.resize(nb, 0.0f);
        last_num_bins_ = static_cast<int>(nb);
    }

    // Aggregate FFT mags into log-spaced bins (peak per bin)
    for (size_t b = 0; b < nb; b++) {
        double lo = bin_edges_[b];
        double hi = bin_edges_[b + 1];
        float peak = 0.0f;

        for (arma::uword j = 0; j < freqs.n_elem && j < mags.n_cols; j++) {
            double f = freqs(j);
            if (f >= lo && f < hi) {
                float m = static_cast<float>(mags(0, j));
                if (m > peak) peak = m;
            }
        }

        // Log compress for better dynamic range
        float compressed = std::log1p(peak * gain * 10.0f) / std::log1p(10.0f);

        // Fast attack, slow decay
        if (compressed > fft_bins_[b]) {
            fft_bins_[b] = compressed;
        } else {
            fft_bins_[b] = 0.85f * fft_bins_[b] + 0.15f * compressed;
        }
    }
}

void SHBlob3DView::set_bpm(float bpm) {
    bpm_ = bpm;
}

void SHBlob3DView::ensure_initialized() {
    if (initialized_) return;

    shader_ = vibe::SHBlobShader{};
    mesh_ = MeshTools::compile(Primitives::icosphereSolid(4));

    Vector2i size{texture_size, texture_size};

    // Resolve target: regular texture + depth for ImGui::Image display
    texture_ = GL::Texture2D{};
    texture_.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGBA8, size);

    depth_ = GL::Renderbuffer{};
    depth_.setStorage(GL::RenderbufferFormat::DepthComponent24, size);

    fbo_ = GL::Framebuffer{{{}, size}};
    fbo_.attachTexture(GL::Framebuffer::ColorAttachment{0}, texture_, 0)
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, depth_);

    // MSAA render target: 8x multisample renderbuffers
    msaa_color_ = GL::Renderbuffer{};
    msaa_color_.setStorageMultisample(8, GL::RenderbufferFormat::RGBA8, size);

    msaa_depth_ = GL::Renderbuffer{};
    msaa_depth_.setStorageMultisample(8, GL::RenderbufferFormat::DepthComponent24, size);

    msaa_fbo_ = GL::Framebuffer{{{}, size}};
    msaa_fbo_.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, msaa_color_)
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, msaa_depth_);

    initialized_ = true;
}

void SHBlob3DView::draw_blob(const Matrix4& projection, const Matrix4& view) {
    // Y-axis rotation (BPM sync or manual speed)
    constexpr float TWO_PI = 6.28318530718f;
    float effective_speed = rotation_speed;
    if (bpm_sync && bpm_ > 0.0f && beats_per_rotation > 0.0f) {
        effective_speed = TWO_PI * bpm_ / (60.0f * beats_per_rotation);
    }
    float rotAngle = time_ * effective_speed;
    Matrix4 model = Matrix4::rotationY(Rad{rotAngle});
    Matrix4 transformation = view * model;

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    if (wireframe) {
        GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Line);
        glEnable(GL_LINE_SMOOTH);
        GL::Renderer::setLineWidth(1.5f);
    }

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::One);

    // Log-compress 5-band values for fallback/color
    auto compress = [](float v, float g) -> float {
        float scaled = v * g;
        return std::log1p(scaled * 10.0f) / std::log1p(10.0f);
    };

    shader_
        .setTransformationMatrix(transformation)
        .setProjectionMatrix(projection)
        .setNormalMatrix(transformation.normalMatrix())
        .setTime(time_)
        .setSnapResolution(snap_resolution)
        .setBaseColor(base_color)
        .setRimColor(rim_color)
        .setRimPower(rim_power)
        .setSub(compress(sub_, gain))
        .setKick(compress(kick_, gain))
        .setMid(compress(mid_, gain))
        .setSnare(compress(snare_, gain))
        .setHat(compress(hat_, gain));

    // Pass FFT bin data for radial displacement
    if (!fft_bins_.empty()) {
        shader_.setNumBins(static_cast<int>(fft_bins_.size()));
        shader_.setFFTBins({fft_bins_.data(), fft_bins_.size()});
    } else {
        shader_.setNumBins(0);
    }

    shader_.draw(mesh_);

    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    if (wireframe) {
        GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill);
        glDisable(GL_LINE_SMOOTH);
        GL::Renderer::setLineWidth(1.0f);
    }

    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
}

void SHBlob3DView::render_to_texture() {
    ensure_initialized();

    Vector2i size{texture_size, texture_size};
    Range2Di rect{{}, size};

    // Render to MSAA FBO
    msaa_fbo_.bind();
    msaa_fbo_.setViewport(rect);

    GL::Renderer::enable(GL::Renderer::Feature::Multisampling);
    GL::Renderer::setClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    msaa_fbo_.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    Matrix4 projection = Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 50.0f);
    Matrix4 view = Matrix4::lookAt({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, 0.0f},
                                    {0.0f, 1.0f, 0.0f}).invertedRigid();

    draw_blob(projection, view);

    GL::Renderer::disable(GL::Renderer::Feature::Multisampling);

    // Resolve MSAA → texture
    GL::AbstractFramebuffer::blit(msaa_fbo_, fbo_, rect, GL::FramebufferBlit::Color);

    // Restore default framebuffer
    GL::defaultFramebuffer.bind();
}

void SHBlob3DView::draw_background(int viewport_width, int viewport_height) {
    ensure_initialized();
    // time_ is advanced in draw_window()

    GL::defaultFramebuffer.bind();
    GL::defaultFramebuffer.setViewport({{}, {viewport_width, viewport_height}});

    // Pitch black background
    GL::Renderer::setClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    float aspect = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);
    Matrix4 projection = Matrix4::perspectiveProjection(35.0_degf, aspect, 0.01f, 50.0f);
    Matrix4 view = Matrix4::lookAt({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, 0.0f},
                                    {0.0f, 1.0f, 0.0f}).invertedRigid();

    draw_blob(projection, view);
}

void SHBlob3DView::draw_window(const char* title, bool* p_open) {
    time_ += ImGui::GetIO().DeltaTime;

    if (!render_to_background) {
        render_to_texture();
    }

    ImGui::SetNextWindowSize(ImVec2(300, 380), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin(title, p_open)) {
        if (!render_to_background) {
            // Display texture as ImGui::Image (Y-flip for GL→ImGui)
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float side = avail.x < avail.y ? avail.x : avail.y;
            if (side < 40.0f) side = 40.0f;
            ImVec2 img_size{side, side};

            auto tex_id = static_cast<ImTextureID>(texture_.id());
            ImGui::Image(tex_id, img_size, ImVec2{0, 1}, ImVec2{1, 0});
        } else {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Rendering to background");
        }

        // Settings
        ImGui::Separator();
        ImGui::Checkbox("Background", &render_to_background);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::SliderFloat("Gain", &gain, 1.0f, 100.0f, "%.0f");
        ImGui::Checkbox("BPM Sync", &bpm_sync);
        if (bpm_sync) {
            ImGui::SameLine();
            ImGui::Text("%.0f BPM", static_cast<double>(bpm_));
            ImGui::SliderFloat("Beats/Rot", &beats_per_rotation, 1.0f, 16.0f, "%.0f");
        } else {
            ImGui::SliderFloat("Rotation", &rotation_speed, 0.0f, 2.0f);
        }
        ImGui::SliderFloat("Snap", &snap_resolution, 0.0f, 320.0f);
        ImGui::SliderFloat("Rim Power", &rim_power, 0.5f, 8.0f);
    }
    ImGui::End();
}

}} // namespace vibe::gfx
