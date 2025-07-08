#ifndef FFT_H
#define FFT_H

#include "types.h"

void fft(Complex* x, int n);
void compute_magnitude_spectrum(const Complex* x, float* magnitude, int n);
void print_fft_result(const Complex* x, int n);

#endif
