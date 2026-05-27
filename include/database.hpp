#pragma once
// database.hpp, SQLite-backed fingerprint database
// Schema
//   songs(id INTEGER PK, title TEXT, path TEXT, duration_s REAL)
//   fingerprints(hash INTEGER, song_id INTEGER, time_offset INTEGER)
//   INDEX on fingerprints.hash  for O(1) lookup

#include "fingerprint.hpp"
#include <string>
#include <optional>
#include <vector>

struct sqlite3;

namespace shazam {

struct SongInfo {
    int         id;
    std::string title;
    std::string path;
    double      duration_s;
};

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    /// Register a new song and store all its fingerprints.
    /// Returns the assigned song_id.
    int register_song(const std::string&             title,
                      const std::string&             path,
                      double                         duration_s,
                      const std::vector<Fingerprint>& fps);

    /// Look up all (song_id, time_offset) entries matching a given hash.
    struct Match { int song_id; int time_offset; };
    std::vector<Match> lookup(uint32_t hash) const;

    /// Fetch metadata for a song by id.
    std::optional<SongInfo> get_song(int song_id) const;

    /// List all registered songs.
    std::vector<SongInfo> list_songs() const;

    /// Delete a song and all its fingerprints.
    void remove_song(int song_id);

    /// Number of fingerprints in the database.
    std::size_t fingerprint_count() const;

private:
    sqlite3*    db_ = nullptr;
    void        exec(const std::string& sql) const;
    void        check(int rc, const std::string& ctx) const;
    void        create_schema();
};

} 
