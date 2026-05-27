// matcher.cpp — offset histogram identification

#include "matcher.hpp"
#include <unordered_map>
#include <map>
#include <algorithm>

namespace shazam {

std::optional<MatchResult> identify(const std::vector<Fingerprint>& query_fps,
                                    const Database&                  db,
                                    const MatcherParams&             params) {
    if (query_fps.empty()) return std::nullopt;

    // histogram[song_id][delta_t] = vote count
    // Using nested unordered_maps for O(1) average insertion and lookup.
    std::unordered_map<int, std::unordered_map<int, int>> histogram;

    int total_matches = 0;

    for (const auto& qfp : query_fps) {
        // Look up this hash in the database — hits the indexed fingerprints table
        const auto matches = db.lookup(qfp.hash);
        for (const auto& m : matches) {
            const int delta = m.time_offset - qfp.time_offset;
            histogram[m.song_id][delta]++;
            ++total_matches;
        }
    }

    if (histogram.empty()) return std::nullopt;

    // Find the (song_id, delta) pair with the most votes — the histogram spike.
    int best_song   = -1;
    int best_delta  = 0;
    int best_votes  = 0;

    for (const auto& [song_id, deltas] : histogram) {
        for (const auto& [delta, votes] : deltas) {
            if (votes > best_votes) {
                best_votes = votes;
                best_delta = delta;
                best_song  = song_id;
            }
        }
    }

    if (best_votes < params.min_votes) return std::nullopt;

    const double confidence =
        static_cast<double>(best_votes) / static_cast<double>(query_fps.size());

    if (confidence < params.min_confidence) return std::nullopt;

    const auto song_info = db.get_song(best_song);
    if (!song_info) return std::nullopt;

    return MatchResult{
        best_song,
        song_info->title,
        confidence,
        best_votes,
        best_delta
    };
}

std::optional<MatchResult> identify_audio(const std::vector<float>& audio,
                                          const Database&            db,
                                          const SpectrogramParams&   sp,
                                          const FingerprintParams&   fp,
                                          const MatcherParams&       mp) {
    const auto fps = fingerprint_audio(audio, sp, fp);
    return identify(fps, db, mp);
}

} 
