// test_fft.cpp — verify Cooley-Tukey FFT against brute-force DFT

#include "fft.hpp"
#include <complex>
#include <vector>
#include <cmath>
#include <cassert>
#include <iostream>
#include <numeric>

static constexpr float PI   = 3.14159265358979f;
static constexpr float EPS  = 1e-3f;

static std::vector<std::complex<float>> naive_dft(const std::vector<std::complex<float>>& x) {
    const int N = static_cast<int>(x.size());
    std::vector<std::complex<float>> X(N, {0.f, 0.f});
    for (int k = 0; k < N; ++k)
        for (int n = 0; n < N; ++n)
            X[k] += x[n] * std::exp(std::complex<float>(0.f, -2.f * PI * n * k / N));
    return X;
}

static bool approx_equal(const std::vector<std::complex<float>>& a,
                          const std::vector<std::complex<float>>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::abs(a[i] - b[i]) > EPS) return false;
    return true;
}


static void test_impulse() {
    // δ[n] → all 1s in frequency (flat spectrum)
    std::vector<std::complex<float>> x(16, {0.f, 0.f});
    x[0] = {1.f, 0.f};
    auto y = x;
    shazam::fft(y);
    for (const auto& v : y)
        assert(std::abs(v - std::complex<float>{1.f, 0.f}) < EPS);
    std::cout << "  [PASS] impulse → flat spectrum\n";
}

static void test_dc() {
    // x[n] = 1 → X[0] = N, X[k≠0] = 0
    const int N = 32;
    std::vector<std::complex<float>> x(N, {1.f, 0.f});
    shazam::fft(x);
    assert(std::abs(x[0] - std::complex<float>{static_cast<float>(N), 0.f}) < EPS);
    for (int k = 1; k < N; ++k)
        assert(std::abs(x[k]) < EPS);
    std::cout << "  [PASS] DC → X[0]=N, others zero\n";
}

static void test_matches_naive_dft() {
    const int N = 64;
    std::vector<std::complex<float>> x(N);
    for (int n = 0; n < N; ++n)
        x[n] = {std::sin(2.f * PI * 3.f * n / N) +
                std::cos(2.f * PI * 7.f * n / N), 0.f};

    auto ref = naive_dft(x);
    auto got = x;
    shazam::fft(got);

    assert(approx_equal(ref, got));
    std::cout << "  [PASS] matches naive O(N²) DFT (N=" << N << ")\n";
}

static void test_ifft_roundtrip() {
    const int N = 128;
    std::vector<std::complex<float>> original(N);
    for (int n = 0; n < N; ++n)
        original[n] = {static_cast<float>(n % 7) * 0.1f, 0.f};

    auto x = original;
    shazam::fft(x);
    shazam::ifft(x);

    for (int n = 0; n < N; ++n)
        assert(std::abs(x[n] - original[n]) < EPS);
    std::cout << "  [PASS] IFFT(FFT(x)) == x\n";
}

static void test_linearity() {
    // FFT(a·x + b·y) = a·FFT(x) + b·FFT(y)
    const int N  = 32;
    const float a = 3.0f, b = -1.5f;
    std::vector<std::complex<float>> x(N), y(N);
    for (int n = 0; n < N; ++n) {
        x[n] = {std::cos(2.f * PI * n / N), 0.f};
        y[n] = {std::sin(2.f * PI * 2.f * n / N), 0.f};
    }

    std::vector<std::complex<float>> z(N);
    for (int n = 0; n < N; ++n) z[n] = a * x[n] + b * y[n];

    auto Fx = x; shazam::fft(Fx);
    auto Fy = y; shazam::fft(Fy);
    auto Fz = z; shazam::fft(Fz);

    for (int k = 0; k < N; ++k)
        assert(std::abs(Fz[k] - (a * Fx[k] + b * Fy[k])) < EPS);
    std::cout << "  [PASS] linearity: FFT(a·x + b·y) = a·FFT(x) + b·FFT(y)\n";
}

int main() {
    std::cout << "=== FFT Tests ===\n";
    test_impulse();
    test_dc();
    test_matches_naive_dft();
    test_ifft_roundtrip();
    test_linearity();
    std::cout << "All FFT tests passed.\n";
}
