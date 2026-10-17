#include "GridMesh.hxx"

#include <algorithm>

namespace vibe::gfx {

GridMesh gridUvs(int cols, int rows) {
    cols = std::max(1, cols);
    rows = std::max(1, rows);
    GridMesh m;
    m.uvs.reserve(static_cast<std::size_t>((cols + 1) * (rows + 1)));
    for(int j = 0; j <= rows; ++j)
        for(int i = 0; i <= cols; ++i)
            m.uvs.emplace_back(static_cast<float>(i) / static_cast<float>(cols), static_cast<float>(j) / static_cast<float>(rows));
    m.indices.reserve(static_cast<std::size_t>(6 * cols * rows));
    const auto at = [cols](int i, int j) { return static_cast<Magnum::UnsignedInt>(i + (cols + 1) * j); };
    for(int j = 0; j < rows; ++j)
        for(int i = 0; i < cols; ++i)
            for(Magnum::UnsignedInt k : {at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j), at(i + 1, j + 1), at(i, j + 1)})
                m.indices.push_back(k);
    return m;
}

} // namespace vibe::gfx
