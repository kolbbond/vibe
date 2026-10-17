#include "vibe/fluid/CpuStableFluid.hxx"

#include <algorithm>
#include <cmath>

namespace vibe::fluid {

std::unique_ptr<FluidSolver> makeCpuStableFluid(int width, int height) {
    return std::make_unique<CpuStableFluid>(width, height);
}

CpuStableFluid::CpuStableFluid(int width, int height) { resize(width, height); }

void CpuStableFluid::resize(int width, int height) {
    _w = std::max(4, width);
    _h = std::max(4, height);
    const std::size_t n = static_cast<std::size_t>(_w + 2) * static_cast<std::size_t>(_h + 2);
    for(std::vector<float>* field : {&_u, &_v, &_u0, &_v0, &_p, &_div, &_curl}) field->assign(n, 0.0f);
    for(int c = 0; c < 3; ++c) {
        _dye[c].assign(n, 0.0f);
        _dye0[c].assign(n, 0.0f);
    }
    _dyeOut.assign(static_cast<std::size_t>(_w) * static_cast<std::size_t>(_h), Color3{0.0f});
    _forces.clear();
    _dyeSplats.clear();
}

void CpuStableFluid::clear() { resize(_w, _h); }

namespace {
// splat input guard: non-finite values would turn into out-of-range cell indices, and
// absurd magnitudes (a runaway gain) would saturate the field; drop or clamp them
constexpr float kMaxSplatVelocity = 100.0f; // field heights per second
constexpr float kMaxSplatDye = 100.0f;
bool finite(Vec2 v) { return std::isfinite(v.x()) && std::isfinite(v.y()); }
bool finite(Color3 c) { return std::isfinite(c.r()) && std::isfinite(c.g()) && std::isfinite(c.b()); }
} // namespace

void CpuStableFluid::addForce(Vec2 at, Vec2 velocity, float radius) {
    if(!finite(at) || !finite(velocity) || !std::isfinite(radius) || radius < 0.0f) return;
    const float speed = velocity.length();
    if(speed > kMaxSplatVelocity) velocity *= kMaxSplatVelocity / speed;
    _forces.push_back({at, velocity, std::min(radius, 1.0f)});
}

void CpuStableFluid::addDye(Vec2 at, Color3 color, float radius) {
    if(!finite(at) || !finite(color) || !std::isfinite(radius) || radius < 0.0f) return;
    for(int c = 0; c < 3; ++c) color[c] = std::clamp(color[c], 0.0f, kMaxSplatDye);
    _dyeSplats.push_back({at, color, std::min(radius, 1.0f)});
}

// b = 0: scalar (copy), 1: x-velocity (negated at the left/right walls), 2: y-velocity (bottom/top)
void CpuStableFluid::setBoundary(int b, std::vector<float>& x) const {
    for(int j = 1; j <= _h; ++j) {
        if(params.wrapX) { // periodic: each ghost column mirrors the opposite edge, no negation
            x[idx(0, j)] = x[idx(_w, j)];
            x[idx(_w + 1, j)] = x[idx(1, j)];
        } else {
            x[idx(0, j)] = b == 1 ? -x[idx(1, j)] : x[idx(1, j)];
            x[idx(_w + 1, j)] = b == 1 ? -x[idx(_w, j)] : x[idx(_w, j)];
        }
    }
    for(int i = 1; i <= _w; ++i) {
        x[idx(i, 0)] = b == 2 ? -x[idx(i, 1)] : x[idx(i, 1)];
        x[idx(i, _h + 1)] = b == 2 ? -x[idx(i, _h)] : x[idx(i, _h)];
    }
    x[idx(0, 0)] = 0.5f * (x[idx(1, 0)] + x[idx(0, 1)]);
    x[idx(0, _h + 1)] = 0.5f * (x[idx(1, _h + 1)] + x[idx(0, _h)]);
    x[idx(_w + 1, 0)] = 0.5f * (x[idx(_w, 0)] + x[idx(_w + 1, 1)]);
    x[idx(_w + 1, _h + 1)] = 0.5f * (x[idx(_w, _h + 1)] + x[idx(_w + 1, _h)]);
}

// Gauss-Seidel: x = (x0 + a * neighbours) / c
void CpuStableFluid::linearSolve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c, int iterations) const {
    // red-black Gauss-Seidel: cells of one checkerboard colour only read the other colour,
    // so a pass has no cell-to-cell dependency chain (plain Gauss-Seidel waited on the left
    // neighbour every cell, which dominated the step)
    const float invC = 1.0f / c;
    for(int k = 0; k < iterations; ++k) {
        for(int colour = 0; colour < 2; ++colour)
            for(int j = 1; j <= _h; ++j)
                for(int i = 1 + ((j + colour) & 1); i <= _w; i += 2)
                    x[idx(i, j)] = (x0[idx(i, j)] + a * (x[idx(i - 1, j)] + x[idx(i + 1, j)] + x[idx(i, j - 1)] + x[idx(i, j + 1)])) * invC;
        setBoundary(b, x);
    }
}

void CpuStableFluid::project() {
    if(params.pressureIterations <= 0) return;
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i) {
            _div[idx(i, j)] = -0.5f * (_u[idx(i + 1, j)] - _u[idx(i - 1, j)] + _v[idx(i, j + 1)] - _v[idx(i, j - 1)]);
            _p[idx(i, j)] = 0.0f;
        }
    setBoundary(0, _div);
    setBoundary(0, _p);
    linearSolve(0, _p, _div, 1.0f, 4.0f, params.pressureIterations);
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i) {
            _u[idx(i, j)] -= 0.5f * (_p[idx(i + 1, j)] - _p[idx(i - 1, j)]);
            _v[idx(i, j)] -= 0.5f * (_p[idx(i, j + 1)] - _p[idx(i, j - 1)]);
        }
    setBoundary(1, _u);
    setBoundary(2, _v);
}

void CpuStableFluid::advect(int b, std::vector<float>& d, const std::vector<float>& d0, const std::vector<float>& u,
    const std::vector<float>& v, float dt) const {
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i) {
            float x = static_cast<float>(i) - dt * u[idx(i, j)];
            if(params.wrapX) {
                // into [0.5, w + 0.5): the ghost columns 0 and w+1 hold the wrapped neighbours
                const float w = static_cast<float>(_w);
                x = x - 0.5f - w * std::floor((x - 0.5f) / w) + 0.5f;
            } else {
                x = std::clamp(x, 0.5f, static_cast<float>(_w) + 0.5f);
            }
            const float y = std::clamp(static_cast<float>(j) - dt * v[idx(i, j)], 0.5f, static_cast<float>(_h) + 0.5f);
            const int i0 = static_cast<int>(x);
            const int j0 = static_cast<int>(y);
            const float s1 = x - static_cast<float>(i0);
            const float t1 = y - static_cast<float>(j0);
            const float s0 = 1.0f - s1;
            const float t0 = 1.0f - t1;
            d[idx(i, j)] = s0 * (t0 * d0[idx(i0, j0)] + t1 * d0[idx(i0, j0 + 1)]) + s1 * (t0 * d0[idx(i0 + 1, j0)] + t1 * d0[idx(i0 + 1, j0 + 1)]);
        }
    setBoundary(b, d);
}

void CpuStableFluid::applySplats() {
    const float cellsPerHeight = static_cast<float>(_h);
    // Gaussian splat around `at` (normalized), radius in field heights
    const auto each = [&](Vec2 at, float radius, auto&& apply) {
        const float cx = at.x() * static_cast<float>(_w) + 0.5f;
        const float cy = at.y() * static_cast<float>(_h) + 0.5f;
        const float r = std::max(0.5f, radius * cellsPerHeight);
        // with wrap the x range runs past the edges and each column folds back onto the field
        const int i0 = params.wrapX ? static_cast<int>(std::floor(cx - 3.0f * r)) : std::max(1, static_cast<int>(std::floor(cx - 3.0f * r)));
        const int i1 = params.wrapX ? static_cast<int>(std::ceil(cx + 3.0f * r)) : std::min(_w, static_cast<int>(std::ceil(cx + 3.0f * r)));
        const int j0 = std::max(1, static_cast<int>(std::floor(cy - 3.0f * r)));
        const int j1 = std::min(_h, static_cast<int>(std::ceil(cy + 3.0f * r)));
        for(int j = j0; j <= j1; ++j)
            for(int i = i0; i <= i1; ++i) {
                const float dx = static_cast<float>(i) - cx;
                const float dy = static_cast<float>(j) - cy;
                const float g = std::exp(-(dx * dx + dy * dy) / (r * r));
                const int cell = params.wrapX ? 1 + (((i - 1) % _w) + _w) % _w : i;
                if(g > 1e-4f) apply(idx(cell, j), g);
            }
    };
    for(const Force& f : _forces)
        each(f.at, f.radius, [&](int k, float g) {
            _u[k] += f.velocity.x() * cellsPerHeight * g;
            _v[k] += f.velocity.y() * cellsPerHeight * g;
        });
    for(const DyeSplat& s : _dyeSplats)
        each(s.at, s.radius, [&](int k, float g) {
            for(int c = 0; c < 3; ++c) _dye[c][k] += s.color[c] * g;
        });
    _forces.clear();
    _dyeSplats.clear();
    setBoundary(1, _u);
    setBoundary(2, _v);
}

void CpuStableFluid::confineVorticity(float dt) {
    if(params.vorticity <= 0.0f) return;
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i)
            _curl[idx(i, j)] = 0.5f * ((_v[idx(i + 1, j)] - _v[idx(i - 1, j)]) - (_u[idx(i, j + 1)] - _u[idx(i, j - 1)]));
    setBoundary(0, _curl);
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i) {
            // N points up the |curl| gradient; force = eps * (N x curl)
            const float nx = 0.5f * (std::abs(_curl[idx(i + 1, j)]) - std::abs(_curl[idx(i - 1, j)]));
            const float ny = 0.5f * (std::abs(_curl[idx(i, j + 1)]) - std::abs(_curl[idx(i, j - 1)]));
            const float length = std::sqrt(nx * nx + ny * ny) + 1e-5f;
            const float w = _curl[idx(i, j)];
            _u[idx(i, j)] += dt * params.vorticity * (ny / length) * w;
            _v[idx(i, j)] -= dt * params.vorticity * (nx / length) * w;
        }
    setBoundary(1, _u);
    setBoundary(2, _v);
}

// wrap just switched on: the two x edges were walls and now touch, so cross-fade a band on
// each side toward their shared average instead of leaving a hard line where they meet
void CpuStableFluid::blendSeam() {
    const int band = std::min(8, _w / 2);
    for(std::vector<float>* field : {&_u, &_v, &_dye[0], &_dye[1], &_dye[2]}) {
        std::vector<float>& x = *field;
        for(int j = 1; j <= _h; ++j) {
            const float m = 0.5f * (x[idx(1, j)] + x[idx(_w, j)]);
            for(int d = 0; d < band; ++d) {
                const float t = 1.0f - static_cast<float>(d) / static_cast<float>(band); // 1 at the edge
                x[idx(1 + d, j)] += (m - x[idx(1 + d, j)]) * t;
                x[idx(_w - d, j)] += (m - x[idx(_w - d, j)]) * t;
            }
        }
        setBoundary(field == &_u ? 1 : field == &_v ? 2 : 0, x);
    }
}

void CpuStableFluid::step(float dt) {
    if(params.wrapX && !_wrapped) blendSeam();
    _wrapped = params.wrapX;
    applySplats();
    confineVorticity(dt);
    if(params.viscosity > 0.0f) {
        const float a = dt * params.viscosity;
        _u0 = _u;
        _v0 = _v;
        linearSolve(1, _u, _u0, a, 1.0f + 4.0f * a, 20);
        linearSolve(2, _v, _v0, a, 1.0f + 4.0f * a, 20);
    }
    project();
    _u0 = _u;
    _v0 = _v;
    advect(1, _u, _u0, _u0, _v0, dt);
    advect(2, _v, _v0, _u0, _v0, dt);
    project();
    for(int c = 0; c < 3; ++c) {
        _dye0[c] = _dye[c];
        advect(0, _dye[c], _dye0[c], _u, _v, dt);
    }
    const float frames = dt * 60.0f;
    const float dyeKeep = std::pow(params.dyeFade, frames);
    const float velocityKeep = std::pow(params.velocityFade, frames);
    for(std::size_t k = 0; k < _u.size(); ++k) {
        _u[k] *= velocityKeep;
        _v[k] *= velocityKeep;
        for(int c = 0; c < 3; ++c) _dye[c][k] *= dyeKeep;
    }
    refreshDyeOut();
}

void CpuStableFluid::refreshDyeOut() {
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i) {
            const int k = idx(i, j);
            _dyeOut[static_cast<std::size_t>((i - 1) + (j - 1) * _w)] = Color3{_dye[0][k], _dye[1][k], _dye[2][k]};
        }
}

Vec2 CpuStableFluid::velocityAt(Vec2 at) const {
    const float x = std::clamp(at.x() * static_cast<float>(_w) + 0.5f, 0.0f, static_cast<float>(_w) + 1.0f);
    const float y = std::clamp(at.y() * static_cast<float>(_h) + 0.5f, 0.0f, static_cast<float>(_h) + 1.0f);
    const int i0 = std::min(static_cast<int>(x), _w);
    const int j0 = std::min(static_cast<int>(y), _h);
    const float s = x - static_cast<float>(i0);
    const float t = y - static_cast<float>(j0);
    const auto sample = [&](const std::vector<float>& f) {
        return (1.0f - s) * ((1.0f - t) * f[idx(i0, j0)] + t * f[idx(i0, j0 + 1)]) +
               s * ((1.0f - t) * f[idx(i0 + 1, j0)] + t * f[idx(i0 + 1, j0 + 1)]);
    };
    return Vec2{sample(_u), sample(_v)} / static_cast<float>(_h);
}

void CpuStableFluid::setCellVelocity(int i, int j, Vec2 cellsPerSecond) {
    _u[idx(i + 1, j + 1)] = cellsPerSecond.x();
    _v[idx(i + 1, j + 1)] = cellsPerSecond.y();
    setBoundary(1, _u);
    setBoundary(2, _v);
}

Vec2 CpuStableFluid::cellVelocity(int i, int j) const { return Vec2{_u[idx(i + 1, j + 1)], _v[idx(i + 1, j + 1)]}; }

float CpuStableFluid::maxAbsDivergence() const {
    float m = 0.0f;
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i)
            m = std::max(m, std::abs(0.5f * (_u[idx(i + 1, j)] - _u[idx(i - 1, j)] + _v[idx(i, j + 1)] - _v[idx(i, j - 1)])));
    return m;
}

float CpuStableFluid::totalAbsCurl() const {
    float sum = 0.0f;
    for(int j = 1; j <= _h; ++j)
        for(int i = 1; i <= _w; ++i)
            sum += std::abs(0.5f * ((_v[idx(i + 1, j)] - _v[idx(i - 1, j)]) - (_u[idx(i, j + 1)] - _u[idx(i, j - 1)])));
    return sum;
}

Color3 CpuStableFluid::totalDye() const {
    Color3 sum{0.0f};
    for(const Color3& c : _dyeOut) sum += c;
    return sum;
}

} // namespace vibe::fluid
