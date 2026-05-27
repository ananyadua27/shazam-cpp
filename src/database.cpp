// database.cpp — SQLite fingerprint store

#include "database.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>

namespace shazam {

void Database::check(int rc, const std::string& ctx) const {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error("SQLite error in " + ctx + ": " +
                                 sqlite3_errmsg(db_));
    }
}

void Database::exec(const std::string& sql) const {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        throw std::runtime_error("SQLite exec: " + msg + "\nSQL: " + sql);
    }
}

void Database::create_schema() {
    exec(R"sql(
        CREATE TABLE IF NOT EXISTS songs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            title       TEXT    NOT NULL,
            path        TEXT,
            duration_s  REAL    DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS fingerprints (
            hash        INTEGER NOT NULL,
            song_id     INTEGER NOT NULL,
            time_offset INTEGER NOT NULL,
            FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_fp_hash
            ON fingerprints(hash);
    )sql");

    // WAL mode: faster bulk inserts during registration
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA synchronous=NORMAL;");
    exec("PRAGMA foreign_keys=ON;");
}

Database::Database(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    check(rc, "open");
    create_schema();
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

int Database::register_song(const std::string&             title,
                            const std::string&             path,
                            double                         duration_s,
                            const std::vector<Fingerprint>& fps) {
    sqlite3_stmt* stmt = nullptr;
    const char* insert_song =
        "INSERT INTO songs(title, path, duration_s) VALUES(?,?,?)";
    check(sqlite3_prepare_v2(db_, insert_song, -1, &stmt, nullptr),
          "prepare insert_song");
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, path.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, duration_s);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    const int song_id = static_cast<int>(sqlite3_last_insert_rowid(db_));

    exec("BEGIN;");
    const char* insert_fp =
        "INSERT INTO fingerprints(hash, song_id, time_offset) VALUES(?,?,?)";
    check(sqlite3_prepare_v2(db_, insert_fp, -1, &stmt, nullptr),
          "prepare insert_fp");

    for (const auto& fp : fps) {
        sqlite3_bind_int(stmt, 1, static_cast<int>(fp.hash));
        sqlite3_bind_int(stmt, 2, song_id);
        sqlite3_bind_int(stmt, 3, fp.time_offset);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    exec("COMMIT;");

    return song_id;
}


std::vector<Database::Match> Database::lookup(uint32_t hash) const {
    sqlite3_stmt* stmt = nullptr;
    const char* q = "SELECT song_id, time_offset FROM fingerprints WHERE hash=?";
    check(sqlite3_prepare_v2(db_, q, -1, &stmt, nullptr), "prepare lookup");
    sqlite3_bind_int(stmt, 1, static_cast<int>(hash));

    std::vector<Match> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back({
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1)
        });
    }
    sqlite3_finalize(stmt);
    return results;
}

std::optional<SongInfo> Database::get_song(int song_id) const {
    sqlite3_stmt* stmt = nullptr;
    const char* q = "SELECT id,title,path,duration_s FROM songs WHERE id=?";
    check(sqlite3_prepare_v2(db_, q, -1, &stmt, nullptr), "prepare get_song");
    sqlite3_bind_int(stmt, 1, song_id);

    std::optional<SongInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = SongInfo{
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            sqlite3_column_double(stmt, 3)
        };
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SongInfo> Database::list_songs() const {
    sqlite3_stmt* stmt = nullptr;
    const char* q = "SELECT id,title,path,duration_s FROM songs ORDER BY id";
    check(sqlite3_prepare_v2(db_, q, -1, &stmt, nullptr), "prepare list_songs");

    std::vector<SongInfo> songs;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        songs.push_back({
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            sqlite3_column_double(stmt, 3)
        });
    }
    sqlite3_finalize(stmt);
    return songs;
}

void Database::remove_song(int song_id) {
    sqlite3_stmt* stmt = nullptr;
    const char* q = "DELETE FROM songs WHERE id=?";
    check(sqlite3_prepare_v2(db_, q, -1, &stmt, nullptr), "prepare remove_song");
    sqlite3_bind_int(stmt, 1, song_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::size_t Database::fingerprint_count() const {
    sqlite3_stmt* stmt = nullptr;
    const char* q = "SELECT COUNT(*) FROM fingerprints";
    check(sqlite3_prepare_v2(db_, q, -1, &stmt, nullptr), "prepare count");
    sqlite3_step(stmt);
    auto n = static_cast<std::size_t>(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);
    return n;
}

} 
