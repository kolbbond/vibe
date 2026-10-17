// WaterfallRibbonShader - waveform lines as screen-space ribbons (real width + AA edges)
// and the black fill under each history row. Core GL has no wide or smooth lines, so
// every point is two vertices pushed apart along the screen-space normal.
#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

namespace vibe {

class WaterfallRibbonShader : public Magnum::GL::AbstractShaderProgram {
public:
    // grid-space point (x = column, y = amplitude, z = row) and its polyline neighbours
    using Position = Magnum::GL::Attribute<0, Magnum::Vector3>;
    using Prev = Magnum::GL::Attribute<1, Magnum::Vector3>;
    using Next = Magnum::GL::Attribute<2, Magnum::Vector3>;
    // x: side (-1/+1 across a line; for a fill 0 = on the curve, 1 = on the floor)
    // y: kind (0 = line, 1 = fill)
    using Side = Magnum::GL::Attribute<3, Magnum::Vector2>;

    struct Vertex {
        Magnum::Vector3 position;
        Magnum::Vector3 prev;
        Magnum::Vector3 next;
        Magnum::Vector2 side;
    };

    explicit WaterfallRibbonShader(Magnum::NoCreateT): Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {}
    explicit WaterfallRibbonShader();

    WaterfallRibbonShader& setTransformationMatrix(const Magnum::Matrix4& matrix);
    WaterfallRibbonShader& setProjectionMatrix(const Magnum::Matrix4& matrix);
    WaterfallRibbonShader& setGridSize(const Magnum::Vector2& size); // cols, rows
    WaterfallRibbonShader& setHeightScale(float scale);
    WaterfallRibbonShader& setViewport(const Magnum::Vector2& size); // render target, px
    WaterfallRibbonShader& setLineWidth(float px);
    WaterfallRibbonShader& setGlow(float strength); // 0 = core pass, >0 = wide additive halo pass
    WaterfallRibbonShader& setFloor(float worldY);  // fill bottom
    WaterfallRibbonShader& setColors(const Magnum::Color3& base, const Magnum::Color3& peak);
    WaterfallRibbonShader& setFillColor(const Magnum::Color3& color);
    WaterfallRibbonShader& setFade(float amount);     // 0 = no distance fade, 1 = far rows near black
    WaterfallRibbonShader& setScanlines(float amount, float time);

private:
    Magnum::Int _transformationMatrix{0}, _projectionMatrix{0}, _gridSize{0}, _heightScale{0}, _viewport{0},
        _lineWidth{0}, _glow{0}, _floor{0}, _colorBase{0}, _colorPeak{0}, _fillColor{0}, _fade{0}, _scanlines{0},
        _time{0};
};

} // namespace vibe
