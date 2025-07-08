/*#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Initialize database
int db_init(sqlite3** db, const char* path);

// Check if file exists
int db_file_exists(const char* path);

// Close database
void db_close(sqlite3* db);

// Create tables
int db_create_tables(sqlite3* db);

// Insert new song (checks for duplicates)
int db_insert_song(sqlite3* db, const char* name, const char* artist, int* song_id);

// Find existing song by name + artist
int db_find_song(sqlite3* db, const char* name, const char* artist, int* song_id);

// Insert fingerprint
int db_insert_fingerprint(sqlite3* db, const char* hash, int time_offset, int song_id);

#endif
*/
// File: include/db.h

#ifndef DB_H
#define DB_H

int db_open(const char* path);        // Open or create DB and ensure tables exist
void db_close();                      // Close DB

int db_create_tables();               // Create tables if not exist
int db_find_song(const char* name, const char* artist, int* song_id);
int db_insert_song(const char* name, const char* artist, int* song_id);
int db_insert_fingerprint(const char* hash, int time_offset, int song_id);

#endif
