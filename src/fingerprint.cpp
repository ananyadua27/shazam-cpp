// fingerprint.cpp — constellation map and combinatorial hashing

#include "fingerprint.hpp"
#include <algorithm>
#include <stdexcept>

namespace shazam {

// Peak Extraction
// A cell (b, t) is a peak if:
//   1. spec[b][t] >= min_magnitude  (not just noise)
//   2. spec[b][t] is the maximum within the [b±nb, t±nt] neighbourhood
//
// This produces the "constellation map", a sparse set of salient
// time-frequency points that survive compression, room noise, and
// lossy recording.

std::vector<Peak> extract_peaks(const Spectrogram& spec,
                                const FingerprintParams& params) {
    if (spec.empty() || spec[0].empty()) return {};

    const int n_bins   = static_cast<int>(spec.size());
    const int n_frames = static_cast<int>(spec[0].size());
    const int nb       = params.neighborhood_freq;
    const int nt       = params.neighborhood_time;

    std::vector<Peak> all_peaks;

    for (int b = nb; b < n_bins - nb; ++b) {
        for (int t = nt; t < n_frames - nt; ++t) {
            const float val = spec[b][t];
            if (val < params.min_magnitude) continue;

            // Check local maximum within rectangular neighbourhood
            bool is_max = true;
            for (int db = -nb; db <= nb && is_max; ++db)
                for (int dt = -nt; dt <= nt && is_max; ++dt)
                    if ((db != 0 || dt != 0) && spec[b + db][t + dt] > val)
                        is_max = false;

            if (is_max)
                all_peaks.push_back({b, t, val});
        }
    }

    // Per-frame cap: keep only the N strongest peaks in each time frame.
    // This mimics Shazam's "top peaks per segment" strategy, keeping the
    // constellation sparse enough for fast matching without losing the
    // most prominent notes/harmonics.

    // Group by frame, sort by magnitude desc, truncate
    std::sort(all_peaks.begin(), all_peaks.end(),
              [](const Peak& a, const Peak& b_) {
                  // primary sort by time frame, secondary by magnitude (desc)
                  return a.time_frame != b_.time_frame
                             ? a.time_frame < b_.time_frame
                             : a.magnitude   > b_.magnitude;
              });

    std::vector<Peak> filtered;
    filtered.reserve(all_peaks.size());
    int cur_frame = -1, count = 0;
    for (const auto& p : all_peaks) {
        if (p.time_frame != cur_frame) { cur_frame = p.time_frame; count = 0; }
        if (count < params.peaks_per_frame) {
            filtered.push_back(p);
            ++count;
        }
    }

    // Re-sort by time (needed for the target-zone pairing below)
    std::sort(filtered.begin(), filtered.end(),
              [](const Peak& a, const Peak& b_) {
                  return a.time_frame < b_.time_frame;
              });

    return filtered;
}

std::vector<Fingerprint> generate_fingerprints(const std::vector<Peak>& peaks,
                                               const FingerprintParams& params) {
    std::vector<Fingerprint> fps;
    fps.reserve(peaks.size() * params.fan_out);

    const int n = static_cast<int>(peaks.size());

    for (int i = 0; i < n; ++i) {
        const auto& anchor = peaks[i];
        int paired = 0;

        for (int j = i + 1; j < n && paired < params.fan_out; ++j) {
            const auto& target = peaks[j];
            const int dt = target.time_frame - anchor.time_frame;

            if (dt < params.target_t_min) continue;
            if (dt > params.target_t_max) break; // peaks are time-sorted

            const int df = std::abs(target.freq_bin - anchor.freq_bin);
            if (df > params.target_f_range) continue;

            fps.push_back({
                encode_hash(anchor.freq_bin, target.freq_bin, dt),
                anchor.time_frame
            });
            ++paired;
        }
    }

    return fps;
}

std::vector<Fingerprint> fingerprint_audio(const std::vector<float>& audio,
                                           const SpectrogramParams&   sp,
                                           const FingerprintParams&   fp) {
    const auto spec  = compute_spectrogram(audio, sp);
    const auto peaks = extract_peaks(spec, fp);
    return generate_fingerprints(peaks, fp);
}

} 
