// audio_reader.cpp — libsndfile-based audio I/O

#include "audio_reader.hpp"
#include "spectrogram.hpp"  
#include <sndfile.h>
#include <stdexcept>

namespace shazam {

AudioData read_audio_file(const std::string& path, int target_sample_rate) {
    SF_INFO info{};
    SNDFILE* snd = sf_open(path.c_str(), SFM_READ, &info);
    if (!snd)
        throw std::runtime_error(
            std::string("Cannot open audio file '") + path + "': " +
            sf_strerror(nullptr));

    const int   src_rate  = info.samplerate;
    const int   channels  = info.channels;
    const sf_count_t total_frames = info.frames;

    // Read all interleaved samples
    std::vector<float> raw(static_cast<std::size_t>(total_frames * channels));
    const sf_count_t read = sf_readf_float(snd, raw.data(), total_frames);
    sf_close(snd);

    if (read != total_frames)
        throw std::runtime_error("Incomplete read from: " + path);

    // Mix to mono
    std::vector<float> mono;
    if (channels == 1) {
        mono = std::move(raw);
    } else if (channels == 2) {
        mono = stereo_to_mono(raw);
    } else {
        // General N-channel downmix
        mono.resize(static_cast<std::size_t>(total_frames));
        for (sf_count_t f = 0; f < total_frames; ++f) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += raw[static_cast<std::size_t>(f * channels + c)];
            mono[static_cast<std::size_t>(f)] = sum / channels;
        }
    }

    // Resample if needed
    if (src_rate != target_sample_rate)
        mono = resample(mono, src_rate, target_sample_rate);

    const double dur = static_cast<double>(total_frames) / src_rate;

    return AudioData{std::move(mono), target_sample_rate, dur, channels};
}

void write_wav(const std::string&        path,
               const std::vector<float>& samples,
               int                       sample_rate) {
    SF_INFO info{};
    info.samplerate = sample_rate;
    info.channels   = 1;
    info.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE* snd = sf_open(path.c_str(), SFM_WRITE, &info);
    if (!snd)
        throw std::runtime_error("Cannot create WAV: " + path);

    sf_writef_float(snd, samples.data(),
                    static_cast<sf_count_t>(samples.size()));
    sf_close(snd);
}

} 
