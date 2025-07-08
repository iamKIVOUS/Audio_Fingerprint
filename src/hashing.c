// File: src/hashing.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hashing.h"
#include "config.h"


// Quantize magnitude in dB (assumes peaks[].magnitude already in dB)
static inline uint8_t quantize_mag(float mag_db) {
    if (mag_db < 0) mag_db = 0;
    if (mag_db > 60) mag_db = 60;
    return (uint8_t)((mag_db / 60.0f) * 255.0f);
}

// Signed 6-bit encoding: [-32, +31] → [0, 63]
static inline int encode_delta_freq(int df) {
    return df & 0x3F;
}

// Create a 64-bit hash with optimized bit allocation
static uint64_t generate_hash64(int a_freq, int delta_f, int dt, int a_time, uint8_t mag_q) {
    return  (((uint64_t)(a_freq   & 0x3FF))   << 54) |  // bits 63–54: anchor freq (10 bits)
            (((uint64_t)(delta_f  & 0x3F))    << 48) |  // bits 53–48: delta freq (6 bits)
            (((uint64_t)(dt       & 0xFFF))   << 36) |  // bits 47–36: delta time (12 bits)
            (((uint64_t)(mag_q    & 0xFF))    << 28) |  // bits 35–28: magnitude byte (8 bits)
            (((uint64_t)(a_time   & 0xFFFFF)) << 8);    // bits 27–8: anchor time (20 bits)
            // bits 7–0: reserved (unused)
}

FingerprintHash64* generate_fingerprint_hashes(const Peak* peaks,
                                              int num_peaks,
                                              int song_id,
                                              int* out_count) {
    if (!peaks || num_peaks <= 0 || !out_count) {
        fprintf(stderr, "Error: invalid input to generate_fingerprint_hashes\n");
        return NULL;
    }

    int capacity = num_peaks * FAN_VALUE;
    FingerprintHash64* list = malloc(capacity * sizeof(*list));
    if (!list) {
        perror("malloc");
        return NULL;
    }

    int n = 0;
    for (int i = 0; i < num_peaks; ++i) {
        int af = peaks[i].freq_bin;
        int at = peaks[i].time_index;
        float am = peaks[i].magnitude;
        uint8_t aq = quantize_mag(am);

        if (af > MAX_FREQ_BIN || at > MAX_TIME) continue;

        for (int j = 1; j <= FAN_VALUE; ++j) {
            int k = i + j;
            if (k >= num_peaks) break;

            int tf = peaks[k].freq_bin;
            int tt = peaks[k].time_index;
            int dt = tt - at;
            float tm = peaks[k].magnitude;
            uint8_t tq = quantize_mag(tm);

            if (dt <= 0 || dt > MAX_TIME_DELTA) continue;
            if (tf > MAX_FREQ_BIN) continue;

            int df = tf - af;
            if (df < -32 || df > 31) continue;  // signed 6-bit range check

            int df_encoded = encode_delta_freq(df);

            // Pack both magnitudes into a byte: high nibble = anchor, low = target
            uint8_t mag_byte = ((aq >> 4) << 4) | ((tq >> 4) & 0x0F);

            uint64_t h = generate_hash64(af, df_encoded, dt, at, mag_byte);
            list[n++] = (FingerprintHash64){ .hash = h,
                                             .time_offset = at,
                                             .song_id = song_id };

            if (n >= capacity) {
                capacity *= 2;
                FingerprintHash64* tmp = realloc(list, capacity * sizeof(*list));
                if (!tmp) {
                    perror("realloc");
                    free(list);
                    return NULL;
                }
                list = tmp;
            }
        }
    }

    // Deduplicate (in-place): same hash + time_offset
    int w = 0;
    for (int r = 0; r < n; ++r) {
        uint64_t h = list[r].hash;
        int to = list[r].time_offset;
        int dup = 0;
        for (int t = 0; t < w; ++t) {
            if (list[t].hash == h && list[t].time_offset == to) {
                dup = 1;
                break;
            }
        }
        if (!dup) {
            list[w++] = list[r];
        }
    }

    *out_count = w;
    return list;
}
