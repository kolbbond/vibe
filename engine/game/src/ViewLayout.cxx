#include "vibe/game/ViewLayout.hxx"

#include <algorithm>

namespace vibe::game {

namespace {

struct Span {
    int offset;
    int size;
};

// split `total` into `n` spans separated by `gutter`; the last span takes the remainder
std::vector<Span> spans(int total, int n, int gutter) {
    const int available = std::max(0, total - gutter * (n - 1));
    const int base = available / n;
    const int remainder = available % n;
    std::vector<Span> out;
    int offset = 0;
    for(int i = 0; i < n; ++i) {
        const int size = base + (i == n - 1 ? remainder : 0);
        out.push_back({offset, size});
        offset += size + gutter;
    }
    return out;
}

View makeView(const Magnum::Range2Di& viewport, int player) {
    const float aspect = viewport.sizeY() > 0 ? float(viewport.sizeX()) / float(viewport.sizeY()) : 1.0f;
    return View{viewport, player, aspect};
}

} // namespace

std::vector<View> layoutViews(Magnum::Vector2i framebuffer, int playerCount, LayoutMode mode, int gutterPx) {
    if(framebuffer.x() <= 0 || framebuffer.y() <= 0) return {};
    const int count = std::clamp(playerCount, 0, 4);
    const Magnum::Range2Di full{{}, framebuffer};

    if(mode == LayoutMode::Shared || count <= 1)
        return {makeView(full, mode == LayoutMode::Split && count == 1 ? 0 : -1)};

    int cols = 2;
    int rows = 2;
    if(count == 2) {
        const bool landscape = framebuffer.x() >= framebuffer.y();
        cols = landscape ? 2 : 1;
        rows = landscape ? 1 : 2;
    }
    const int gutter = std::max(0, gutterPx);
    const std::vector<Span> xs = spans(framebuffer.x(), cols, gutter);
    const std::vector<Span> ys = spans(framebuffer.y(), rows, gutter); // top-down

    std::vector<View> views;
    for(int r = 0; r < rows; ++r)
        for(int c = 0; c < cols; ++c) {
            const int cell = r * cols + c;
            const int glY = framebuffer.y() - ys[r].offset - ys[r].size;
            const auto viewport = Magnum::Range2Di::fromSize({xs[c].offset, glY}, {xs[c].size, ys[r].size});
            views.push_back(makeView(viewport, cell < count ? cell : -1));
        }
    return views;
}

Magnum::Range2D viewToWindow(const Magnum::Range2Di& viewport, Magnum::Vector2i framebuffer, Magnum::Vector2 windowSize) {
    const Magnum::Vector2 scale = windowSize / Magnum::Vector2{framebuffer};
    const Magnum::Vector2 min{float(viewport.min().x()) * scale.x(), float(framebuffer.y() - viewport.max().y()) * scale.y()};
    const Magnum::Vector2 max{float(viewport.max().x()) * scale.x(), float(framebuffer.y() - viewport.min().y()) * scale.y()};
    return {min, max};
}

} // namespace vibe::game
