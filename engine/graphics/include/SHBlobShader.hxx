// SHBlobShader - Audio-reactive blob with FFT-driven radial displacement
#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Corrade/Containers/ArrayView.h>

namespace vibe {

class SHBlobShader : public Magnum::GL::AbstractShaderProgram {
public:
    static constexpr int MaxFFTBins = 128;

    using Position = Magnum::GL::Attribute<0, Magnum::Vector4>;
    using Normal = Magnum::GL::Attribute<1, Magnum::Vector3>;

    explicit SHBlobShader(Magnum::NoCreateT): Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {}
    explicit SHBlobShader();

    SHBlobShader& setTransformationMatrix(const Magnum::Matrix4& matrix);
    SHBlobShader& setProjectionMatrix(const Magnum::Matrix4& matrix);
    SHBlobShader& setNormalMatrix(const Magnum::Matrix3& matrix);
    SHBlobShader& setTime(float time);
    SHBlobShader& setSnapResolution(float res);
    SHBlobShader& setBaseColor(const Magnum::Color3& color);
    SHBlobShader& setRimColor(const Magnum::Color3& color);
    SHBlobShader& setRimPower(float power);

    // 5-band audio coefficients (kept for overall energy / color)
    SHBlobShader& setSub(float v);
    SHBlobShader& setKick(float v);
    SHBlobShader& setMid(float v);
    SHBlobShader& setSnare(float v);
    SHBlobShader& setHat(float v);

    // FFT bin data for radial displacement
    SHBlobShader& setFFTBins(Corrade::Containers::ArrayView<const float> bins);
    SHBlobShader& setNumBins(int count);

private:
    Magnum::Int _transformationMatrixUniform{0},
                _projectionMatrixUniform{0},
                _normalMatrixUniform{0},
                _timeUniform{0},
                _snapResolutionUniform{0},
                _baseColorUniform{0},
                _rimColorUniform{0},
                _rimPowerUniform{0},
                _subUniform{0},
                _kickUniform{0},
                _midUniform{0},
                _snareUniform{0},
                _hatUniform{0},
                _fftBinsUniform{0},
                _numBinsUniform{0};
};

} // namespace vibe
