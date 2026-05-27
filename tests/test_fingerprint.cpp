// test_fingerprint.cpp — fingerprinting pipeline unit tests

#include "fingerprint.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <numeric>
#include <algorithm>

static constexpr float PI = 3.14159265358979f;

// Synthesise a clean tone: sum of sinusoids
static std::vector<float> make_tone(int sample_rate, float duration_s,
                                    std::initializer_list<float> freqs) {
    const int N = static_cast<int>(sample_rate * duration_s);
    std::vector<float> audio(N, 0.f);
    for (float f : freqs)
        for (int n = 0; n < N; ++n)
            audio[n] += std::sin(2.f * PI * f * n / sample_rate);
    // Normalise
    float mx = *std::max_element(audio.begin(), audio.end());
    if (mx > 0.f) for (auto& s : audio) s /= mx;
    return audio;
}

static void test_spectrogram_shape() {
    const auto audio = make_tone(44100, 3.0f, {440.f, 880.f, 1320.f});
    shazam::SpectrogramParams sp;
    const auto spec = shazam::compute_spectrogram(audio, sp);

    // Should have (max_freq - min_freq) rows
    assert(spec.size() == static_cast<std::size_t>(sp.max_freq_bin - sp.min_freq_bin));

    // Each row should have the right number of frames
    const int expected_frames = (static_cast<int>(audio.size()) - sp.window_size) / sp.hop_size + 1;
    for (const auto& row : spec) {
        (void)row;
        assert(static_cast<int>(row.size()) == expected_frames);
    }
    (void)expected_frames;

    std::cout << "  [PASS] spectrogram shape: "
              << spec.size() << " bins × " << spec[0].size() << " frames\n";
}

static void test_peaks_not_empty() {
    const auto audio = make_tone(44100, 3.0f, {440.f, 880.f, 1320.f, 2000.f});
    shazam::SpectrogramParams sp;
    shazam::FingerprintParams fp;
    fp.min_magnitude = 1.0f;  // lower threshold for synthetic tone

    const auto spec  = shazam::compute_spectrogram(audio, sp);
    const auto peaks = shazam::extract_peaks(spec, fp);

    assert(!peaks.empty());
    std::cout << "  [PASS] peak extraction: " << peaks.size() << " peaks found\n";
}

static void test_fingerprints_not_empty() {
    const auto audio = make_tone(44100, 5.0f, {440.f, 880.f, 1320.f, 2000.f, 3000.f});
    shazam::FingerprintParams fp;
    fp.min_magnitude = 1.0f;

    const auto fps = shazam::fingerprint_audio(audio, {}, fp);
    assert(!fps.empty());
    std::cout << "  [PASS] fingerprinting: " << fps.size() << " fingerprints generated\n";
}

static void test_hash_encoding() {
    // encode_hash must be invertible for the values we pack
    const int f1 = 123, f2 = 456 % 512, dt = 50;
    const uint32_t h = shazam::encode_hash(f1, f2, dt);

    assert(((h >> 23) & 0x1FF) == (uint32_t)f1);
    assert(((h >> 14) & 0x1FF) == (uint32_t)f2);
    assert(((h)       & 0x3FFF)== (uint32_t)dt);
    std::cout << "  [PASS] hash encoding/decoding roundtrip\n";
}

int main() {
    std::cout << "=== Fingerprint Tests ===\n";
    test_spectrogram_shape();
    test_peaks_not_empty();
    test_fingerprints_not_empty();
    test_hash_encoding();
    std::cout << "All fingerprint tests passed.\n";
}
