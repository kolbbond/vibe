#include "vibe/graphics/Batch2DBuilder.hxx"

#include <cmath>

namespace vibe::gfx {

using Magnum::Color4;
using Magnum::UnsignedInt;
using Magnum::Vector2;

void Batch2DBuilder::clear() {
    _vertices.clear();
    _indices.clear();
}

void Batch2DBuilder::quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, const Color4& color) {
    const auto base = static_cast<UnsignedInt>(_vertices.size());
    _vertices.push_back({p0, color});
    _vertices.push_back({p1, color});
    _vertices.push_back({p2, color});
    _vertices.push_back({p3, color});
    for(const UnsignedInt i : {0u, 1u, 2u, 0u, 2u, 3u}) _indices.push_back(base + i);
}

void Batch2DBuilder::rect(Vector2 min, Vector2 size, const Color4& color) {
    const Vector2 max = min + size;
    quad(min, {max.x(), min.y()}, max, {min.x(), max.y()}, color);
}

void Batch2DBuilder::line(Vector2 a, Vector2 b, float width, const Color4& color) {
    const Vector2 d = b - a;
    const float length = d.length();
    if(length <= 0.0f) return;
    const Vector2 n = Vector2{-d.y(), d.x()} / length * (width * 0.5f); // left of a->b
    quad(a - n, b - n, b + n, a + n, color);
}

void Batch2DBuilder::grid(const Magnum::Range2D& area, Vector2 cell, float width, const Color4& color) {
    if(cell.x() <= 0.0f || cell.y() <= 0.0f) return;
    const int columns = static_cast<int>(std::floor(area.sizeX() / cell.x() + 1e-4f)) + 1;
    const int rows = static_cast<int>(std::floor(area.sizeY() / cell.y() + 1e-4f)) + 1;
    for(int i = 0; i < columns; ++i) {
        const float x = area.min().x() + float(i) * cell.x();
        line({x, area.min().y()}, {x, area.max().y()}, width, color);
    }
    for(int j = 0; j < rows; ++j) {
        const float y = area.min().y() + float(j) * cell.y();
        line({area.min().x(), y}, {area.max().x(), y}, width, color);
    }
}

} // namespace vibe::gfx
