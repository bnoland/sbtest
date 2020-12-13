//
// wave.c
// Functions for reading PCM WAVE files.
//

#include "wave.h"
#include <string.h>

//
// Read a PCM WAVE file header from the given file. Indicate failure reading
// failed, or file isn't of the right format.
//
int WaveFileHeader_read(WaveFileHeader *header, FILE *file)
{
    // Read assumes we're on a little endian system
    if (fread(header, sizeof(*header), 1, file) != 1)
        return 1;

    // Make sure the header info is correct
    if (memcmp(header->riff_id, "RIFF", 4))
        return 2;
    if (memcmp(header->format, "WAVEfmt ", 8))
        return 2;
    if (header->num_channels != 2)
        return 2;
    if (memcmp(header->data_id, "data", 4))
        return 2;

    return 0;
}

//
// Prints the pertinent WAVE file header attributes to stdout.
//
void WaveFileHeader_print(WaveFileHeader *header)
{
    printf("Format size:        %ld\n", header->fmt_size);
    printf("Audio format:       %hd\n", header->audio_format);
    printf("Number of channels: %hd\n", header->num_channels);
    printf("Sample rate:        %ld\n", header->sample_rate);
    printf("Bits per sample:    %hd\n", header->bits_per_sample);
    printf("Data size (bytes):  %ld\n", header->data_size);
}
