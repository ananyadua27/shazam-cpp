#pragma once
// matcher.hpp, Offset-histogram song identification
// The matching algorithm (Wang 2003 §4) works as follows:
//   For each query fingerprint (hash, t_q):
//     For each database entry with the same hash (song_id, t_db):
//       Δ = t_db - t_q   ← "time alignment offset"
//       increment histogram[song_id][Δ]
//
//   The song whose histogram has the tallest peak wins.

#include "database.hpp"
#include "fingerprint.hpp"
#include <string>
#include <optional>

namespace shazam {
struct MatchResult {
    int         song_id;
    std::string title;
    double      confidence;    // peak_count / total_matching_hashes (0–1)
    int         peak_count;    // votes at the winning offset
    int         offset_frames; // time alignment (anchor frame of match)
};

struct MatcherParams {
    int   min_votes       = 5;    // discard results with fewer histogram votes
    float min_confidence  = 0.01f; // fraction of query hashes that aligned
};

/// Identify a query audio clip against the database.
std::optional<MatchResult> identify(const std::vector<Fingerprint>& query_fps,
                                    const Database&                  db,
                                    const MatcherParams&             params = {});

std::optional<MatchResult> identify_audio(const std::vector<float>& audio,
                                          const Database&            db,
                                          const SpectrogramParams&   sp = {},
                                          const FingerprintParams&   fp = {},
                                          const MatcherParams&       mp = {});

} 
