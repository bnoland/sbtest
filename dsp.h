//
// dsp.h
// Functions for interfacing with the DSP.
//

#ifndef DSP_H
#define DSP_H

int dsp_reset(int base_io_port);
void dsp_write(int base_io_port, int value);
int dsp_get_version(int base_io_port);
void dsp_speaker_on(int base_io_port);

#endif
