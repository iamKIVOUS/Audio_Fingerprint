// config.h

#ifndef CONFIG_H
#define CONFIG_H

// ===========================
// Audio Configuration
// ===========================

#define SAMPLE_RATE         44100       // Resample target sample rate
#define FRAME_SIZE          2048        // STFT frame size
#define HOP_SIZE            1024        // 50% overlap
#define PI                  3.14159265358979f
#define THRESHOLD_MAGNITUDE 27.0f       // Peak detection threshold
#define NEIGHBORHOOD_SIZE   3  // Adjust for sensitivity vs precision

// ===========================
// Database Configuration
// ===========================

#define SONGS_FOLDER         "songs/"
#define SONGS_DB_PATH        "data/songs.db"         // Songs metadata
#define FINGERPRINTS_DB_PATH "data/fingerprints.db"  // Fingerprint hashes
#define DB_PATH              "data/audio_fingerprint.db" // Audio fingerprints

// ===========================
// Hashing Configuration
// ===========================

#define FAN_VALUE            5           // Number of nearby points used for each hash
#define MAX_FREQ_BIN     1023     // 10 bits
#define MAX_TIME_DELTA   4095     // 12 bits
#define MAX_TIME         1048575  // 20 bits
#define MAX_DELTA_FREQ   31       // for signed 6-bit range [-32, +31]

#endif // CONFIG_H
