// File: include/audio_io.h

#ifndef AUDIO_IO_H
#define AUDIO_IO_H

int load_audio(const char* filepath, float** out_buffer, int* out_samples, int* out_samplerate);

#endif
