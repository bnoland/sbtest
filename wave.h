//
// wave.h
// Functions for reading PCM WAVE files.
//

#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>

//
// Header format for PCM WAVE files.
//
typedef struct {
    // RIFF header
    char riff_id[4];                // Contains the string "RIFF"
    unsigned long chunk_size;       // Size of header minus 8 bytes
    char format[4];                 // Contains the string "WAVE"

    // "fmt " subchunk
    char fmt_id[4];                 // Contains the string "fmt "
    unsigned long fmt_size;         // 16 for PCM
    unsigned short audio_format;    // = 1 for uncompressed data
    unsigned short num_channels;    // 1 = mono, 2 = stereo
    unsigned long sample_rate;      // Digital audio sample rate
    unsigned long byte_rate;        // Who cares
    unsigned short block_align;     // Who cares
    unsigned short bits_per_sample; // 8 = 8 bits, 16 = 16 bits, etc.

    // "data" subchunk
    char data_id[4];                // Contains the string "data"
    unsigned long data_size;        // Number of data bytes
} WaveFileHeader;

int WaveFileHeader_read(WaveFileHeader *header, FILE *file);
void WaveFileHeader_print(WaveFileHeader *header);

#endif
