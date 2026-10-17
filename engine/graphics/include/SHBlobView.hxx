// SHBlobView - 2D spherical harmonics blob widget for ImGui
// Renders an equatorial SH cross-section as a radial blob using ImDrawList
#pragma once

#include <array>

namespace vibe { namespace gfx {

class SHBlobView {
public:
    SHBlobView();

    // Feed 5-band audio data each frame
    void set_audio(float sub, float kick, float mid, float snare, float hat);

    // Draw as a standalone ImGui window (pass p_open for close button)
    void draw_window(const char* title = "SH Blob", bool* p_open = nullptr);

    // Draw the blob shape (call inside your own ImGui window)
    void draw_blob();

    // Settings
    float base_radius = 100.0f;
    int num_segments = 128;
    float line_thickness = 2.0f;
    float fill_alpha = 0.25f;

    // Per-band gain multipliers
    std::array<float, 5> band_gains = {2.0f, 3.0f, 2.5f, 4.0f, 3.5f};

private:
    std::array<float, 5> coeffs_ = {};
};

}} // namespace vibe::gfx
