// test_roundtrip.cpp: in-memory register -> query -> identify

#include "fingerprint.hpp"
#include "database.hpp"
#include "matcher.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>

static constexpr float PI = 3.14159265358979f;

static std::vector<float> make_song(int sample_rate, float dur_s,
                                    std::initializer_list<float> freqs) {
    const int N = static_cast<int>(sample_rate * dur_s);
    std::vector<float> audio(N, 0.f);
    for (float f : freqs)
        for (int n = 0; n < N; ++n)
            audio[n] += std::sin(2.f * PI * f * n / sample_rate);
    float mx = 0;
    for (float v : audio) mx = std::max(mx, std::abs(v));
    if (mx > 0) for (auto& s : audio) s /= mx;
    return audio;
}

static std::vector<float> add_noise(const std::vector<float>& clean, float sigma) {
    std::srand(42u);
    std::vector<float> noisy = clean;
    for (auto& s : noisy)
        s += sigma * (2.f * std::rand() / RAND_MAX - 1.f);
    return noisy;
}

int main() {
    std::cout << "=== Roundtrip Test ===\n";

    const int SR = 44100;
    const std::string db_file = "/tmp/shazam_test.db";
    std::filesystem::remove(db_file);

    auto song_a = make_song(SR, 10.f, {440.f, 880.f, 1320.f, 2000.f, 2640.f});
    auto song_b = make_song(SR, 10.f, {300.f, 600.f, 900.f,  1500.f, 3000.f});

    shazam::FingerprintParams fp;
    fp.min_magnitude = 1.0f;

    shazam::Database db(db_file);

    const auto fps_a = shazam::fingerprint_audio(song_a, {}, fp);
    const auto fps_b = shazam::fingerprint_audio(song_b, {}, fp);

    assert(!fps_a.empty() && !fps_b.empty());

    const int id_a = db.register_song("Song A", "", 10.0, fps_a);
    const int id_b = db.register_song("Song B", "", 10.0, fps_b);

    std::cout << "  Registered: Song A (id=" << id_a << ", "
              << fps_a.size() << " fps)  "
              << "Song B (id=" << id_b << ", " << fps_b.size() << " fps)\n";

    std::vector<float> snippet_a(song_a.begin() + 2 * SR,
                                 song_a.begin() + 7 * SR);
    auto result = shazam::identify(shazam::fingerprint_audio(snippet_a, {}, fp), db);

    assert(result.has_value());
    assert(result->song_id == id_a);
    std::cout << "  [PASS] clean snippet → matched \"" << result->title << "\""
              << "  (votes=" << result->peak_count << ")\n";

    std::vector<float> snippet_b(song_b.begin() + SR,
                                 song_b.begin() + 6 * SR);
    auto noisy_b = add_noise(snippet_b, 0.01f);  // 1% amplitude noise
    auto result_b = shazam::identify(shazam::fingerprint_audio(noisy_b, {}, fp), db);

    assert(result_b.has_value());
    assert(result_b->song_id == id_b);
    std::cout << "  [PASS] noisy snippet  → matched \"" << result_b->title << "\""
              << "  (votes=" << result_b->peak_count << ")\n";

    std::filesystem::remove(db_file);
    std::cout << "All roundtrip tests passed.\n";
}
