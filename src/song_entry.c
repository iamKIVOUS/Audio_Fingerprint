// File: src/song_entry.c
// Song database entry and fingerprinting pipeline.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "sqlite3.h"
#include "config.h"
#include "audio_io.h"
#include "spectrogram.h"
#include "peak_detection.h"
#include "hashing.h"
#include "db.h"
#include "types.h"

// Process a single song file: build spectrogram, detect peaks, generate hashes, and store in DB.
void process_song_file(const char* filepath, sqlite3* songs_db, sqlite3* fingerprints_db) {
    float** spectrogram = NULL;
    int num_frames = 0, num_bins = 0;

    // Build spectrogram from file
    if (build_spectrogram(filepath, &spectrogram, &num_frames, &num_bins) != 0) {
        fprintf(stderr, "Failed to build spectrogram for %s\n", filepath);
        return;
    }

    // Extract song name and artist (basic filename parsing: artist-title.wav)
    const char* filename = strrchr(filepath, '/');
    if (!filename) filename = filepath;
    else filename++;  // skip '/'

    char name[256] = {0}, artist[256] = {0};
    sscanf(filename, "%[^-]-%[^.]", artist, name);

    // Insert song into songs.db and get song_id
    int song_id = -1;
    if (db_insert_song(songs_db, name, artist, &song_id) != 0 || song_id < 0) {
        fprintf(stderr, "Failed to insert/find song in database: %s - %s\n", artist, name);
        // Clean up
        for (int i = 0; i < num_frames; i++) free(spectrogram[i]);
        free(spectrogram);
        return;
    }

    // Peak Detection
    int num_peaks = 0;
    Peak* peaks = detect_peaks(spectrogram, num_frames, num_bins, &num_peaks);
    if (!peaks || num_peaks == 0) {
        fprintf(stderr, "No peaks found in %s\n", filepath);
        goto cleanup;
    }

    // Hashing
    int num_hashes = 0;
    FingerprintHash* hashes = generate_fingerprints(peaks, num_peaks, song_id, &num_hashes);
    if (!hashes || num_hashes == 0) {
        fprintf(stderr, "No hashes generated for %s\n", filepath);
        goto cleanup;
    }

    // Insert hashes into fingerprint DB
    for (int i = 0; i < num_hashes; i++) {
        char hash_str[16];
        snprintf(hash_str, sizeof(hash_str), "%08X", hashes[i].hash);
        if (db_insert_fingerprint(fingerprints_db, hash_str, hashes[i].time_offset, hashes[i].song_id) != 0) {
            fprintf(stderr, "Failed to insert hash %s\n", hash_str);
        }
    }

    printf("Processed %s: %d peaks, %d hashes\n", filepath, num_peaks, num_hashes);

cleanup:
    for (int i = 0; i < num_frames; i++) free(spectrogram[i]);
    free(spectrogram);
    free(peaks);
    free(hashes);
}

// Main entry point: processes all WAV files in the songs folder and stores fingerprints in DB.
int main() {
    // Open songs DB
    sqlite3* songs_db = NULL;
    if (db_init(&songs_db, SONGS_DB_PATH) != 0) return 1;
    db_create_tables(songs_db);

    // Open fingerprint DB
    sqlite3* fingerprints_db = NULL;
    if (db_init(&fingerprints_db, FINGERPRINTS_DB_PATH) != 0) {
        db_close(songs_db);
        return 1;
    }
    db_create_tables(fingerprints_db);

    // Read all WAV files from songs folder
    DIR* dir = opendir(SONGS_FOLDER);
    if (!dir) {
        perror("opendir");
        db_close(songs_db);
        db_close(fingerprints_db);
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".wav")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s%s", SONGS_FOLDER, entry->d_name);
            process_song_file(filepath, songs_db, fingerprints_db);
        }
    }

    closedir(dir);
    db_close(songs_db);
    db_close(fingerprints_db);

    return 0;
}
