// types.h

#ifndef TYPES_H
#define TYPES_H

// ===========================
// Complex Number (for FFT)
// ===========================
typedef struct {
    float real;
    float imag;
} Complex;

// ===========================
// Peak Structure
// ===========================

typedef struct {
    int time_index;      // Frame index
    int freq_bin;        // Frequency bin
    float magnitude;     // Magnitude of the peak
} Peak;

// ===========================
// Hash Structure
// ===========================

typedef struct {
    unsigned int hash;   // Hash value
    int time_offset;     // Time of anchor peak (used for alignment)*/
    int anchor_time;
    int song_id;         // Reference to song in database
} FingerprintHash64;

#endif // TYPES_H
