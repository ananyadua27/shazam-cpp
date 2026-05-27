#pragma once
// fft.hpp — Cooley-Tukey Radix-2 Decimation-in-Time FFT
// X[k] = A[k] + e^{-i 2π k / N} · B[k]   k = 0 … N-1
// where A, B are N/2-point DFTs of the even- and odd-indexed
// sub-sequences respectively.  

#include <complex>
#include <vector>
#include <cstddef>

namespace shazam {

/// In-place radix-2 FFT.  Length of x must be a power of two.
/// Complexity: O(N log N) 
void fft(std::vector<std::complex<float>>& x);

/// In-place inverse FFT (conjugate -> forward FFT -> conjugate -> scale).
void ifft(std::vector<std::complex<float>>& x);

/// Returns the next power of two >= n
std::size_t next_pow2(std::size_t n);

/// Zero-pad x to the next power of two in-place.
void zero_pad_to_pow2(std::vector<std::complex<float>>& x);

} 
