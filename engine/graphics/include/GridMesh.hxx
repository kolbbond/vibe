#pragma once

#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

namespace vibe::gfx {

// a cols x rows grid over the unit square: (cols+1)*(rows+1) uvs (row-major, v up)
// and 6*cols*rows counter-clockwise triangle indices. Shaders turn uvs into surfaces.
struct GridMesh {
    std::vector<Magnum::Vector2> uvs;
    std::vector<Magnum::UnsignedInt> indices;
};

GridMesh gridUvs(int cols, int rows);

} // namespace vibe::gfx
