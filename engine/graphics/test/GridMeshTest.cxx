#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Vector2.h>

#include "vibe/graphics/GridMesh.hxx"

using namespace vibe::gfx;
using Magnum::Vector2;

struct GridMeshTest : Corrade::TestSuite::Tester {
    explicit GridMeshTest();

    void counts();
    void cornersSpanUnitSquare();
    void indicesInRange();
    void trianglesCounterClockwise();
};

GridMeshTest::GridMeshTest() {
    addTests({&GridMeshTest::counts, &GridMeshTest::cornersSpanUnitSquare, &GridMeshTest::indicesInRange,
        &GridMeshTest::trianglesCounterClockwise});
}

void GridMeshTest::counts() {
    const GridMesh m = gridUvs(4, 3);
    CORRADE_COMPARE(m.uvs.size(), 20u);
    CORRADE_COMPARE(m.indices.size(), 72u);
}

void GridMeshTest::cornersSpanUnitSquare() {
    const GridMesh m = gridUvs(4, 3);
    CORRADE_COMPARE(m.uvs.front(), (Vector2{0.0f, 0.0f}));
    CORRADE_COMPARE(m.uvs.back(), (Vector2{1.0f, 1.0f}));
    CORRADE_COMPARE(m.uvs[4], (Vector2{1.0f, 0.0f}));
}

void GridMeshTest::indicesInRange() {
    const GridMesh m = gridUvs(7, 5);
    for(Magnum::UnsignedInt i : m.indices) CORRADE_VERIFY(i < m.uvs.size());
}

void GridMeshTest::trianglesCounterClockwise() {
    const GridMesh m = gridUvs(3, 2);
    for(std::size_t t = 0; t < m.indices.size(); t += 3) {
        const Vector2 a = m.uvs[m.indices[t]], b = m.uvs[m.indices[t + 1]], c = m.uvs[m.indices[t + 2]];
        const float cross = (b - a).x() * (c - a).y() - (b - a).y() * (c - a).x();
        CORRADE_VERIFY(cross > 0.0f);
    }
}

CORRADE_TEST_MAIN(GridMeshTest)
