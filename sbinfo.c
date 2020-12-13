//
// sbinfo.c
// Structures and functions for accessing info about the Sound Blaster card.
//

#include "sbinfo.h"
#include "dsp.h"
#include <stdio.h>
#include <stdlib.h>

//
// Initializes a Sound Blaster info structure. Fills in attributes using the
// BLASTER environment variable, as available. The DSP version has to be filled
// in separately.
//
int SBInfo_init(SBInfo *sb_info)
{
    SBInfo_init_missing(sb_info);
    if (SBInfo_get_from_blaster_env(sb_info) == 0)
        return 0;
    return 1;
}

//
// Initializes a Sound Blaster info structure. All attributes are initially set
// to -1, indicating a missing value.
//
void SBInfo_init_missing(SBInfo *sb_info)
{
    sb_info->base_io_port = -1;
    sb_info->irq_number = -1;
    sb_info->dma8_channel = -1;
    sb_info->dma16_channel = -1;
    sb_info->dsp_version = -1;
}

//
// Extracts info about the Sound Blaster card from the BLASTER environment
// variable. Assumes that the BLASTER variable is well-formed at the very least,
// even if it doesn't contain all of the required values.
//
int SBInfo_get_from_blaster_env(SBInfo *sb_info)
{
    const char *env_string;
    int i, j;
    char c;
    int base;
    int value;
    char buffer[3];
    int have_base = 0, have_irq = 0, have_dma = 0;

    env_string = getenv("BLASTER");
    if (env_string == NULL) {
        // BLASTER environment variable not found
        return 0;
    }

    i = 0;
    while (env_string[i] != '\0') {
        switch (env_string[i]) {
        // Sound Blaster card base I/O address
        case 'a':
        case 'A':
            i++;
            base = 16 * 16;
            value = 0;
            for (j = 0; j < 3; j++) {
                value += base * (env_string[i + j] - '0');
                base >>= 4;
            }
            sb_info->base_io_port = value;
            have_base = 1;
            break;

        // Interrupt request (IRQ) number
        case 'i':
        case 'I':
            i++;
            j = 0;
            buffer[j++] = env_string[i++];
            if (env_string[i] >= '0' && env_string[i] <= '9')
                buffer[j++] = env_string[i++];
            buffer[j] = '\0';
            sb_info->irq_number = atoi(buffer);
            have_irq = 1;
            break;

        // 8-bit DMA channel
        case 'd':
        case 'D':
        // 16-bit DMA channel
        case 'h':
        case 'H':
            c = env_string[i++];
            j = 0;
            buffer[j++] = env_string[i++];
            if (env_string[i] >= '0' && env_string[i] <= '9')
                buffer[j++] = env_string[i++];
            buffer[j] = '\0';
            if (c == 'd' || c == 'D')
                sb_info->dma8_channel = atoi(buffer);
            else
                sb_info->dma16_channel = atoi(buffer);
            have_dma = 1;
            break;
        }

        i++;
    }

    // Did we extract all the values we need?
    return have_base && have_dma && have_irq;
}

//
// Prints the Sound Blaster card info to stdout.
//
void SBInfo_print(SBInfo *sb_info)
{
#define PRINT_ATTRIBUTE(message, format, attrib) \
    if (attrib >= 0) \
        printf(message format "\n", attrib); \
    else \
        printf(message "(missing)\n");

    PRINT_ATTRIBUTE("Base I/O port:      ", "%x", sb_info->base_io_port);
    PRINT_ATTRIBUTE("IRQ number:         ", "%d", sb_info->irq_number);
    PRINT_ATTRIBUTE("8-bit DMA channel:  ", "%d", sb_info->dma8_channel);
    PRINT_ATTRIBUTE("16-bit DMA channel: ", "%d", sb_info->dma16_channel);
    PRINT_ATTRIBUTE("DSP version:        ", "%d", sb_info->dsp_version);
}

//
// Fills in the DSP version attribute. Fails if the base I/O port isn't already
// set.
//
int SBInfo_get_dsp_version(SBInfo *sb_info)
{
    if (sb_info->base_io_port == -1) {
        // Base I/O port missing, so fail
        return 0;
    }

    sb_info->dsp_version = dsp_get_version(sb_info->base_io_port);
    return 1;
}
