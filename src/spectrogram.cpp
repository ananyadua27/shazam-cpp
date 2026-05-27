// spectrogram.cpp — STFT magnitude spectrogram

#include "spectrogram.hpp"
#include "fft.hpp"
#include <complex>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace shazam {

static constexpr float PI = 3.14159265358979323846f;

// Hann window: w[n] = 0.5 * (1 - cos(2π n / (N-1)))
// Reduces spectral leakage from the implicit rectangular windowing of each
// frame — essential so that peaks in the spectrogram correspond to real
// frequency components rather than edge-effect artefacts.
static std::vector<float> make_hann_window(int N) {
    std::vector<float> w(N);
    for (int n = 0; n < N; ++n)
        w[n] = 0.5f * (1.0f - std::cos(2.0f * PI * n / (N - 1)));
    return w;
}

Spectrogram compute_spectrogram(const std::vector<float>& audio,
                                const SpectrogramParams& params) {
    const int N    = params.window_size;
    const int hop  = params.hop_size;
    const int bins = params.max_freq_bin - params.min_freq_bin;

    if (bins <= 0)
        throw std::invalid_argument("max_freq_bin must be > min_freq_bin");

    const auto hann = make_hann_window(N);
    const int n_frames = static_cast<int>((audio.size() - N) / hop) + 1;
    if (n_frames <= 0) return {};

    // Output: [freq_bin][time_frame] layout chosen so that the inner loop
    // over time is contiguous in memory during peak search.
    Spectrogram spec(bins, std::vector<float>(n_frames, 0.0f));

    std::vector<std::complex<float>> frame(N);

    for (int t = 0; t < n_frames; ++t) {
        const int start = t * hop;

        // Apply Hann window to this frame
        for (int n = 0; n < N; ++n)
            frame[n] = {hann[n] * audio[start + n], 0.0f};

        fft(frame);  // O(N log N) 

        // Store magnitude for the frequency range we care about.
        // Only need bins 0..N/2 (positive frequencies); the upper half is
        // the conjugate mirror.
        for (int b = 0; b < bins; ++b) {
            const int bin_idx = b + params.min_freq_bin;
            spec[b][t] = std::abs(frame[bin_idx]);
        }
    }

    return spec;
}

std::vector<float> stereo_to_mono(const std::vector<float>& interleaved) {
    const std::size_t frames = interleaved.size() / 2;
    std::vector<float> mono(frames);
    for (std::size_t i = 0; i < frames; ++i)
        mono[i] = (interleaved[2 * i] + interleaved[2 * i + 1]) * 0.5f;
    return mono;
}

std::vector<float> resample(const std::vector<float>& audio,
                            int src_rate, int target_rate) {
    if (src_rate == target_rate) return audio;
    const double ratio = static_cast<double>(target_rate) / src_rate;
    const std::size_t out_len =
        static_cast<std::size_t>(audio.size() * ratio);
    std::vector<float> out(out_len);
    for (std::size_t i = 0; i < out_len; ++i) {
        const double src_pos = i / ratio;
        const std::size_t lo = static_cast<std::size_t>(src_pos);
        const std::size_t hi = std::min(lo + 1, audio.size() - 1);
        const float frac = static_cast<float>(src_pos - lo);
        out[i] = audio[lo] * (1.0f - frac) + audio[hi] * frac;
    }
    return out;
}

} 
