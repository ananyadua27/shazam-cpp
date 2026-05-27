#pragma once
// Supports format libsndfile handles: WAV, AIFF, FLAC, OGG.
//
// Output:
//   - mono (averaged channels)
//   - float32 samples in [-1, 1]
//   - resampled to target_sample_rate if the file differs

#include <vector>
#include <string>

namespace shazam {

struct AudioData {
    std::vector<float> samples;    // mono, float32
    int                sample_rate;
    double             duration_s;
    int                channels;   
};

AudioData read_audio_file(const std::string& path, int target_sample_rate = 44100);
void write_wav(const std::string&        path,
               const std::vector<float>& samples,
               int                       sample_rate);

} 
