#pragma once
// fingerprint.hpp 
// Implements the core algorithm from:
//   Wang, A. (2003). "An Industrial-Strength Audio Search Algorithm."
//   Proc. ISMIR 2003.
//   1. Find local maxima in the spectrogram -> "constellation map"
//   2. For each anchor peak, pair it with nearby peaks in a target zone
//   3. Encode each (f1, f2, Δt) pair as a 32-bit hash
//
// The hash is noise-robust because:
//   - Only peaks survive additive noise (low-energy regions wash out)
//   - Relative coordinates (Δt, f1, f2) are invariant to playback offset
//     and microphone gain — only their ratio/difference matters

#include "spectrogram.hpp"
#include <vector>
#include <cstdint>

namespace shazam {

struct Peak {
    int   freq_bin;    // bin index within the clipped range [min_freq, max_freq)
    int   time_frame;  // STFT frame index
    float magnitude;   // |FFT bin| at this point
};

struct FingerprintParams {
    // Peak extraction
    int   neighborhood_freq  = 3;   // ±bins around candidate for local max test
    int   neighborhood_time  = 3;   // ±frames around candidate
    int   peaks_per_frame    = 5;   // keep at most this many per time frame
    float min_magnitude      = 10.0f; // ignore very quiet bins (noise floor)

    int   fan_out            = 10;  // pairs per anchor
    int   target_t_min       = 1;   // earliest target frame relative to anchor
    int   target_t_max       = 100; // latest  target frame relative to anchor
    int   target_f_range     = 100; // ±bins for target peak freq
};

struct Fingerprint {
    uint32_t hash;        // packed (f1, f2, Δt) 32-bit key into the DB
    int      time_offset; // absolute time_frame of the anchor peak
};

inline uint32_t encode_hash(int f1, int f2, int dt) {
    return (static_cast<uint32_t>(f1 & 0x1FF) << 23)
         | (static_cast<uint32_t>(f2 & 0x1FF) << 14)
         | (static_cast<uint32_t>(dt & 0x3FFF));
}

/// Extract local-maxima peaks from a magnitude spectrogram.
std::vector<Peak> extract_peaks(const Spectrogram& spec,
                                const FingerprintParams& params = {});

/// Generate fingerprint hashes from a constellation map.
std::vector<Fingerprint> generate_fingerprints(const std::vector<Peak>& peaks,
                                               const FingerprintParams& params = {});

/// Convenience: spectrogram -> fingerprints in one call.
std::vector<Fingerprint> fingerprint_audio(const std::vector<float>& audio,
                                           const SpectrogramParams&   sp = {},
                                           const FingerprintParams&   fp = {});

} 
