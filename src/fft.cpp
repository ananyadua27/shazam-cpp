// fft.cpp — iterative Cooley-Tukey radix-2 DIT FFT
// Stage-by-stage butterfly matching 
//   X[k]         = A[k mod N/2] + W_N^k · B[k mod N/2]
//   X[k + N/2]   = A[k mod N/2] - W_N^k · B[k mod N/2]

#include "fft.hpp"
#include <cmath>
#include <cassert>
#include <stdexcept>

namespace shazam {

static constexpr float PI = 3.14159265358979323846f;

// Bit-reversal permutation: reorders samples so that the iterative
// bottom-up butterfly is equivalent to the top-down recursive split.
static void bit_reverse(std::vector<std::complex<float>>& x) {
    const std::size_t N = x.size();
    std::size_t j = 0;
    for (std::size_t i = 1; i < N; ++i) {
        std::size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(x[i], x[j]);
    }
}

void fft(std::vector<std::complex<float>>& x) {
    const std::size_t N = x.size();
    if (N == 0) return;
    if ((N & (N - 1)) != 0)
        throw std::invalid_argument("FFT: length must be a power of two");

    bit_reverse(x);

    // Bottom-up butterfly — each outer loop doubles the sub-DFT size,
    // corresponding to one level of the lab's recursion tree.
    for (std::size_t s = 1; (1u << s) <= N; ++s) {
        const std::size_t m   = 1u << s;          // sub-DFT size at this stage
        const std::size_t m2  = m >> 1;           // half-size
        // Principal N-th root of unity for this stage (twiddle factor base)
        const std::complex<float> W_m{
            std::cos(-2.0f * PI / static_cast<float>(m)),
            std::sin(-2.0f * PI / static_cast<float>(m))
        };

        for (std::size_t k = 0; k < N; k += m) {
            std::complex<float> w{1.0f, 0.0f};  // w = W_m^j
            for (std::size_t j = 0; j < m2; ++j) {
                const auto t   = w * x[k + j + m2]; // twiddle × odd part
                const auto u   = x[k + j];           // even part
                x[k + j]       = u + t;              // butterfly top
                x[k + j + m2]  = u - t;              // butterfly bottom
                w *= W_m;                            // advance twiddle factor
            }
        }
    }
}

void ifft(std::vector<std::complex<float>>& x) {
    for (auto& v : x) v = std::conj(v);
    fft(x);
    const float inv_N = 1.0f / static_cast<float>(x.size());
    for (auto& v : x) v = std::conj(v) * inv_N;
}

std::size_t next_pow2(std::size_t n) {
    if (n == 0) return 1;
    --n;
    for (std::size_t shift = 1; shift < sizeof(n) * 8; shift <<= 1)
        n |= n >> shift;
    return n + 1;
}

void zero_pad_to_pow2(std::vector<std::complex<float>>& x) {
    x.resize(next_pow2(x.size()), {0.0f, 0.0f});
}

} 
