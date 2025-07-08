// File: src/audio_io.c
// Audio file loading and preprocessing utilities.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sndfile.h"
#include <math.h>
#include "config.h"
#include "audio_io.h"

// Loads an audio file, converts to mono, resamples if needed, and normalizes.
// Returns 0 on success, -1 on failure.
// Caller must free *out_buffer.
int load_audio(const char* filepath, float** out_buffer, int* out_samples, int* out_samplerate) {
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    SNDFILE* file = sf_open(filepath, SFM_READ, &sfinfo);
    if (!file) {
        fprintf(stderr, "Error opening audio file: %s\n", filepath);
        return -1;
    }

    int total_samples = sfinfo.frames;
    int channels = sfinfo.channels;
    int sample_rate = sfinfo.samplerate;

    float* buffer = (float*)malloc(total_samples * channels * sizeof(float));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed.\n");
        sf_close(file);
        return -1;
    }

    int readcount = sf_readf_float(file, buffer, total_samples);
    sf_close(file);

    // Convert to mono if stereo
    float* mono = (float*)calloc(total_samples, sizeof(float));
    if (!mono) {
        fprintf(stderr, "Memory allocation failed (mono).\n");
        free(buffer);
        return -1;
    }

    if (channels == 1) {
        memcpy(mono, buffer, total_samples * sizeof(float));
    } else {
        for (int i = 0; i < total_samples; ++i) {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                sum += buffer[i * channels + ch];
            }
            mono[i] = sum / channels;
        }
    }
    free(buffer);

    // Resample if sample rate != 44100
    if (sample_rate != SAMPLE_RATE) {
        int new_length = (int)((double)total_samples * SAMPLE_RATE / sample_rate);
        float* resampled = (float*)malloc(new_length * sizeof(float));
        if (!resampled) {
            fprintf(stderr, "Memory allocation failed (resample).\n");
            free(mono);
            return -1;
        }

        for (int i = 0; i < new_length; ++i) {
            double src_index = (double)i * sample_rate / SAMPLE_RATE;
            int idx = (int)src_index;
            double frac = src_index - idx;

            // Linear interpolation
            float a = (idx < total_samples) ? mono[idx] : 0.0f;
            float b = (idx + 1 < total_samples) ? mono[idx + 1] : 0.0f;
            resampled[i] = a + frac * (b - a);
        }

        free(mono);
        mono = resampled;
        total_samples = new_length;
        sample_rate = SAMPLE_RATE;
    }

    // Normalize to [-1.0, 1.0]
    float max_amp = 0.0f;
    for (int i = 0; i < total_samples; ++i) {
        if (fabsf(mono[i]) > max_amp) {
            max_amp = fabsf(mono[i]);
        }
    }

    if (max_amp > 0.0f) {
        for (int i = 0; i < total_samples; ++i) {
            mono[i] /= max_amp;
        }
    }

    *out_buffer = mono;
    *out_samples = total_samples;
    *out_samplerate = sample_rate;

    return 0;
}
