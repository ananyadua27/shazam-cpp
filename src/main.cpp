// main.cpp — shazam-cpp command-line interface
// Usage:
//   shazam register <audio_file> [--title "Song Name"] [--db shazam.db]
//   shazam query    <audio_file> [--db shazam.db]
//   shazam list                  [--db shazam.db]
//   shazam remove   <song_id>    [--db shazam.db]
//   shazam stats                 [--db shazam.db]

#include "audio_reader.hpp"
#include "fingerprint.hpp"
#include "database.hpp"
#include "matcher.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

struct Args {
    std::string command;
    std::string audio_file;
    std::string db_path   = "shazam.db";
    std::string title;
    int         song_id   = -1;
};

static void print_usage(const char* prog) {
    std::cerr <<
        "Usage:\n"
        "  " << prog << " register <audio_file> [--title \"Song Name\"] [--db FILE]\n"
        "  " << prog << " query    <audio_file> [--db FILE]\n"
        "  " << prog << " list                  [--db FILE]\n"
        "  " << prog << " remove   <song_id>    [--db FILE]\n"
        "  " << prog << " stats                 [--db FILE]\n";
}

static Args parse_args(int argc, char** argv) {
    Args a;
    if (argc < 2) { print_usage(argv[0]); std::exit(1); }
    a.command = argv[1];

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc)
            a.db_path = argv[++i];
        else if (std::strcmp(argv[i], "--title") == 0 && i + 1 < argc)
            a.title = argv[++i];
        else if (argv[i][0] != '-') {
            if (a.audio_file.empty())
                a.audio_file = argv[i];
            else
                a.song_id = std::stoi(argv[i]);
        }
    }
    return a;
}

static int cmd_register(const Args& args) {
    if (args.audio_file.empty()) {
        std::cerr << "register: audio file required\n";
        return 1;
    }

    const std::string title = args.title.empty()
        ? fs::path(args.audio_file).stem().string()
        : args.title;

    std::cout << "Reading:       " << args.audio_file << "\n";
    auto t0 = std::chrono::steady_clock::now();

    const auto audio = shazam::read_audio_file(args.audio_file);

    std::cout << "Duration:      " << std::fixed << std::setprecision(2)
              << audio.duration_s << " s  ("
              << audio.sample_rate << " Hz, "
              << audio.channels   << " ch → mono)\n";

    std::cout << "Fingerprinting …\n";
    const auto fps = shazam::fingerprint_audio(audio.samples);

    std::cout << "Fingerprints:  " << fps.size() << "\n";

    shazam::Database db(args.db_path);
    const int id = db.register_song(title, args.audio_file, audio.duration_s, fps);

    auto t1 = std::chrono::steady_clock::now();
    const double elapsed =
        std::chrono::duration<double>(t1 - t0).count();

    std::cout << "Registered:    \"" << title << "\"  (id=" << id << ")\n";
    std::cout << "Elapsed:       " << std::fixed << std::setprecision(2)
              << elapsed << " s\n";
    return 0;
}

static int cmd_query(const Args& args) {
    if (args.audio_file.empty()) {
        std::cerr << "query: audio file required\n";
        return 1;
    }

    std::cout << "Reading:       " << args.audio_file << "\n";
    auto t0 = std::chrono::steady_clock::now();

    const auto audio = shazam::read_audio_file(args.audio_file);
    std::cout << "Duration:      " << std::fixed << std::setprecision(2)
              << audio.duration_s << " s\n";

    std::cout << "Fingerprinting …\n";
    const auto fps = shazam::fingerprint_audio(audio.samples);
    std::cout << "Fingerprints:  " << fps.size() << "\n";

    std::cout << "Searching …\n";
    const shazam::Database db(args.db_path);
    const auto result = shazam::identify(fps, db);

    auto t1 = std::chrono::steady_clock::now();
    const double elapsed =
        std::chrono::duration<double>(t1 - t0).count();

    if (result) {
        std::cout << "\n✓  Matched: \"" << result->title << "\"\n"
                  << "   Song ID:    " << result->song_id    << "\n"
                  << "   Votes:      " << result->peak_count << "\n"
                  << "   Confidence: " << std::fixed << std::setprecision(4)
                                       << result->confidence << "\n"
                  << "   Offset:     " << result->offset_frames << " frames\n";
    } else {
        std::cout << "\n✗  No match found.\n";
    }

    std::cout << "\nElapsed:       " << std::fixed << std::setprecision(2)
              << elapsed << " s\n";
    return result ? 0 : 2;
}

static int cmd_list(const Args& args) {
    const shazam::Database db(args.db_path);
    const auto songs = db.list_songs();

    if (songs.empty()) {
        std::cout << "(database is empty)\n";
        return 0;
    }

    std::cout << std::left
              << std::setw(6)  << "ID"
              << std::setw(40) << "Title"
              << std::setw(10) << "Duration"
              << "Path\n"
              << std::string(80, '-') << "\n";

    for (const auto& s : songs) {
        std::cout << std::setw(6)  << s.id
                  << std::setw(40) << s.title
                  << std::setw(10) << (std::to_string(static_cast<int>(s.duration_s)) + "s")
                  << s.path << "\n";
    }
    return 0;
}

static int cmd_remove(const Args& args) {
    if (args.song_id < 0) {
        std::cerr << "remove: song_id required\n";
        return 1;
    }
    shazam::Database db(args.db_path);
    const auto info = db.get_song(args.song_id);
    if (!info) {
        std::cerr << "No song with id=" << args.song_id << "\n";
        return 1;
    }
    db.remove_song(args.song_id);
    std::cout << "Removed: \"" << info->title << "\" (id=" << args.song_id << ")\n";
    return 0;
}

static int cmd_stats(const Args& args) {
    const shazam::Database db(args.db_path);
    const auto songs = db.list_songs();
    std::cout << "Songs:         " << songs.size()              << "\n"
              << "Fingerprints:  " << db.fingerprint_count()    << "\n"
              << "Database:      " << args.db_path              << "\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);

        if      (args.command == "register") return cmd_register(args);
        else if (args.command == "query")    return cmd_query(args);
        else if (args.command == "list")     return cmd_list(args);
        else if (args.command == "remove")   return cmd_remove(args);
        else if (args.command == "stats")    return cmd_stats(args);
        else {
            std::cerr << "Unknown command: " << args.command << "\n";
            print_usage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
