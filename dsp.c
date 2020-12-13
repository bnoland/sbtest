//
// dsp.c
// Functions for interfacing with the DSP.
//

#include "dsp.h"
#include <i86.h>
#include <conio.h>

//
// The following I/O ports are given as offsets from the base I/O address
//
#define DSP_RESET           0x06    // Used to reset DSP to default state
#define DSP_READ            0x0A    // Used to read data from DSP
#define DSP_WRITE           0x0C    // Used to write data/commands to DSP
#define DSP_WRITE_STATUS    0x0C    // Is DSP ready to accept commands/data?
#define DSP_READ_STATUS     0x0E    // Any data available to read from DSP?

#define DSP_READY           0xAA    // Used to indicate DSP completed reset

//
// DSP commands
//
#define DSP_VERSION         0xE1    // Get DSP version
#define DSP_SPEAKER_ON      0xD1    // Turn on speaker

//
// Resets the DSP. Return value indicates whether or not the reset was
// successful.
//
int dsp_reset(int base_io_port)
{
    unsigned int tries_left;

    outp(base_io_port + DSP_RESET, 1);
    delay(1);   // TODO: Only need to delay >= 3 microseconds
    outp(base_io_port + DSP_RESET, 0);

    // Give the DSP some time to initialize
    delay(1);   // TODO: Needed?

    // Poll the read port until we get a response indicating a reset, or fail if
    // it takes too long
    for (tries_left = 0xFFFF; tries_left > 0; tries_left--)
        if (inp(base_io_port + DSP_READ_STATUS) & 0x80) {
            if (inp(base_io_port + DSP_READ) == DSP_READY)
                return 1;
        }

    return 0;
}

//
// Writes the given value to the DSP. Will block until DSP ready to accept data.
//
void dsp_write(int base_io_port, int value)
{
    // Wait until DSP ready to accept data
    while (inp(base_io_port + DSP_WRITE_STATUS) & 0x80)
        ;

    // Write the value to the DSP
    outp(base_io_port + DSP_WRITE, value);
}

//
// Reads data from the DSP. Will block until data is available to read.
//
int dsp_read(int base_io_port)
{
    // Wait until data to read from DSP
    while (!(inp(base_io_port + DSP_READ_STATUS) & 0x80))
        ;

    // Return the value read from the DSP
    return inp(base_io_port + DSP_READ);
}

//
// Returns the DSP version.
//
int dsp_get_version(int base_io_port)
{
    int version;
    dsp_write(base_io_port, DSP_VERSION);
    version = dsp_read(base_io_port);   // Major version number
    dsp_read(base_io_port);             // Minor version number (ignored)
    return version;
}

//
// Turns the speaker on. Note that this isn't strictly necessary on DSP versions
// 4.xx.
//
void dsp_speaker_on(int base_io_port)
{
    dsp_write(base_io_port, DSP_SPEAKER_ON);
}
