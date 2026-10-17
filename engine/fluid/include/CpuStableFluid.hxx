#pragma once

#include <array>
#include <vector>

#include "vibe/fluid/FluidSolver.hxx"

namespace vibe::fluid {

// Jos Stam's "stable fluids" on a collocated grid with a one-cell wall border:
// splats, vorticity confinement, diffusion, Gauss-Seidel pressure projection,
// semi-Lagrangian advection of velocity and 3-channel dye, fading
class CpuStableFluid : public FluidSolver {
public:
    CpuStableFluid(int width, int height);

    void resize(int width, int height) override;
    int width() const override { return _w; }
    int height() const override { return _h; }
    void step(float dt) override;
    void addForce(Vec2 at, Vec2 velocity, float radius) override;
    void addDye(Vec2 at, Color3 color, float radius) override;
    Vec2 velocityAt(Vec2 at) const override;
    const std::vector<Color3>& dye() const override { return _dyeOut; }
    void clear() override;

    // test and diagnostic hooks; cell (i, j) with i in [0, width), j in [0, height)
    void setCellVelocity(int i, int j, Vec2 cellsPerSecond);
    Vec2 cellVelocity(int i, int j) const;
    float maxAbsDivergence() const;
    float totalAbsCurl() const;
    Color3 totalDye() const;

private:
    struct Force {
        Vec2 at;
        Vec2 velocity;
        float radius;
    };
    struct DyeSplat {
        Vec2 at;
        Color3 color;
        float radius;
    };

    // (i, j) in [0, w+1] x [0, h+1]; the interior is [1, w] x [1, h]
    int idx(int i, int j) const { return i + (_w + 2) * j; }
    void setBoundary(int b, std::vector<float>& x) const;
    void linearSolve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c, int iterations) const;
    void project();
    void advect(int b, std::vector<float>& d, const std::vector<float>& d0, const std::vector<float>& u,
        const std::vector<float>& v, float dt) const;
    void applySplats();
    void confineVorticity(float dt);
    void refreshDyeOut();
    void blendSeam();

    int _w = 0;
    int _h = 0;
    bool _wrapped = false; // wrapX as of the last step, to catch it switching on
    std::vector<float> _u, _v, _u0, _v0, _p, _div, _curl; // velocity in cells per second
    std::array<std::vector<float>, 3> _dye;
    std::array<std::vector<float>, 3> _dye0;
    std::vector<Color3> _dyeOut;
    std::vector<Force> _forces;
    std::vector<DyeSplat> _dyeSplats;
};

} // namespace vibe::fluid
