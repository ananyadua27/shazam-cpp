#pragma once
// spectrogram.hpp, Short-Time Fourier Transform (STFT)
// Produces the time-frequency magnitude spectrogram used for peak picking.
// Each column is the |FFT| of one Hann-windowed frame of audio, mirroring
// the real-world Shazam pipeline 

#include <vector>
#include <cstddef>

namespace shazam {

struct SpectrogramParams {
    int window_size = 4096;   // FFT window (samples)
    int hop_size    = 512;    // frame step; overlap = window - hop
    int sample_rate = 44100;  // Hz
    int min_freq_bin = 10;    // ignore DC and very low bins  (~108 Hz at 44.1 kHz / 4096)
    int max_freq_bin = 512;   // focus on 0–5.5 kHz where vocal/instrument content lives
};

// 2D magnitude spectrogram: [freq_bin][time_frame]
using Spectrogram = std::vector<std::vector<float>>;

/// Compute magnitude spectrogram of mono audio signal.
/// audio     — mono PCM samples in [-1, 1]
/// params    — STFT configuration
Spectrogram compute_spectrogram(const std::vector<float>& audio,
                                const SpectrogramParams& params = {});

/// Convert an audio stereo (interleaved L,R) buffer to mono by averaging.
std::vector<float> stereo_to_mono(const std::vector<float>& interleaved);

/// Resample audio to target_rate using linear interpolation (good enough for
/// fingerprinting; avoids a heavy dependency like libresample).
std::vector<float> resample(const std::vector<float>& audio,
                            int src_rate, int target_rate);

}
