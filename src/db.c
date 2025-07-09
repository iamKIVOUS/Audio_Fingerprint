// File: src/db.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "sqlite3.h"
#include "db.h"

static sqlite3* db = NULL;  // Global database connection

static int db_file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

int db_open(const char* path) {
    if (db_file_exists(path)) {
        printf("Opening existing database: %s\n", path);
    } else {
        printf("Creating new database: %s\n", path);
    }

    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open DB: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return db_create_tables();  // Ensure tables exist
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

int db_create_tables() {
    const char* songs_sql =
        "CREATE TABLE IF NOT EXISTS Songs ("
        "id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "artist TEXT NOT NULL, "
        "UNIQUE(name, artist));";

    const char* fingerprints_sql =
        "CREATE TABLE IF NOT EXISTS Fingerprints ("
        "id INTEGER PRIMARY KEY, "
        "hash TEXT NOT NULL, "
        "time_offset INTEGER NOT NULL, "
        "song_id INTEGER NOT NULL, "
        "FOREIGN KEY(song_id) REFERENCES Songs(id), "
        "UNIQUE(hash, time_offset, song_id)); ";

    const char* index_sql =
        "CREATE INDEX IF NOT EXISTS idx_hash ON Fingerprints(hash);";

    char* err = NULL;

    if (sqlite3_exec(db, songs_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Error creating Songs table: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    if (sqlite3_exec(db, fingerprints_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Error creating Fingerprints table: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    if (sqlite3_exec(db, index_sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "Error creating index: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

int db_find_song(const char* name, const char* artist, int* song_id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM Songs WHERE name = ? AND artist = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *song_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_insert_song(const char* name, const char* artist, int* song_id) {
    int found = db_find_song(name, artist, song_id);
    if (found == 1) {
        printf("Duplicate song: '%s' by '%s', ID=%d\n", name, artist, *song_id);
        return 1;
    } else if (found == -1) {
        return -1;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Songs (name, artist) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    *song_id = (int)sqlite3_last_insert_rowid(db);
    return 0;
}

int db_insert_fingerprint(const char* hash, int time_offset, int song_id) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT OR IGNORE INTO Fingerprints (hash, time_offset, song_id) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, time_offset);
    sqlite3_bind_int(stmt, 3, song_id);

    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);  // Get number of rows actually inserted
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE && changes > 0) ? 0 : 1;  // 0 = inserted, 1 = duplicate ignored
}
