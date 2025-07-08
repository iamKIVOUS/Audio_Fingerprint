// File: src/hashing.c
// Audio fingerprint hash generation utilities.

#include <stdio.h>
#include <stdlib.h>
#include "hashing.h"
#include "types.h"
#include "config.h"

// --------------------
// Internal Hash Helper
// --------------------

/**
 * Generates a compact 32-bit fingerprint hash from anchor frequency, target frequency, and time delta.
 * Format: [f1:10 bits][f2:10 bits][delta_t:12 bits] = 32 bits
 */
static unsigned int generate_hash(int f1, int f2, int delta_t) {
    return ((f1 & 0x3FF) << 22) | ((f2 & 0x3FF) << 12) | (delta_t & 0xFFF);
}

// ---------------------------
// Fingerprint Generation API
// ---------------------------

/**
 * Generates fingerprint hashes using anchor-target pairing within a fan-out window.
 *
 * @param peaks            Array of detected spectral peaks.
 * @param num_peaks        Total number of peaks in the array.
 * @param song_id          ID of the song to associate with each hash.
 * @param num_hashes_out   Pointer to store the number of hashes generated.
 * @return FingerprintHash* Pointer to dynamically allocated hash array (must be freed by caller).
 */
FingerprintHash* generate_fingerprints(const Peak* peaks, int num_peaks, int song_id, int* num_hashes_out) {
    if (!peaks || num_peaks <= 0 || !num_hashes_out) {
        fprintf(stderr, "Error [generate_fingerprints]: Invalid input arguments.\n");
        return NULL;
    }

    int max_hashes = num_peaks * FAN_VALUE;
    FingerprintHash* hashes = (FingerprintHash*)malloc(max_hashes * sizeof(FingerprintHash));
    if (!hashes) {
        fprintf(stderr, "Error [generate_fingerprints]: Memory allocation failed for hashes.\n");
        return NULL;
    }

    int count = 0;

    for (int i = 0; i < num_peaks; i++) {
        int anchor_time = peaks[i].time_index;
        int anchor_freq = peaks[i].freq_bin;

        for (int j = 1; j <= FAN_VALUE; j++) {
            int target_idx = i + j;
            if (target_idx >= num_peaks)
                break;

            int target_time = peaks[target_idx].time_index;
            int delta_t = target_time - anchor_time;

            if (delta_t < HASH_TIME_DELTA_MIN || delta_t > HASH_TIME_DELTA_MAX)
                continue;

            int target_freq = peaks[target_idx].freq_bin;

            FingerprintHash fp;
            fp.hash = generate_hash(anchor_freq, target_freq, delta_t);
            fp.time_offset = anchor_time;
            fp.song_id = song_id;

            hashes[count++] = fp;

            // Optional: Resize if unexpectedly many hashes
            if (count >= max_hashes) {
                max_hashes *= 2;
                FingerprintHash* temp = (FingerprintHash*)realloc(hashes, max_hashes * sizeof(FingerprintHash));
                if (!temp) {
                    fprintf(stderr, "Error [generate_fingerprints]: Realloc failed at %d hashes.\n", count);
                    free(hashes);
                    return NULL;
                }
                hashes = temp;
            }
        }
    }

    *num_hashes_out = count;
    return hashes;
}
