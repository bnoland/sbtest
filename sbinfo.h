//
// sbinfo.h
// Structures and functions for accessing info about the Sound Blaster card.
//

#ifndef SB_INFO_H
#define SB_INFO_H

//
// Structure containing info about the Sound Blaster card.
//
typedef struct {
    int base_io_port;   // Base I/O port
    int irq_number;     // IRQ number
    int dma8_channel;   // 8-bit DMA channel
    int dma16_channel;  // 16-bit DMA channel
    int dsp_version;    // DSP version
} SBInfo;

int SBInfo_init(SBInfo *sb_info);
void SBInfo_init_missing(SBInfo *sb_info);
int SBInfo_get_from_blaster_env(SBInfo *sb_info);
void SBInfo_print(SBInfo *sb_info);
int SBInfo_get_dsp_version(SBInfo *sb_info);

#endif
