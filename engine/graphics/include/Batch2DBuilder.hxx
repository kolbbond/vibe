#pragma once

#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector2.h>

namespace vibe::gfx {

struct Vertex2D {
    Magnum::Vector2 position;
    Magnum::Color4 color;
};

// CPU side of Batch2D: builds indexed, counter-clockwise triangles (no GL)
class Batch2DBuilder {
public:
    void clear();
    void rect(Magnum::Vector2 min, Magnum::Vector2 size, const Magnum::Color4& color);
    // a quad of `width` centered on the segment; zero-length segments add nothing
    void line(Magnum::Vector2 a, Magnum::Vector2 b, float width, const Magnum::Color4& color);
    // lines every `cell` across `area`, including both edges
    void grid(const Magnum::Range2D& area, Magnum::Vector2 cell, float width, const Magnum::Color4& color);

    const std::vector<Vertex2D>& vertices() const { return _vertices; }
    const std::vector<Magnum::UnsignedInt>& indices() const { return _indices; }

private:
    void quad(Magnum::Vector2 p0, Magnum::Vector2 p1, Magnum::Vector2 p2, Magnum::Vector2 p3, const Magnum::Color4& color);

    std::vector<Vertex2D> _vertices;
    std::vector<Magnum::UnsignedInt> _indices;
};

} // namespace vibe::gfx
