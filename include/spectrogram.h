// File: include/spectrogram.h

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include "types.h"

/**
 * Load an audio file, preprocess it (mono, resample, normalize),
 * then compute its spectrogram via STFT.
 *
 * @param filepath          Path to input WAV file
 * @param out_spectrogram   Pointer to receive 2D spectrogram [frames][bins]
 * @param out_num_frames    Pointer to receive number of time frames
 * @param out_num_bins      Pointer to receive number of frequency bins
 * @return 0 on success, non-zero on error
 */
int build_spectrogram(const char* filepath,
                      float*** out_spectrogram,
                      int* out_num_frames,
                      int* out_num_bins);

/**
 * Compute spectrogram from raw audio samples.
 *
 * @param samples           Mono, normalized float samples
 * @param num_samples       Number of samples in buffer
 * @param sample_rate       Sample rate of the samples (should equal SAMPLE_RATE)
 * @param out_spectrogram   Pointer to receive 2D spectrogram [frames][bins]
 * @param out_num_frames    Pointer to receive number of time frames
 * @param out_num_bins      Pointer to receive number of frequency bins
 * @return 0 on success, non-zero on error
 */
int build_spectrogram_from_samples(const float* samples,
                                   int num_samples,
                                   int sample_rate,
                                   float*** out_spectrogram,
                                   int* out_num_frames,
                                   int* out_num_bins);

#endif // SPECTROGRAM_H