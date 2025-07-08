#ifndef HASHING_H
#define HASHING_H

#include "types.h"

//FingerprintHash* generate_fingerprints(const Peak* peaks, int num_peaks, int song_id, int* num_hashes_out);
FingerprintHash64* generate_fingerprint_hashes(const Peak* peaks, int num_peaks, int song_id, int* out_count);
#endif // HASHING_H
 