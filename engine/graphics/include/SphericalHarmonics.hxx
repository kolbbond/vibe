// SphericalHarmonics.hxx — Header-only real SH basis functions for l=0,1,2
// Maps 5 audio bands (sub/kick/mid/snare/hat) to SH coefficients for blob deformation
#pragma once

#include <cmath>
#include <array>

namespace vibe { namespace sh {

// Real spherical harmonic basis Y_l^m(theta, phi)
// theta = polar angle from +Z axis [0, pi]
// phi = azimuthal angle in XY plane [0, 2*pi]
// Using real-valued conventions (cosine for m>0, sine for m<0)

inline float sh_basis(int l, int m, float theta, float phi) {
    float ct = std::cos(theta);
    float st = std::sin(theta);

    if (l == 0 && m == 0) {
        // Y_0^0 = 1/(2*sqrt(pi)) ≈ 0.28209
        return 0.28209479177f;
    }

    if (l == 1) {
        if (m == -1) {
            // Y_1^{-1} = sqrt(3/(4*pi)) * sin(theta) * sin(phi)
            return 0.48860251190f * st * std::sin(phi);
        }
        if (m == 0) {
            // Y_1^0 = sqrt(3/(4*pi)) * cos(theta)
            return 0.48860251190f * ct;
        }
        if (m == 1) {
            // Y_1^1 = sqrt(3/(4*pi)) * sin(theta) * cos(phi)
            return 0.48860251190f * st * std::cos(phi);
        }
    }

    if (l == 2) {
        if (m == -2) {
            // Y_2^{-2} = sqrt(15/(4*pi)) * sin^2(theta) * sin(2*phi) / 2
            // = 0.5 * sqrt(15/pi) * sin^2(theta) * sin(phi) * cos(phi)
            return 0.54627421529f * st * st * std::sin(2.0f * phi);
        }
        if (m == -1) {
            // Y_2^{-1} = sqrt(15/(4*pi)) * sin(theta) * cos(theta) * sin(phi)
            return 1.09254843059f * st * ct * std::sin(phi);
        }
        if (m == 0) {
            // Y_2^0 = sqrt(5/(16*pi)) * (3*cos^2(theta) - 1)
            return 0.31539156525f * (3.0f * ct * ct - 1.0f);
        }
        if (m == 1) {
            // Y_2^1 = sqrt(15/(4*pi)) * sin(theta) * cos(theta) * cos(phi)
            return 1.09254843059f * st * ct * std::cos(phi);
        }
        if (m == 2) {
            // Y_2^2 = sqrt(15/(16*pi)) * sin^2(theta) * cos(2*phi)
            return 0.54627421529f * st * st * std::cos(2.0f * phi);
        }
    }

    return 0.0f;
}

// Evaluate SH audio deformation at a point on the sphere
// coeffs[5] maps: sub→Y₀⁰, kick→Y₁⁰, mid→Y₁¹, snare→Y₂⁰, hat→Y₂¹
// Returns base_radius + weighted SH sum (always positive via abs)
inline float sh_audio_eval(float theta, float phi, const std::array<float, 5>& coeffs) {
    float r = 1.0f;
    r += coeffs[0] * sh_basis(0, 0, theta, phi);   // sub → uniform breathing
    r += coeffs[1] * sh_basis(1, 0, theta, phi);   // kick → vertical bulge
    r += coeffs[2] * sh_basis(1, 1, theta, phi);   // mid → side lobe
    r += coeffs[3] * sh_basis(2, 0, theta, phi);   // snare → cross/clover
    r += coeffs[4] * sh_basis(2, 1, theta, phi);   // hat → pinch
    return r;
}

}} // namespace vibe::sh
