// File: src/spectrogram.c
// Spectrogram computation utilities.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "audio_io.h"
#include "fft.h"
#include "spectrogram.h"

// Apply Hanning window in-place to a frame of audio samples.
static void apply_hanning_window(float* frame, int size) {
    for (int i = 0; i < size; ++i) {
        frame[i] *= 0.5f * (1.0f - cosf(2.0f * PI * i / (FRAME_SIZE - 1)));
    }
}

// Build a spectrogram from raw audio samples.
// Allocates and fills out_spectrogram, sets out_num_frames and out_num_bins.
// Returns 0 on success, -1 on error.
int build_spectrogram_from_samples(const float* samples, int num_samples, int sample_rate, float*** out_spectrogram, int* out_num_frames, int* out_num_bins) {
    if (!samples || num_samples < FRAME_SIZE) {
        fprintf(stderr, "Insufficient samples for spectrogram: %d < %d", num_samples, FRAME_SIZE);
        return -1;
    }
    if (sample_rate != SAMPLE_RATE) {
        fprintf(stderr, "Sample rate mismatch: expected %d, got %d", SAMPLE_RATE, sample_rate);
        return -1;
    }

    int num_frames = 1 + (num_samples - FRAME_SIZE) / HOP_SIZE;
    int num_bins = FRAME_SIZE / 2;

    // Allocate spectrogram matrix
    float** spectrogram = (float**)malloc(num_frames * sizeof(float*));
    if (!spectrogram) {
        fprintf(stderr, "Failed to allocate spectrogram rows.");
        return -1;
    }

    // Buffers for FFT
    Complex* fft_buffer = (Complex*)malloc(sizeof(Complex) * FRAME_SIZE);
    float* magnitude = (float*)malloc(sizeof(float) * num_bins);
    float* frame_buffer = (float*)malloc(sizeof(float) * FRAME_SIZE);
    if (!fft_buffer || !magnitude || !frame_buffer) {
        fprintf(stderr, "Memory allocation failed in spectrogram.");
        free(spectrogram);
        free(fft_buffer);
        free(magnitude);
        free(frame_buffer);
        return -1;
    }

    for (int f = 0; f < num_frames; ++f) {
        int offset = f * HOP_SIZE;
        memcpy(frame_buffer, samples + offset, sizeof(float) * FRAME_SIZE);

        apply_hanning_window(frame_buffer, FRAME_SIZE);
        // Real to complex
        for (int i = 0; i < FRAME_SIZE; ++i) {
            fft_buffer[i].real = frame_buffer[i];
            fft_buffer[i].imag = 0.0f;
        }

        // FFT
        fft(fft_buffer, FRAME_SIZE);

        // Magnitude
        compute_magnitude_spectrum(fft_buffer, magnitude, FRAME_SIZE);

        // Copy to row
        spectrogram[f] = (float*)malloc(sizeof(float) * num_bins);
        if (!spectrogram[f]) {
            fprintf(stderr, "Failed to allocate spectrogram row %d.", f);
            // cleanup
            for (int k = 0; k < f; ++k) free(spectrogram[k]);
            free(spectrogram);
            free(fft_buffer);
            free(magnitude);
            free(frame_buffer);
            return -1;
        }
        memcpy(spectrogram[f], magnitude, sizeof(float) * num_bins);
    }

    free(fft_buffer);
    free(magnitude);
    free(frame_buffer);

    *out_spectrogram = spectrogram;
    *out_num_frames = num_frames;
    *out_num_bins = num_bins;
    return 0;
}

// Build a spectrogram from an audio file path.
// Loads audio, then calls build_spectrogram_from_samples.
int build_spectrogram(const char* filepath, float*** out_spectrogram, int* out_num_frames, int* out_num_bins) {
    float* samples = NULL;
    int num_samples = 0;
    int sample_rate = 0;

    if (load_audio(filepath, &samples, &num_samples, &sample_rate) != 0) {
        fprintf(stderr, "Error loading audio for spectrogram: %s", filepath);
        return -1;
    }

    int rc = build_spectrogram_from_samples(samples, num_samples, sample_rate, out_spectrogram, out_num_frames, out_num_bins);
    free(samples);
    return rc;
}
