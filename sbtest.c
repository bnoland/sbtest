//
// sbtest.c
// Sound Blaster test program. Plays a 16-bit uncompressed PCM WAVE file given
// as a command line argument. Only supports DSP versions 4.xx for now.
//

#include "sbinfo.h"
#include "dsp.h"
#include "wave.h"
#include "dmabuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#define DMA5_ADDR       0xC4
#define DMA5_COUNT      0xC6
#define DMA5_PAGE       0x8B
#define DMA6_ADDR       0xC8
#define DMA6_COUNT      0xCA
#define DMA6_PAGE       0x89
#define DMA7_ADDR       0xCC
#define DMA7_COUNT      0xCE
#define DMA7_PAGE       0x8A

#define DMA16_FF_REG    0xD8
#define DMA16_MASK_REG  0xD4
#define DMA16_MODE_REG  0xD6

#define DMA_SINGLE_CYCLE    0
#define DMA_AUTO_INIT       1

#define DSP_HALT_SINGLE_CYCLE_DMA   0xD0

#define DMA_BUFFER_SIZE 8192    // Size of DMA buffer in bytes

#define PIC_END_OF_INT  0x20
#define PIC_MASK        0x21
#define PIC_MODE        0x20

#define MIXER_ADDR      0x04
#define MIXER_DATA      0x05
#define MIC_VOLUME      0x0A
#define MASTER_VOLUME   0x22
#define VOICE_VOLUME    0x04

static SBInfo sb_info;              // Info about Sound Blaster card
static DMABuffer dma_buf;           // DMA buffer for transferring audio data
static WaveFileHeader wave_header;  // WAVE file wave_header
static int volatile playing_half;   // Half of DMA buffer currently being played
static FILE *file;                  // The input file

//
// ISR invoked each time the DSP finishes playing half the DMA buffer.
//
void __interrupt dma_output_isr(void)
{
    int base_io_port = sb_info.base_io_port;
    int int_status;

    outp(base_io_port + 4, 0x82);       // Select interrupt status register
    int_status = inp(base_io_port + 5); // Read interrupt status register
    if (int_status & 2)
        inp(base_io_port + 0x0F);       // Acknowledge interrupt

    playing_half ^= 1;  // Switch half of DMA buffer currently being played

    outp(PIC_MODE, PIC_END_OF_INT);     // End of interrupt
}

//
// Programs the DMA controller. Return value indicates failure if DMA channel is
// invalid. Note that we only support 16-bit DMA transfers for now.
//
int program_dma(void)
{
    int dma_addr, dma_count, dma_page;
    unsigned long phys_addr;
    unsigned int page, offset;

    switch (sb_info.dma16_channel) {
    case 5:
        dma_addr = DMA5_ADDR;
        dma_count = DMA5_COUNT;
        dma_page = DMA5_PAGE;
        break;
    case 6:
        dma_addr = DMA6_ADDR;
        dma_count = DMA6_COUNT;
        dma_page = DMA6_PAGE;
        break;
    case 7:
        dma_addr = DMA7_ADDR;
        dma_count = DMA7_COUNT;
        dma_page = DMA7_PAGE;
        break;
    default:
        // Invalid DMA channel.
        return 0;
    }

    phys_addr = DMABuffer_get_physical_address(&dma_buf);
    page = phys_addr >> 16;
    offset = phys_addr & 0xFFFF;

    offset >>= 1;
    offset &= 0x7FFF;
    offset |= (page & 1) << 15;

    outp(DMA16_MASK_REG, (sb_info.dma16_channel - 4) | 4);
    outp(DMA16_FF_REG, 0);
    outp(DMA16_MODE_REG, (sb_info.dma16_channel - 4) | 0x58);

    outp(dma_count, (dma_buf.size / 2 - 1) & 0xFF);
    outp(dma_count, (dma_buf.size / 2 - 1) >> 8);

    outp(dma_page, page);

    outp(dma_addr, offset & 0xFF);
    outp(dma_addr, offset >> 8);

    outp(DMA16_MASK_REG, sb_info.dma16_channel - 4);

    // Note: not strictly necessary on DSP versions 4.xx.
    dsp_speaker_on(sb_info.base_io_port);

    return 1;
}

//
// Starts playing the audio sample using the given DMA mode. Provide count
// giving number of sample bytes to play at a time.
//
void play(int dma_mode, unsigned long count)
{
    int base_io_port = sb_info.base_io_port;

    // Ensure that the expression "count / 2 - 1" used below doesn't dip below
    // zero.
    if (count <= 1)
        count = 2;

    dsp_write(base_io_port, 0x41);
    dsp_write(base_io_port, (wave_header.sample_rate & 0xFF00) >> 8);
    dsp_write(base_io_port, wave_header.sample_rate & 0xFF);

    if (dma_mode == DMA_AUTO_INIT)
        dsp_write(base_io_port, 0xB6);
    else
        dsp_write(base_io_port, 0xB0);

    dsp_write(base_io_port, 0x30);  // 16-bit stereo

    // Assumes 16-bit samples
    dsp_write(base_io_port, (count / 2 - 1) & 0xFF);
    dsp_write(base_io_port, (count / 2 - 1) >> 8);
}

//
// Repeatedly refills the DMA buffer and waits for the read sample to be played.
// Stops when the entire audio file has been played.
//
void play_and_refill_buffer(unsigned long bytes_left)
{
    do {
        unsigned long count;
        int last_fill_half = dma_buf.fill_half ^ 1;

        // Fill the next half of the DMA buffer with audio data while we're
        // waiting for the current sample to finish playing.
        if (DMABuffer_fill_half_buffer(&dma_buf, file, &count) == 1) {
            fprintf(stderr, "Couldn't fill DMA buffer\n");
            exit(1);
        }
        bytes_left -= count;

        // Wait until we're done playing the last half of the DMA buffer that
        // was filled.
        while (playing_half == last_fill_half)
            ;

        if (count < dma_buf.size / 2 || kbhit()) {
            // If user terminated playback by pressing the keyboard, eat the
            // typed key.
            if (kbhit())
                getch();

            // Can play the remaining audio data in a single DMA cycle, so
            // switch to single-cycle DMA mode (terminates auto-initialize DMA
            // mode).
            play(DMA_SINGLE_CYCLE, count);
            while (playing_half == last_fill_half)  // Finish playing sample
                ;

            break;  // Done playing
        }
    } while (feof(file) == 0);
}

void set_mixer(void)
{
    int base_io_port = sb_info.base_io_port;

    outp(base_io_port + MIXER_ADDR, MIC_VOLUME);
    outp(base_io_port + MIXER_DATA, 0);
    outp(base_io_port + MIXER_ADDR, VOICE_VOLUME);
    outp(base_io_port + MIXER_DATA, 0xFF);
    outp(base_io_port + MIXER_ADDR, MASTER_VOLUME);
    outp(base_io_port + MIXER_DATA, 0xFF);
}

int main(int argc, char *argv[])
{
    void __interrupt (*old_isr)(void);
    int old_pic_mask, irq_mask;
    unsigned long bytes_left;
    unsigned long count;

    if (argc != 2) {
        fprintf(stderr, "Usage: sbtest <wave file>\n");
        exit(1);
    }

    //
    // Read the WAVE file header.
    //

    file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open given file\n");
        exit(1);
    }

    switch (WaveFileHeader_read(&wave_header, file)) {
    case 1:
        fprintf(stderr, "Failed to read file header\n");
        exit(1);
    case 2:
        fprintf(stderr, "Given file not of correct format\n");
        exit(1);
    }

    //
    // Initialize the Sound Blaster card and read the DSP version.
    //

    if (SBInfo_init(&sb_info) == 0) {
        fprintf(stderr, "Failed to get necessary Sound Blaster card info\n");
        exit(1);
    }

    if (dsp_reset(sb_info.base_io_port) == 0) {
        fprintf(stderr, "Failed to reset Sound Blaster card\n");
        exit(1);
    }

    SBInfo_get_dsp_version(&sb_info);

    //
    // Allocate the DMA buffer.
    //

    if (DMABuffer_init(&dma_buf, DMA_BUFFER_SIZE) == 0) {
        fprintf(stderr, "Failed to allocate DMA buffer\n");
        exit(1);
    }

    //
    // Print information about the Sound Blaster, the WAVE file read, and the
    // DMA buffer.
    //

    printf("---- Sound Blaster info:\n");
    SBInfo_print(&sb_info);
    printf("\n---- Wave file info:\n");
    WaveFileHeader_print(&wave_header);
    printf("\n---- DMA buffer info:\n");
    DMABuffer_print(&dma_buf);

    //
    // Register an ISR to handle the end of DMA transfers.
    //

    old_isr = _dos_getvect(sb_info.irq_number + 8);
    _dos_setvect(sb_info.irq_number + 8, dma_output_isr);

    old_pic_mask = inp(PIC_MASK);
    irq_mask = 1 << sb_info.irq_number;
    outp(PIC_MASK, old_pic_mask & ~irq_mask);

    //
    // Program the DMA chip.
    //

    if (program_dma() == 0) {
        fprintf(stderr, "Failed to program DMA\n");
        exit(1);
    }

    //
    // Read the first audio sample into the DMA buffer and start playing.
    //

    // set_mixer();

    playing_half = dma_buf.fill_half;
    bytes_left = wave_header.data_size;

    if (DMABuffer_fill_half_buffer(&dma_buf, file, &count) == 1) {
        fprintf(stderr, "Couldn't fill DMA buffer\n");
        exit(1);
    }
    bytes_left -= count;

    if (wave_header.data_size < dma_buf.size / 2) {
        // Can play the audio sample in a single DMA cycle
        play(DMA_SINGLE_CYCLE, count);
        while (playing_half == 0)
            ;
    } else {
        // Need multiple DMA cycles to play the audio sample
        play(DMA_AUTO_INIT, count);
        play_and_refill_buffer(bytes_left);
    }

    //
    // Cleanup.
    //

    // Halt single-cycle DMA.
    dsp_write(sb_info.base_io_port, DSP_HALT_SINGLE_CYCLE_DMA);

    outp(PIC_MASK, old_pic_mask);

    // Restore old ISR
    _dos_setvect(sb_info.irq_number + 8, old_isr);

    DMABuffer_free(&dma_buf);
    fclose(file);
    return 0;
}
