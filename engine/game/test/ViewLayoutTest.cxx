#include <cmath>

#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Range.h>

#include "vibe/game/ViewLayout.hxx"

using namespace vibe::game;
using Magnum::Range2D;
using Magnum::Range2Di;
using Magnum::Vector2;
using Magnum::Vector2i;

namespace {
bool overlaps(const Range2Di& a, const Range2Di& b) {
    return a.min().x() < b.max().x() && b.min().x() < a.max().x() && a.min().y() < b.max().y() && b.min().y() < a.max().y();
}
} // namespace

struct ViewLayoutTest : Corrade::TestSuite::Tester {
    explicit ViewLayoutTest();

    void sharedIsOneFullView();
    void splitOnePlayerIsFull();
    void splitTwoLandscapeSideBySide();
    void splitTwoPortraitStacked();
    void splitFourIsGridFromTopLeft();
    void splitThreeHasEmptyCell();
    void oddSizeRemainderGoesLast();
    void viewsNeverOverlapAndStayInside();
    void zeroSizeGivesNoViews();
    void viewToWindowFlipsAndScales();
};

ViewLayoutTest::ViewLayoutTest() {
    addTests({&ViewLayoutTest::sharedIsOneFullView,
        &ViewLayoutTest::splitOnePlayerIsFull,
        &ViewLayoutTest::splitTwoLandscapeSideBySide,
        &ViewLayoutTest::splitTwoPortraitStacked,
        &ViewLayoutTest::splitFourIsGridFromTopLeft,
        &ViewLayoutTest::splitThreeHasEmptyCell,
        &ViewLayoutTest::oddSizeRemainderGoesLast,
        &ViewLayoutTest::viewsNeverOverlapAndStayInside,
        &ViewLayoutTest::zeroSizeGivesNoViews,
        &ViewLayoutTest::viewToWindowFlipsAndScales});
}

void ViewLayoutTest::sharedIsOneFullView() {
    const auto views = layoutViews({1280, 720}, 3, LayoutMode::Shared, 4);
    CORRADE_COMPARE(views.size(), 1u);
    CORRADE_COMPARE(views[0].viewport, (Range2Di{{0, 0}, {1280, 720}}));
    CORRADE_COMPARE(views[0].player, -1);
    CORRADE_VERIFY(std::abs(views[0].aspect - 1280.0f / 720.0f) < 1e-6f);
}

void ViewLayoutTest::splitOnePlayerIsFull() {
    const auto views = layoutViews({800, 600}, 1, LayoutMode::Split, 4);
    CORRADE_COMPARE(views.size(), 1u);
    CORRADE_COMPARE(views[0].viewport, (Range2Di{{0, 0}, {800, 600}}));
    CORRADE_COMPARE(views[0].player, 0);
}

void ViewLayoutTest::splitTwoLandscapeSideBySide() {
    const auto views = layoutViews({1280, 720}, 2, LayoutMode::Split, 4);
    CORRADE_COMPARE(views.size(), 2u);
    CORRADE_COMPARE(views[0].viewport, (Range2Di{{0, 0}, {638, 720}}));
    CORRADE_COMPARE(views[1].viewport, (Range2Di{{642, 0}, {1280, 720}}));
    CORRADE_COMPARE(views[0].player, 0);
    CORRADE_COMPARE(views[1].player, 1);
}

void ViewLayoutTest::splitTwoPortraitStacked() {
    const auto views = layoutViews({720, 1280}, 2, LayoutMode::Split, 4);
    CORRADE_COMPARE(views.size(), 2u);
    // player 0 on top: GL y grows upward
    CORRADE_COMPARE(views[0].viewport, (Range2Di{{0, 642}, {720, 1280}}));
    CORRADE_COMPARE(views[1].viewport, (Range2Di{{0, 0}, {720, 638}}));
}

void ViewLayoutTest::splitFourIsGridFromTopLeft() {
    const auto views = layoutViews({1280, 720}, 4, LayoutMode::Split, 0);
    CORRADE_COMPARE(views.size(), 4u);
    CORRADE_COMPARE(views[0].viewport, (Range2Di{{0, 360}, {640, 720}}));
    CORRADE_COMPARE(views[1].viewport, (Range2Di{{640, 360}, {1280, 720}}));
    CORRADE_COMPARE(views[2].viewport, (Range2Di{{0, 0}, {640, 360}}));
    CORRADE_COMPARE(views[3].viewport, (Range2Di{{640, 0}, {1280, 360}}));
    for(int i = 0; i < 4; ++i) CORRADE_COMPARE(views[i].player, i);
}

void ViewLayoutTest::splitThreeHasEmptyCell() {
    const auto views = layoutViews({1280, 720}, 3, LayoutMode::Split, 4);
    CORRADE_COMPARE(views.size(), 4u);
    CORRADE_COMPARE(views[2].player, 2);
    CORRADE_COMPARE(views[3].player, -1);
}

void ViewLayoutTest::oddSizeRemainderGoesLast() {
    const auto views = layoutViews({1001, 500}, 2, LayoutMode::Split, 0);
    CORRADE_COMPARE(views[0].viewport.sizeX(), 500);
    CORRADE_COMPARE(views[1].viewport.sizeX(), 501);
    CORRADE_COMPARE(views[1].viewport.max().x(), 1001);
}

void ViewLayoutTest::viewsNeverOverlapAndStayInside() {
    const Vector2i sizes[] = {{1280, 720}, {720, 1280}, {1001, 777}};
    for(const Vector2i fb : sizes)
        for(const LayoutMode mode : {LayoutMode::Shared, LayoutMode::Split})
            for(int players = 0; players <= 4; ++players) {
                const auto views = layoutViews(fb, players, mode, 4);
                CORRADE_VERIFY(!views.empty());
                for(std::size_t a = 0; a < views.size(); ++a) {
                    const Range2Di& r = views[a].viewport;
                    CORRADE_VERIFY(r.min().x() >= 0 && r.min().y() >= 0);
                    CORRADE_VERIFY(r.max().x() <= fb.x() && r.max().y() <= fb.y());
                    CORRADE_VERIFY(r.sizeX() > 0 && r.sizeY() > 0);
                    for(std::size_t b = a + 1; b < views.size(); ++b) CORRADE_VERIFY(!overlaps(r, views[b].viewport));
                }
            }
}

void ViewLayoutTest::zeroSizeGivesNoViews() {
    CORRADE_VERIFY(layoutViews({0, 720}, 2, LayoutMode::Split, 4).empty());
    CORRADE_VERIFY(layoutViews({1280, 0}, 1, LayoutMode::Shared, 0).empty());
}

void ViewLayoutTest::viewToWindowFlipsAndScales() {
    // HiDPI: framebuffer is twice the window size; top-left quadrant in GL coordinates
    const Range2D r = viewToWindow(Range2Di{{0, 720}, {1280, 1440}}, {2560, 1440}, {1280.0f, 720.0f});
    CORRADE_COMPARE(r.min(), (Vector2{0.0f, 0.0f}));
    CORRADE_COMPARE(r.max(), (Vector2{640.0f, 360.0f}));
    const Range2D bottomRight = viewToWindow(Range2Di{{1280, 0}, {2560, 720}}, {2560, 1440}, {1280.0f, 720.0f});
    CORRADE_COMPARE(bottomRight.min(), (Vector2{640.0f, 360.0f}));
    CORRADE_COMPARE(bottomRight.max(), (Vector2{1280.0f, 720.0f}));
}

CORRADE_TEST_MAIN(ViewLayoutTest)
