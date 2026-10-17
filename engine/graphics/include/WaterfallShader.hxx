// WaterfallShader - 3D terrain mesh with height-based coloring
#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

namespace vibe {

class WaterfallShader : public Magnum::GL::AbstractShaderProgram {
public:
    using Position = Magnum::GL::Attribute<0, Magnum::Vector3>;

    explicit WaterfallShader(Magnum::NoCreateT): Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {}
    explicit WaterfallShader();

    WaterfallShader& setTransformationMatrix(const Magnum::Matrix4& matrix);
    WaterfallShader& setProjectionMatrix(const Magnum::Matrix4& matrix);
    WaterfallShader& setTime(float time);
    WaterfallShader& setHeightScale(float scale);
    WaterfallShader& setColorLow(const Magnum::Color3& color);
    WaterfallShader& setColorMid(const Magnum::Color3& color);
    WaterfallShader& setColorHigh(const Magnum::Color3& color);
    WaterfallShader& setGridSize(const Magnum::Vector2& size);  // cols, rows

private:
    Magnum::Int _transformationMatrixUniform{0},
                _projectionMatrixUniform{0},
                _timeUniform{0},
                _heightScaleUniform{0},
                _colorLowUniform{0},
                _colorMidUniform{0},
                _colorHighUniform{0},
                _gridSizeUniform{0};
};

} // namespace vibe
