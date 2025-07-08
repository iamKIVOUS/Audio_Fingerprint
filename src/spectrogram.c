#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "audio_io.h"
#include "fft.h"
#include "spectrogram.h"

static void apply_hanning_window(float* frame, int size) {
    for (int i = 0; i < size; ++i) {
        frame[i] *= 0.5f * (1.0f - cosf(2.0f * PI * i / (size - 1)));
    }
}

int build_spectrogram_from_samples(
    const float* samples,
    int num_samples,
    int sample_rate,
    float*** out_spectrogram,
    int* out_num_frames,
    int* out_num_bins
) {
    if (!samples || num_samples < FRAME_SIZE || !out_spectrogram || !out_num_frames || !out_num_bins) {
        fprintf(stderr, "Invalid input to build_spectrogram_from_samples.\n");
        return -1;
    }

    if (sample_rate != SAMPLE_RATE) {
        fprintf(stderr, "Sample rate mismatch: expected %d, got %d\n", SAMPLE_RATE, sample_rate);
        return -1;
    }

    int num_frames = 1 + (num_samples - FRAME_SIZE) / HOP_SIZE;
    int num_bins = FRAME_SIZE / 2;

    float** spectrogram = NULL;
    float* spectrogram_data = NULL;
    Complex* fft_buffer = NULL;
    float* magnitude = NULL;
    float* frame_buffer = NULL;

    // Allocate 2D spectrogram: pointers + contiguous data block
    spectrogram = (float**)malloc(sizeof(float*) * num_frames);
    spectrogram_data = (float*)malloc(sizeof(float) * num_frames * num_bins);
    if (!spectrogram || !spectrogram_data) {
        fprintf(stderr, "Failed to allocate spectrogram memory.\n");
        goto cleanup;
    }

    for (int f = 0; f < num_frames; ++f) {
        spectrogram[f] = &spectrogram_data[f * num_bins];
    }

    fft_buffer = (Complex*)calloc(FRAME_SIZE, sizeof(Complex));
    magnitude = (float*)calloc(num_bins, sizeof(float));
    frame_buffer = (float*)malloc(sizeof(float) * FRAME_SIZE);

    if (!fft_buffer || !magnitude || !frame_buffer) {
        fprintf(stderr, "Memory allocation failed during FFT setup.\n");
        goto cleanup;
    }

    for (int f = 0; f < num_frames; ++f) {
        int offset = f * HOP_SIZE;
        memcpy(frame_buffer, samples + offset, sizeof(float) * FRAME_SIZE);

        apply_hanning_window(frame_buffer, FRAME_SIZE);

        for (int i = 0; i < FRAME_SIZE; ++i) {
            fft_buffer[i].real = frame_buffer[i];
            fft_buffer[i].imag = 0.0f;
        }

        fft(fft_buffer, FRAME_SIZE);
        compute_magnitude_spectrum(fft_buffer, magnitude, FRAME_SIZE);

        memcpy(spectrogram[f], magnitude, sizeof(float) * num_bins);
    }

    *out_spectrogram = spectrogram;
    *out_num_frames = num_frames;
    *out_num_bins = num_bins;

    free(fft_buffer);
    free(magnitude);
    free(frame_buffer);
    return 0;

cleanup:
    free(fft_buffer);
    free(magnitude);
    free(frame_buffer);
    free(spectrogram_data);
    free(spectrogram);
    return -1;
}

int build_spectrogram(
    const char* filepath,
    float*** out_spectrogram,
    int* out_num_frames,
    int* out_num_bins
) {
    float* samples = NULL;
    int num_samples = 0;
    int sample_rate = 0;

    if (load_audio(filepath, &samples, &num_samples, &sample_rate) != 0) {
        fprintf(stderr, "Error loading audio for spectrogram: %s\n", filepath);
        return -1;
    }

    int rc = build_spectrogram_from_samples(samples, num_samples, sample_rate,
                                            out_spectrogram, out_num_frames, out_num_bins);
    free(samples);
    return rc;
}
