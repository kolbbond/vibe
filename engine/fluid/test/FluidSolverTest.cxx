#include <limits>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/Debug.h>

#include "vibe/fluid/CpuStableFluid.hxx"

using namespace vibe::fluid;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// dye-weighted x centroid, in cells
float dyeCentroidX(const CpuStableFluid& f) {
    double sum = 0.0;
    double weight = 0.0;
    const std::vector<Color3>& dye = f.dye();
    for(int j = 0; j < f.height(); ++j)
        for(int i = 0; i < f.width(); ++i) {
            const Color3& c = dye[static_cast<std::size_t>(i + j * f.width())];
            const double w = c.r() + c.g() + c.b();
            sum += w * (i + 0.5);
            weight += w;
        }
    return weight > 0.0 ? static_cast<float>(sum / weight) : 0.0f;
}

float rand01(std::uint64_t& s) {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<float>((s >> 33) % 1000000u) / 1000000.0f;
}

void randomForces(CpuStableFluid& f, std::uint64_t seed, int count) {
    for(int k = 0; k < count; ++k) {
        const float x = 0.2f + 0.6f * rand01(seed);
        const float y = 0.2f + 0.6f * rand01(seed);
        const float vx = rand01(seed) * 2.0f - 1.0f;
        const float vy = rand01(seed) * 2.0f - 1.0f;
        f.addForce({x, y}, {vx, vy}, 0.06f);
    }
}

bool allFinite(const CpuStableFluid& f) {
    for(const Color3& c : f.dye())
        if(!std::isfinite(c.r()) || !std::isfinite(c.g()) || !std::isfinite(c.b())) return false;
    for(int j = 0; j < f.height(); ++j)
        for(int i = 0; i < f.width(); ++i) {
            const Vec2 v = f.cellVelocity(i, j);
            if(!std::isfinite(v.x()) || !std::isfinite(v.y())) return false;
        }
    return true;
}

// dye mass in columns [i0, i1)
float dyeInColumns(const CpuStableFluid& f, int i0, int i1) {
    double sum = 0.0;
    for(int j = 0; j < f.height(); ++j)
        for(int i = i0; i < i1; ++i) {
            const Color3& c = f.dye()[static_cast<std::size_t>(i + j * f.width())];
            sum += c.r() + c.g() + c.b();
        }
    return static_cast<float>(sum);
}

} // namespace

struct FluidSolverTest : Corrade::TestSuite::Tester {
    explicit FluidSolverTest();

    void dyeMovesDownstream();
    void projectionRemovesDivergence();
    void wallsStopNormalFlow();
    void dyeFadesAtConfiguredRate();
    void vorticityConfinementKeepsSwirl();
    void velocityAtIsBilinear();
    void sameInputsSameResult();
    void resizeClears();
    void hugeForceStaysFinite();
    void nonFiniteInputIgnored();
    void wrapCarriesDyeAcrossTheSeam();
    void wallStopsDyeWithoutWrap();
    void splatAtEdgeWrapsAround();
    void wrapProjectionRemovesDivergence();
    void wrapKeepsTopBottomWalls();
    void wrapToggleMidRunStaysFinite();
    void wrapToggleLeavesNoSeam();
    void timingReport();
};

FluidSolverTest::FluidSolverTest() {
    addTests({&FluidSolverTest::dyeMovesDownstream,
        &FluidSolverTest::projectionRemovesDivergence,
        &FluidSolverTest::wallsStopNormalFlow,
        &FluidSolverTest::dyeFadesAtConfiguredRate,
        &FluidSolverTest::vorticityConfinementKeepsSwirl,
        &FluidSolverTest::velocityAtIsBilinear,
        &FluidSolverTest::sameInputsSameResult,
        &FluidSolverTest::resizeClears,
        &FluidSolverTest::hugeForceStaysFinite,
        &FluidSolverTest::nonFiniteInputIgnored,
        &FluidSolverTest::wrapCarriesDyeAcrossTheSeam,
        &FluidSolverTest::wallStopsDyeWithoutWrap,
        &FluidSolverTest::splatAtEdgeWrapsAround,
        &FluidSolverTest::wrapProjectionRemovesDivergence,
        &FluidSolverTest::wrapKeepsTopBottomWalls,
        &FluidSolverTest::wrapToggleMidRunStaysFinite,
        &FluidSolverTest::wrapToggleLeavesNoSeam,
        &FluidSolverTest::timingReport});
}

void FluidSolverTest::dyeMovesDownstream() {
    CpuStableFluid f{64, 32};
    f.params.pressureIterations = 0; // pure advection
    f.params.vorticity = 0.0f;
    f.params.dyeFade = 1.0f;
    f.params.velocityFade = 1.0f;
    for(int j = 0; j < 32; ++j)
        for(int i = 0; i < 64; ++i) f.setCellVelocity(i, j, {20.0f, 0.0f}); // cells per second
    f.addDye({0.25f, 0.5f}, Color3{1.0f}, 0.1f);
    f.step(kDt);
    const float before = dyeCentroidX(f);
    for(int s = 0; s < 30; ++s) f.step(kDt);
    const float moved = dyeCentroidX(f) - before; // 20 cells/s for 0.5 s = 10 cells
    CORRADE_VERIFY(moved > 8.0f);
    CORRADE_VERIFY(moved < 12.0f);
}

void FluidSolverTest::projectionRemovesDivergence() {
    CpuStableFluid unprojected{32, 32};
    CpuStableFluid projected{32, 32};
    for(CpuStableFluid* f : {&unprojected, &projected}) {
        f->params.vorticity = 0.0f;
        randomForces(*f, 7, 20);
    }
    unprojected.params.pressureIterations = 0;
    projected.params.pressureIterations = 200;
    unprojected.step(kDt);
    projected.step(kDt);
    CORRADE_VERIFY(unprojected.maxAbsDivergence() > 1.0f);
    CORRADE_VERIFY(projected.maxAbsDivergence() < 0.2f * unprojected.maxAbsDivergence());
}

void FluidSolverTest::wallsStopNormalFlow() {
    CpuStableFluid f{32, 32};
    f.addForce({0.05f, 0.5f}, {-2.0f, 0.0f}, 0.05f); // into the left wall
    f.addForce({0.5f, 0.95f}, {0.0f, 2.0f}, 0.05f);  // into the top wall
    for(int s = 0; s < 5; ++s) f.step(kDt);
    CORRADE_VERIFY(std::abs(f.velocityAt({0.0f, 0.5f}).x()) < 1e-5f);
    CORRADE_VERIFY(std::abs(f.velocityAt({1.0f, 0.5f}).x()) < 1e-5f);
    CORRADE_VERIFY(std::abs(f.velocityAt({0.5f, 0.0f}).y()) < 1e-5f);
    CORRADE_VERIFY(std::abs(f.velocityAt({0.5f, 1.0f}).y()) < 1e-5f);
}

void FluidSolverTest::dyeFadesAtConfiguredRate() {
    CpuStableFluid f{32, 32};
    f.params.dyeFade = 0.9f;
    f.addDye({0.5f, 0.5f}, Color3{1.0f}, 0.1f);
    f.step(kDt);
    const float first = f.totalDye().r();
    for(int s = 0; s < 59; ++s) f.step(kDt);
    const float expected = first * std::pow(0.9f, 59.0f);
    CORRADE_VERIFY(std::abs(f.totalDye().r() - expected) < 0.01f * expected);
}

void FluidSolverTest::vorticityConfinementKeepsSwirl() {
    CpuStableFluid plain{48, 48};
    CpuStableFluid confined{48, 48};
    plain.params.vorticity = 0.0f;
    confined.params.vorticity = 30.0f;
    for(CpuStableFluid* f : {&plain, &confined}) {
        f->params.velocityFade = 1.0f;
        for(int k = 0; k < 8; ++k) {
            const float a = 6.2831853f * static_cast<float>(k) / 8.0f;
            f->addForce({0.5f + 0.15f * std::cos(a), 0.5f + 0.15f * std::sin(a)}, {-std::sin(a), std::cos(a)}, 0.06f);
        }
    }
    for(int s = 0; s < 30; ++s) {
        plain.step(kDt);
        confined.step(kDt);
    }
    CORRADE_VERIFY(confined.totalAbsCurl() > plain.totalAbsCurl());
}

void FluidSolverTest::velocityAtIsBilinear() {
    CpuStableFluid f{16, 16};
    f.setCellVelocity(4, 4, {16.0f, 0.0f}); // 16 cells/s = 1 field height/s
    CORRADE_COMPARE(f.velocityAt({4.5f / 16.0f, 4.5f / 16.0f}).x(), 1.0f); // cell center
    CORRADE_COMPARE(f.velocityAt({5.0f / 16.0f, 4.5f / 16.0f}).x(), 0.5f); // halfway to cell 5
}

void FluidSolverTest::sameInputsSameResult() {
    CpuStableFluid a{40, 24};
    CpuStableFluid b{40, 24};
    for(CpuStableFluid* f : {&a, &b}) {
        randomForces(*f, 3, 10);
        f->addDye({0.5f, 0.5f}, Color3{1.0f, 0.5f, 0.2f}, 0.1f);
    }
    for(int s = 0; s < 20; ++s) {
        a.step(kDt);
        b.step(kDt);
    }
    CORRADE_COMPARE(a.dye().size(), b.dye().size());
    CORRADE_VERIFY(std::memcmp(a.dye().data(), b.dye().data(), a.dye().size() * sizeof(Color3)) == 0);
}

void FluidSolverTest::resizeClears() {
    CpuStableFluid f{32, 32};
    f.addDye({0.5f, 0.5f}, Color3{1.0f}, 0.1f);
    f.step(kDt);
    CORRADE_VERIFY(f.totalDye().r() > 0.0f);
    f.resize(20, 10);
    CORRADE_COMPARE(f.width(), 20);
    CORRADE_COMPARE(f.height(), 10);
    CORRADE_COMPARE(f.dye().size(), 200u);
    CORRADE_COMPARE(f.totalDye().r(), 0.0f);
    const auto made = makeCpuStableFluid(24, 12);
    CORRADE_COMPARE(made->width(), 24);
    CORRADE_COMPARE(made->height(), 12);
}

void FluidSolverTest::hugeForceStaysFinite() {
    CpuStableFluid f{64, 36};
    f.addForce({0.5f, 0.5f}, {1.0e4f, -1.0e4f}, 0.2f);
    f.addDye({0.5f, 0.5f}, Color3{1.0e3f}, 0.2f);
    for(int s = 0; s < 120; ++s) f.step(kDt);
    CORRADE_VERIFY(allFinite(f));
}

void FluidSolverTest::wrapCarriesDyeAcrossTheSeam() {
    CpuStableFluid f{64, 32};
    f.params.wrapX = true;
    f.params.vorticity = 0.0f;
    f.params.dyeFade = 1.0f;
    f.params.velocityFade = 1.0f;
    for(int j = 0; j < 32; ++j)
        for(int i = 0; i < 64; ++i) f.setCellVelocity(i, j, {30.0f, 0.0f});
    f.addDye({0.9f, 0.5f}, Color3{1.0f}, 0.05f);
    for(int s = 0; s < 60; ++s) f.step(kDt); // 30 cells: from x~57 past the seam to x~23
    CORRADE_VERIFY(dyeInColumns(f, 0, 32) > 4.0f * dyeInColumns(f, 32, 64));
}

void FluidSolverTest::wallStopsDyeWithoutWrap() {
    CpuStableFluid f{64, 32};
    f.params.vorticity = 0.0f;
    f.params.dyeFade = 1.0f;
    for(int j = 0; j < 32; ++j)
        for(int i = 0; i < 64; ++i) f.setCellVelocity(i, j, {30.0f, 0.0f});
    f.addDye({0.9f, 0.5f}, Color3{1.0f}, 0.05f);
    for(int s = 0; s < 60; ++s) f.step(kDt);
    CORRADE_VERIFY(dyeInColumns(f, 0, 16) < 0.01f * dyeInColumns(f, 0, 64));
}

void FluidSolverTest::splatAtEdgeWrapsAround() {
    CpuStableFluid f{64, 32};
    f.params.wrapX = true;
    f.addDye({0.0f, 0.5f}, Color3{1.0f}, 0.05f); // centered on the seam
    f.step(kDt);
    const float left = dyeInColumns(f, 0, 4);
    const float right = dyeInColumns(f, 60, 64);
    CORRADE_VERIFY(left > 0.0f);
    CORRADE_VERIFY(std::abs(left - right) < 0.1f * left);
}

void FluidSolverTest::wrapProjectionRemovesDivergence() {
    CpuStableFluid f{32, 32};
    f.params.wrapX = true;
    f.params.vorticity = 0.0f;
    f.params.pressureIterations = 200;
    CpuStableFluid unprojected{32, 32};
    unprojected.params.wrapX = true;
    unprojected.params.vorticity = 0.0f;
    unprojected.params.pressureIterations = 0;
    for(CpuStableFluid* g : {&f, &unprojected}) {
        g->addForce({0.02f, 0.5f}, {2.0f, 0.5f}, 0.06f); // across the seam
        randomForces(*g, 11, 20);
    }
    f.step(kDt);
    unprojected.step(kDt);
    CORRADE_VERIFY(f.maxAbsDivergence() < 0.2f * unprojected.maxAbsDivergence());
}

void FluidSolverTest::wrapKeepsTopBottomWalls() {
    CpuStableFluid f{32, 32};
    f.params.wrapX = true;
    f.addForce({0.5f, 0.95f}, {0.0f, 2.0f}, 0.05f);
    f.addForce({0.5f, 0.05f}, {0.0f, -2.0f}, 0.05f);
    for(int s = 0; s < 5; ++s) f.step(kDt);
    CORRADE_VERIFY(std::abs(f.velocityAt({0.5f, 0.0f}).y()) < 1e-5f);
    CORRADE_VERIFY(std::abs(f.velocityAt({0.5f, 1.0f}).y()) < 1e-5f);
}

void FluidSolverTest::wrapToggleMidRunStaysFinite() {
    CpuStableFluid f{33, 20}; // odd width on purpose
    randomForces(f, 3, 30);
    for(int s = 0; s < 20; ++s) f.step(kDt);
    f.params.wrapX = true;
    randomForces(f, 4, 30);
    for(int s = 0; s < 20; ++s) f.step(kDt);
    CORRADE_VERIFY(allFinite(f));
}

// switching to wrap with dye against one wall: the two edges become neighbours, and the
// field must blend across them instead of showing a hard line (the planet's meridian seam)
void FluidSolverTest::wrapToggleLeavesNoSeam() {
    CpuStableFluid f{64, 32};
    f.params.dyeFade = 1.0f;
    f.addDye({0.02f, 0.5f}, Color3{1.0f}, 0.1f); // left wall only
    for(int s = 0; s < 5; ++s) f.step(kDt);
    f.params.wrapX = true;
    f.step(kDt);
    const float left = dyeInColumns(f, 0, 1);
    const float right = dyeInColumns(f, 63, 64);
    CORRADE_VERIFY(left > 0.0f);
    CORRADE_VERIFY(std::abs(left - right) < 0.2f * std::max(left, right));
}

void FluidSolverTest::timingReport() {
    CpuStableFluid f{192, 108};
    randomForces(f, 11, 30);
    const auto start = std::chrono::steady_clock::now();
    for(int s = 0; s < 120; ++s) f.step(kDt);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 120.0;
    Corrade::Utility::Debug{} << "CpuStableFluid 192x108:" << ms << "ms/step (target <= 3)";
    CORRADE_VERIFY(ms > 0.0); // informational only
}

// review finding: a non-finite force (e.g. an inf gain) must be dropped, not index out of bounds
void FluidSolverTest::nonFiniteInputIgnored() {
    CpuStableFluid f{32, 18};
    const float inf = std::numeric_limits<float>::infinity();
    f.addForce({0.5f, 0.5f}, {inf, 0.0f}, 0.1f);
    f.addForce({0.5f, 0.5f}, {std::nanf(""), 1.0f}, 0.1f);
    f.addDye({0.5f, 0.5f}, Color3{inf, 0.0f, 0.0f}, 0.1f);
    f.addForce({inf, 0.5f}, {1.0f, 0.0f}, 0.1f);
    for(int s = 0; s < 10; ++s) f.step(1.0f / 60.0f);
    for(const Color3& c : f.dye()) CORRADE_VERIFY(std::isfinite(c.r()) && std::isfinite(c.g()) && std::isfinite(c.b()));
    const Vec2 v = f.velocityAt({0.5f, 0.5f});
    CORRADE_VERIFY(std::isfinite(v.x()) && std::isfinite(v.y()));
}

CORRADE_TEST_MAIN(FluidSolverTest)
