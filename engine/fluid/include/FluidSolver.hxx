#pragma once

#include <memory>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector2.h>

namespace vibe::fluid {

using Vec2 = Magnum::Vector2;
using Color3 = Magnum::Color3;

struct FluidParams {
    float viscosity = 0.0f;      // velocity diffusion (0 = off)
    float dyeFade = 0.985f;      // dye multiplier per 1/60 s (frame-rate independent)
    float velocityFade = 0.999f; // velocity multiplier per 1/60 s
    float vorticity = 12.0f;     // swirl reinforcement strength (vorticity confinement)
    int pressureIterations = 20; // 0 skips the pressure projection
    bool wrapX = false;          // left and right edges connect (periodic), e.g. to wrap a sphere
};

// 2D incompressible fluid with colored dye on a width x height grid of square cells.
// Positions are normalized: x in [0,1] across the width, y in [0,1] up the height.
// Velocities and radii are in field heights (per second).
class FluidSolver {
public:
    virtual ~FluidSolver() = default;

    virtual void resize(int width, int height) = 0; // clears the field
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void step(float dt) = 0;
    // queued; applied at the start of the next step
    virtual void addForce(Vec2 at, Vec2 velocity, float radius) = 0;
    virtual void addDye(Vec2 at, Color3 color, float radius) = 0;
    virtual Vec2 velocityAt(Vec2 at) const = 0;
    // width*height cells, row-major, row 0 at the bottom
    virtual const std::vector<Color3>& dye() const = 0;
    virtual void clear() = 0;

    FluidParams params;
};

std::unique_ptr<FluidSolver> makeCpuStableFluid(int width, int height);

} // namespace vibe::fluid
