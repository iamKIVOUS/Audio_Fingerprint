// File: src/fft.c
// Fast Fourier Transform (FFT) and spectrum utilities.

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "types.h"
#include "fft.h"

// Bit-reversal permutation for in-place FFT reordering.
static void bit_reverse(Complex* x, int n) {
    int i, j = 0;
    for (i = 0; i < n; ++i) {
        if (i < j) {
            Complex tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j |= bit;
    }
}

// In-place iterative Cooley-Tukey FFT. Assumes n is a power of two.
void fft(Complex* x, int n) {
    bit_reverse(x, n);

    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI / len;
        Complex wlen = { cosf(angle), sinf(angle) };

        for (int i = 0; i < n; i += len) {
            Complex w = { 1.0f, 0.0f };
            for (int k = 0; k < len / 2; ++k) {
                Complex u = x[i + k];
                Complex v = {
                    w.real * x[i + k + len/2].real - w.imag * x[i + k + len/2].imag,
                    w.real * x[i + k + len/2].imag + w.imag * x[i + k + len/2].real
                };
                x[i + k]           = (Complex){ u.real + v.real, u.imag + v.imag };
                x[i + k + len/2]   = (Complex){ u.real - v.real, u.imag - v.imag };

                // w *= wlen
                Complex tmp = {
                    w.real * wlen.real - w.imag * wlen.imag,
                    w.real * wlen.imag + w.imag * wlen.real
                };
                w = tmp;
            }
        }
    }
}

// Compute magnitude spectrum |X[k]| for k in [0..n-1].
void compute_magnitude_spectrum(const Complex* x, float* magnitude, int n) {
    int half = n / 2;  // Only compute half spectrum
    for (int i = 0; i < half; ++i) {
        magnitude[i] = sqrtf(x[i].real * x[i].real + x[i].imag * x[i].imag);
    }
}

// Print complex FFT result for debugging.
void print_fft_result(const Complex* x, int n) {
    for (int i = 0; i < n; ++i) {
        printf("Bin %4d: %8.4f + %8.4fi\n", i, x[i].real, x[i].imag);
    }
}

