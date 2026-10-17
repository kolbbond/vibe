#include <cstdio>
#include <cmath>
#include <array>
#include "vibe/graphics/SphericalHarmonics.hxx"

static int failures = 0;

static void check_approx(const char* name, float actual, float expected, float tol = 1e-4f) {
    float diff = std::abs(actual - expected);
    if (diff > tol) {
        std::printf("FAIL: %s — expected %.6f, got %.6f (diff %.6f)\n",
                    name, static_cast<double>(expected), static_cast<double>(actual),
                    static_cast<double>(diff));
        failures++;
    } else {
        std::printf("  ok: %s\n", name);
    }
}

int main() {
    using namespace vibe::sh;

    constexpr float PI = 3.14159265358979f;

    // --- Y_0^0: constant everywhere ---
    float y00_val = 0.28209479177f;
    check_approx("Y00 at north pole", sh_basis(0, 0, 0.0f, 0.0f), y00_val);
    check_approx("Y00 at equator", sh_basis(0, 0, PI / 2.0f, 0.0f), y00_val);
    check_approx("Y00 at south pole", sh_basis(0, 0, PI, 0.0f), y00_val);
    check_approx("Y00 at random angle", sh_basis(0, 0, 1.23f, 4.56f), y00_val);

    // --- Y_1^0: proportional to cos(theta) ---
    float y10_coeff = 0.48860251190f;
    check_approx("Y10 at north pole (theta=0)", sh_basis(1, 0, 0.0f, 0.0f), y10_coeff);
    check_approx("Y10 at equator (theta=pi/2)", sh_basis(1, 0, PI / 2.0f, 0.0f), 0.0f, 1e-4f);
    check_approx("Y10 at south pole (theta=pi)", sh_basis(1, 0, PI, 0.0f), -y10_coeff);

    // --- Y_1^1: proportional to sin(theta)*cos(phi) ---
    float y11_coeff = 0.48860251190f;
    check_approx("Y11 at equator phi=0", sh_basis(1, 1, PI / 2.0f, 0.0f), y11_coeff);
    check_approx("Y11 at equator phi=pi/2", sh_basis(1, 1, PI / 2.0f, PI / 2.0f), 0.0f, 1e-4f);
    check_approx("Y11 at equator phi=pi", sh_basis(1, 1, PI / 2.0f, PI), -y11_coeff);
    check_approx("Y11 at north pole", sh_basis(1, 1, 0.0f, 0.0f), 0.0f, 1e-4f);

    // --- Y_2^0: (3*cos^2(theta) - 1) symmetry ---
    float y20_coeff = 0.31539156525f;
    check_approx("Y20 at north pole", sh_basis(2, 0, 0.0f, 0.0f), y20_coeff * 2.0f);
    check_approx("Y20 at south pole", sh_basis(2, 0, PI, 0.0f), y20_coeff * 2.0f);
    check_approx("Y20 at equator", sh_basis(2, 0, PI / 2.0f, 0.0f), -y20_coeff);
    // Magic angle: 3*cos^2(theta) - 1 = 0 at theta = acos(1/sqrt(3)) ≈ 54.74°
    float magic = std::acos(1.0f / std::sqrt(3.0f));
    check_approx("Y20 at magic angle", sh_basis(2, 0, magic, 0.0f), 0.0f, 1e-3f);

    // --- Y_2^1: sin(theta)*cos(theta)*cos(phi) ---
    float y21_coeff = 1.09254843059f;
    // At theta=pi/4, cos(pi/4)*sin(pi/4) = 0.5
    check_approx("Y21 at 45deg phi=0", sh_basis(2, 1, PI / 4.0f, 0.0f), y21_coeff * 0.5f);
    check_approx("Y21 at equator", sh_basis(2, 1, PI / 2.0f, 0.0f), 0.0f, 1e-4f);

    // --- sh_audio_eval: zero coefficients → base radius 1.0 ---
    std::array<float, 5> zeros = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    check_approx("sh_audio_eval zeros", sh_audio_eval(PI / 2.0f, 0.0f, zeros), 1.0f);
    check_approx("sh_audio_eval zeros at pole", sh_audio_eval(0.0f, 0.0f, zeros), 1.0f);

    // --- Numerical orthogonality: integral of Y_0^0 * Y_1^0 over sphere ≈ 0 ---
    {
        double integral = 0.0;
        int N_theta = 200;
        int N_phi = 200;
        double dtheta = PI / N_theta;
        double dphi = 2.0 * PI / N_phi;

        for (int i = 0; i < N_theta; i++) {
            double theta = (i + 0.5) * dtheta;
            double sin_theta = std::sin(theta);
            for (int j = 0; j < N_phi; j++) {
                double phi = (j + 0.5) * dphi;
                double y00 = sh_basis(0, 0, static_cast<float>(theta), static_cast<float>(phi));
                double y10 = sh_basis(1, 0, static_cast<float>(theta), static_cast<float>(phi));
                integral += y00 * y10 * sin_theta * dtheta * dphi;
            }
        }
        check_approx("orthogonality Y00*Y10", static_cast<float>(integral), 0.0f, 0.01f);
    }

    // --- Normalization: integral of |Y_0^0|^2 over sphere = 1 ---
    {
        double integral = 0.0;
        int N_theta = 200;
        int N_phi = 200;
        double dtheta = PI / N_theta;
        double dphi = 2.0 * PI / N_phi;

        for (int i = 0; i < N_theta; i++) {
            double theta = (i + 0.5) * dtheta;
            double sin_theta = std::sin(theta);
            for (int j = 0; j < N_phi; j++) {
                double phi = (j + 0.5) * dphi;
                double y00 = sh_basis(0, 0, static_cast<float>(theta), static_cast<float>(phi));
                integral += y00 * y00 * sin_theta * dtheta * dphi;
            }
        }
        check_approx("normalization |Y00|^2", static_cast<float>(integral), 1.0f, 0.01f);
    }

    std::printf("\n%s: %d failure(s)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures;
}
