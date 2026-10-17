// SHBlobView - 2D SH blob visualization via ImDrawList
#include "SHBlobView.hxx"
#include "SphericalHarmonics.hxx"

#include <cmath>
#include <vector>
#include <imgui.h>

namespace vibe { namespace gfx {

SHBlobView::SHBlobView() {}

void SHBlobView::set_audio(float sub, float kick, float mid, float snare, float hat) {
    coeffs_[0] = sub * band_gains[0];
    coeffs_[1] = kick * band_gains[1];
    coeffs_[2] = mid * band_gains[2];
    coeffs_[3] = snare * band_gains[3];
    coeffs_[4] = hat * band_gains[4];
}

void SHBlobView::draw_window(const char* title, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin(title, p_open)) {
        draw_blob();
        ImGui::Separator();
        ImGui::SliderFloat("Radius", &base_radius, 20.0f, 200.0f);
        ImGui::SliderFloat("Fill Alpha", &fill_alpha, 0.0f, 1.0f);
        ImGui::SliderFloat("Line", &line_thickness, 0.5f, 5.0f);
        if (ImGui::TreeNode("Band Gains")) {
            ImGui::SliderFloat("Sub",   &band_gains[0], 0.0f, 10.0f);
            ImGui::SliderFloat("Kick",  &band_gains[1], 0.0f, 10.0f);
            ImGui::SliderFloat("Mid",   &band_gains[2], 0.0f, 10.0f);
            ImGui::SliderFloat("Snare", &band_gains[3], 0.0f, 10.0f);
            ImGui::SliderFloat("Hat",   &band_gains[4], 0.0f, 10.0f);
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void SHBlobView::draw_blob() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float side = (avail.x < avail.y ? avail.x : avail.y);
    if (side < 40.0f) return;

    // Use smaller of base_radius or half the available space
    float draw_radius = base_radius;
    if (draw_radius > side * 0.45f) draw_radius = side * 0.45f;

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 center(cursor.x + side * 0.5f, cursor.y + side * 0.5f);

    // Reserve space
    ImGui::Dummy(ImVec2(side, side));

    constexpr float PI = 3.14159265358979f;
    constexpr float THETA = PI / 2.0f; // equatorial slice

    // Compute points on the blob (equatorial cross-section)
    std::vector<ImVec2> points(static_cast<size_t>(num_segments));
    for (int i = 0; i < num_segments; i++) {
        float phi = 2.0f * PI * static_cast<float>(i) / static_cast<float>(num_segments);
        float r = sh::sh_audio_eval(THETA, phi, coeffs_);
        if (r < 0.1f) r = 0.1f; // clamp to avoid collapse
        float px = center.x + r * draw_radius * std::cos(phi);
        float py = center.y - r * draw_radius * std::sin(phi); // flip Y for screen coords
        points[static_cast<size_t>(i)] = ImVec2(px, py);
    }

    // Fill using fan triangulation from center (concave-safe)
    ImU32 fill_col = IM_COL32(0, 200, 220, static_cast<int>(fill_alpha * 255.0f));
    for (int i = 0; i < num_segments; i++) {
        int next = (i + 1) % num_segments;
        dl->AddTriangleFilled(center, points[static_cast<size_t>(i)],
                              points[static_cast<size_t>(next)], fill_col);
    }

    // Outline stroke
    ImU32 line_col = IM_COL32(0, 255, 120, 220);
    for (int i = 0; i < num_segments; i++) {
        int next = (i + 1) % num_segments;
        dl->AddLine(points[static_cast<size_t>(i)],
                    points[static_cast<size_t>(next)], line_col, line_thickness);
    }

    // Center dot
    dl->AddCircleFilled(center, 2.0f, IM_COL32(0, 255, 120, 180));
}

}} // namespace vibe::gfx
