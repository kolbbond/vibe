#include "vibe/game/Hud.hxx"

#include <imgui.h>

namespace vibe::game {

using Magnum::Color4;
using Magnum::Vector2;

namespace {
ImU32 toImGui(const Color4& c) { return ImGui::GetColorU32(ImVec4{c.r(), c.g(), c.b(), c.a()}); }
} // namespace

void Hud::setFrame(Magnum::Vector2i framebufferSize, Vector2 windowSize) {
    _framebuffer = framebufferSize;
    _window = windowSize;
}

Magnum::Range2D Hud::windowRect(const View& view) const { return viewToWindow(view.viewport, _framebuffer, _window); }

Vector2 Hud::viewSize(const View& view) const { return windowRect(view).size(); }

void Hud::text(const View& view, Vector2 posInView, const Color4& color, std::string_view str, float scale) {
    const Magnum::Range2D r = windowRect(view);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect({r.min().x(), r.min().y()}, {r.max().x(), r.max().y()}, true);
    const Vector2 p = r.min() + posInView;
    list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, {p.x(), p.y()}, toImGui(color), str.data(), str.data() + str.size());
    list->PopClipRect();
}

void Hud::textCentered(const View& view, Vector2 centerInView, const Color4& color, std::string_view str, float scale) {
    const ImVec2 size = ImGui::CalcTextSize(str.data(), str.data() + str.size());
    text(view, centerInView - Vector2{size.x, size.y} * scale * 0.5f, color, str, scale);
}

void Hud::panel(const View& view, Vector2 minInView, Vector2 size, const Color4& color) {
    const Magnum::Range2D r = windowRect(view);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect({r.min().x(), r.min().y()}, {r.max().x(), r.max().y()}, true);
    const Vector2 a = r.min() + minInView;
    const Vector2 b = a + size;
    list->AddRectFilled({a.x(), a.y()}, {b.x(), b.y()}, toImGui(color));
    list->PopClipRect();
}

} // namespace vibe::game
