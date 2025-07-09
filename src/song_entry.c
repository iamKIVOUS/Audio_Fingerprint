// File: src/song_entry.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "audio_io.h"
#include "spectrogram.h"
#include "peak_detection.h"
#include "hashing.h"
#include "types.h"
#include "config.h"
#include "db.h"

#define MAX_PATH_LEN 1024

int is_audio_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    return ext && (strcmp(ext, ".wav") == 0 || strcmp(ext, ".mp3") == 0);
}

void process_file(const char* filepath, const char* filename) {
    const char* song_name = filename;
    const char* artist_name = "Unknown";  // Default, can be improved later

    int song_id = -1;
    if (db_insert_song(song_name, artist_name, &song_id) < 0) {
        fprintf(stderr, "Failed to insert song into database.\n");
        return;
    }

    // If duplicate, skip
    if (song_id == -1) {
        printf("Skipping duplicate song: %s\n", song_name);
        return;
    }

    printf("Processing: %s\n", filepath);

    float** spectrogram = NULL;
    int num_frames = 0, num_bins = 0;

    if (build_spectrogram(filepath, &spectrogram, &num_frames, &num_bins) != 0 || !spectrogram) {
        fprintf(stderr, "Spectrogram generation failed for: %s\n", filepath);
        return;
    }

    int num_peaks = 0;
    Peak* peaks = detect_peaks(spectrogram, num_frames, num_bins, &num_peaks);
    if (!peaks) {
        fprintf(stderr, "Peak detection failed.\n");
        goto cleanup;
    }

    printf("Detected %d peaks.\n", num_peaks);

    int hash_count = 0;
    FingerprintHash64* hashes = generate_fingerprint_hashes(peaks, num_peaks, song_id, &hash_count);
    if (!hashes || hash_count == 0) {
        fprintf(stderr, "Hash generation failed or returned zero hashes.\n");
        goto cleanup;
    }

    printf("Generated %d hashes. Inserting into DB...\n", hash_count);

    int inserted = 0, skipped = 0;
    for (int i = 0; i < hash_count; i++) {
        char hex_hash[17];
        snprintf(hex_hash, sizeof(hex_hash), "%016lX", hashes[i].hash);
        int status = db_insert_fingerprint(hex_hash, hashes[i].time_offset, hashes[i].song_id);
        if (status == 0)
            inserted++;
        else if (status == 1)
            skipped++;
        else
            fprintf(stderr, "Failed to insert hash %d into DB.\n", i);
    }

    printf("Inserted %d/%d hashes (%d duplicates skipped).\n", inserted, hash_count, skipped);

cleanup:
    if (peaks) free(peaks);
    if (hashes) free(hashes);
    if (spectrogram) {
        for (int i = 0; i < num_frames; i++) free(spectrogram[i]);
        free(spectrogram);
    }
}

int main() {
    if (db_open(DB_PATH) != 0) {
        fprintf(stderr, "Failed to open/create DB at %s\n", DB_PATH);
        return 1;
    }

    DIR* dir = opendir(SONGS_FOLDER);
    if (!dir) {
        perror("Failed to open songs folder");
        db_close();
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        char filepath[MAX_PATH_LEN];
        snprintf(filepath, MAX_PATH_LEN, "%s%s", SONGS_FOLDER, entry->d_name);

        struct stat path_stat;
        if (stat(filepath, &path_stat) == 0 && S_ISREG(path_stat.st_mode) && is_audio_file(entry->d_name)) {
            process_file(filepath, entry->d_name);
        }
    }

    closedir(dir);
    db_close();
    return 0;
}
