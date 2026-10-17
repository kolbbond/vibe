#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Color.h>

#include "vibe/graphics/Batch2DBuilder.hxx"

using namespace vibe::gfx;
using namespace Magnum::Math::Literals;
using Magnum::Color4;
using Magnum::Range2D;
using Magnum::Vector2;

namespace {
// positive for counter-clockwise triangles
float signedArea(const Batch2DBuilder& b, std::size_t firstIndex) {
    const Vector2 p0 = b.vertices()[b.indices()[firstIndex]].position;
    const Vector2 p1 = b.vertices()[b.indices()[firstIndex + 1]].position;
    const Vector2 p2 = b.vertices()[b.indices()[firstIndex + 2]].position;
    const Vector2 e1 = p1 - p0;
    const Vector2 e2 = p2 - p0;
    return e1.x() * e2.y() - e1.y() * e2.x();
}
} // namespace

struct Batch2DBuilderTest : Corrade::TestSuite::Tester {
    explicit Batch2DBuilderTest();

    void rectIsOneCcwQuad();
    void lineHasWidthAndLength();
    void diagonalLineIsCcw();
    void zeroLengthLineAddsNothing();
    void gridLineCount();
    void clearEmpties();
};

Batch2DBuilderTest::Batch2DBuilderTest() {
    addTests({&Batch2DBuilderTest::rectIsOneCcwQuad,
        &Batch2DBuilderTest::lineHasWidthAndLength,
        &Batch2DBuilderTest::diagonalLineIsCcw,
        &Batch2DBuilderTest::zeroLengthLineAddsNothing,
        &Batch2DBuilderTest::gridLineCount,
        &Batch2DBuilderTest::clearEmpties});
}

void Batch2DBuilderTest::rectIsOneCcwQuad() {
    Batch2DBuilder b;
    const Color4 red = 0xff0000ff_rgbaf;
    b.rect({1.0f, 2.0f}, {3.0f, 4.0f}, red);
    CORRADE_COMPARE(b.vertices().size(), 4u);
    CORRADE_COMPARE(b.indices().size(), 6u);
    CORRADE_COMPARE(b.vertices()[0].position, (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(b.vertices()[2].position, (Vector2{4.0f, 6.0f}));
    for(const Vertex2D& v : b.vertices()) CORRADE_COMPARE(v.color, red);
    CORRADE_VERIFY(signedArea(b, 0) > 0.0f);
    CORRADE_VERIFY(signedArea(b, 3) > 0.0f);
}

void Batch2DBuilderTest::lineHasWidthAndLength() {
    Batch2DBuilder b;
    b.line({0.0f, 0.0f}, {10.0f, 0.0f}, 2.0f, 0xffffffff_rgbaf);
    CORRADE_COMPARE(b.vertices().size(), 4u);
    CORRADE_COMPARE(b.vertices()[0].position, (Vector2{0.0f, -1.0f}));
    CORRADE_COMPARE(b.vertices()[1].position, (Vector2{10.0f, -1.0f}));
    CORRADE_COMPARE(b.vertices()[2].position, (Vector2{10.0f, 1.0f}));
    CORRADE_COMPARE(b.vertices()[3].position, (Vector2{0.0f, 1.0f}));
    CORRADE_VERIFY(signedArea(b, 0) > 0.0f);
}

void Batch2DBuilderTest::diagonalLineIsCcw() {
    Batch2DBuilder b;
    b.line({5.0f, 5.0f}, {-3.0f, 9.0f}, 0.5f, 0xffffffff_rgbaf);
    CORRADE_VERIFY(signedArea(b, 0) > 0.0f);
    CORRADE_VERIFY(signedArea(b, 3) > 0.0f);
}

void Batch2DBuilderTest::zeroLengthLineAddsNothing() {
    Batch2DBuilder b;
    b.line({1.0f, 1.0f}, {1.0f, 1.0f}, 2.0f, 0xffffffff_rgbaf);
    CORRADE_VERIFY(b.vertices().empty());
    CORRADE_VERIFY(b.indices().empty());
}

void Batch2DBuilderTest::gridLineCount() {
    Batch2DBuilder b;
    b.grid(Range2D{{0.0f, 0.0f}, {4.0f, 2.0f}}, {1.0f, 1.0f}, 0.1f, 0xffffffff_rgbaf);
    CORRADE_COMPARE(b.vertices().size(), 8u * 4u); // 5 vertical + 3 horizontal
    b.clear();
    b.grid(Range2D{{0.0f, 0.0f}, {4.0f, 2.0f}}, {0.0f, 1.0f}, 0.1f, 0xffffffff_rgbaf);
    CORRADE_VERIFY(b.vertices().empty()); // degenerate cell size
}

void Batch2DBuilderTest::clearEmpties() {
    Batch2DBuilder b;
    b.rect({0.0f, 0.0f}, {1.0f, 1.0f}, 0xffffffff_rgbaf);
    b.clear();
    CORRADE_VERIFY(b.vertices().empty());
    CORRADE_VERIFY(b.indices().empty());
}

CORRADE_TEST_MAIN(Batch2DBuilderTest)
