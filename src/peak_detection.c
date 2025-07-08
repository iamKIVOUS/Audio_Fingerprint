// File: src/peak_detection.c
// Spectrogram peak detection utilities.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "peak_detection.h"
#include "config.h"
#include "types.h"


// Convert linear magnitude to dB scale (avoids log(0)).
static float magnitude_to_db(float magnitude) {
    return 20.0f * log10f(fmaxf(magnitude, 1e-10f));  // Avoid log(0)
}

// Check if current point is a local maximum in its neighborhood.
static int is_local_maximum(float** spectrogram, int t, int f, int num_frames, int num_bins) {
    float current = spectrogram[t][f];

    for (int dt = -NEIGHBORHOOD_SIZE; dt <= NEIGHBORHOOD_SIZE; dt++) {
        for (int df = -NEIGHBORHOOD_SIZE; df <= NEIGHBORHOOD_SIZE; df++) {
            if (dt == 0 && df == 0) continue;
            int nt = t + dt;
            int nf = f + df;
            if (nt < 0 || nt >= num_frames || nf < 0 || nf >= num_bins)
                continue;

            if (spectrogram[nt][nf] > current)
                return 0;
        }
    }
    return 1;
}

// Detect peaks in the spectrogram and return an array of Peak structs.
// Returns dynamically allocated array (caller must free), and sets num_peaks_out.
Peak* detect_peaks(float** spectrogram, int num_frames, int num_bins, int* num_peaks_out) {
    if (!spectrogram || num_frames <= 0 || num_bins <= 0 || !num_peaks_out) {
        fprintf(stderr, "Invalid input to detect_peaks()\n");
        return NULL;
    }

    int capacity = num_frames * 10;  // Initial estimate
    Peak* peaks = (Peak*)malloc(capacity * sizeof(Peak));
    if (!peaks) {
        fprintf(stderr, "Memory allocation failed for peaks\n");
        return NULL;
    }

    int count = 0;

    for (int t = 0; t < num_frames; t++) {
        for (int f = 1; f < num_bins - 1; f++) {
            float db_mag = magnitude_to_db(spectrogram[t][f]);

            if (db_mag >= THRESHOLD_MAGNITUDE &&
                is_local_maximum(spectrogram, t, f, num_frames, num_bins)) {
                
                if (count >= capacity) {
                    capacity *= 2;
                    Peak* temp = realloc(peaks, capacity * sizeof(Peak));
                    if (!temp) {
                        fprintf(stderr, "Reallocation failed in detect_peaks()\n");
                        free(peaks);
                        return NULL;
                    }
                    peaks = temp;
                }

                peaks[count].time_index = t;
                peaks[count].freq_bin = f;
                peaks[count].magnitude = db_mag;
                count++;
            }
        }
    }

    *num_peaks_out = count;
    return peaks;
}
