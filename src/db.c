// File: src/db.c
// Database utility functions for song and fingerprint storage.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <sys/stat.h>
#include "db.h"

// Check if a file exists at the given path.
int db_file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Initialize (open or create) a SQLite database at the given path.
int db_init(sqlite3** db, const char* path) {
    if (!db_file_exists(path)) {
        printf("Database file does not exist. Creating a new one...\n");
    } else {
        printf("Opening existing database...\n");
    }
    return sqlite3_open(path, db);
}

// Close an open SQLite database.
void db_close(sqlite3* db) {
    if (db) sqlite3_close(db);
}

// Create required tables and indexes if they do not exist.
int db_create_tables(sqlite3* db) {
    char* err = NULL;

    const char* create_songs_sql =
        "CREATE TABLE IF NOT EXISTS Songs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "artist TEXT NOT NULL, "
        "UNIQUE(name, artist));";  // Ensure uniqueness on song name + artist

    const char* create_fingerprints_sql =
        "CREATE TABLE IF NOT EXISTS Fingerprints ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "hash TEXT NOT NULL, "
        "time_offset INTEGER NOT NULL, "
        "song_id INTEGER NOT NULL, "
        "FOREIGN KEY(song_id) REFERENCES Songs(id));";

    const char* create_index_sql =
        "CREATE INDEX IF NOT EXISTS idx_hash ON Fingerprints(hash);";

    if (sqlite3_exec(db, create_songs_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Songs table error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    if (sqlite3_exec(db, create_fingerprints_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Fingerprints table error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    if (sqlite3_exec(db, create_index_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Index creation error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

// Find a song by name and artist. Returns 1 if found, 0 if not, -1 on error.
int db_find_song(sqlite3* db, const char* name, const char* artist, int* song_id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM Songs WHERE name = ? AND artist = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *song_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return 1; // Found
    }

    sqlite3_finalize(stmt);
    return 0; // Not found
}

// Insert a new song if not already present. Returns 0 on insert, 1 if duplicate, -1 on error.
int db_insert_song(sqlite3* db, const char* name, const char* artist, int* song_id) {
    int found = db_find_song(db, name, artist, song_id);
    if (found == 1) {
        printf("Duplicate song detected: '%s' by '%s'. Using existing song_id: %d\n", name, artist, *song_id);
        return 1;
    } else if (found == -1) {
        fprintf(stderr, "Error checking for duplicate song.\n");
        return -1;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Songs (name, artist) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return -1;

    *song_id = (int)sqlite3_last_insert_rowid(db);
    return 0;
}

// Insert a fingerprint hash for a song. Returns 0 on success, -1 on error.
int db_insert_fingerprint(sqlite3* db, const char* hash, int time_offset, int song_id) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Fingerprints (hash, time_offset, song_id) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, time_offset);
    sqlite3_bind_int(stmt, 3, song_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}
