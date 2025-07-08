// File: include/peak_detection.h

#ifndef PEAK_DETECTION_H
#define PEAK_DETECTION_H

#include "types.h"  // For Peak struct

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detects prominent local maxima (peaks) in the given spectrogram.
 *
 * @param spectrogram     2D array [num_frames][num_bins] of magnitudes
 * @param num_frames      Number of time frames in spectrogram
 * @param num_bins        Number of frequency bins in spectrogram
 * @param num_peaks_out   Pointer to int to store number of detected peaks
 * @return Peak*          Dynamically allocated array of Peak structs (must be freed by caller), or NULL on failure
 */
Peak* detect_peaks(float** spectrogram, int num_frames, int num_bins, int* num_peaks_out);

#ifdef __cplusplus
}
#endif

#endif // PEAK_DETECTION_H
