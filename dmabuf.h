//
// dmabuf.h
// A data type for managing DMA buffers.
//

#ifndef DMABUF_H
#define DMABUF_H

#include <stdio.h>

//
// Structure holding info about a DMA buffer.
//
typedef struct {
    unsigned char *region;      // Pointer to region holding DMA buffer
    unsigned int offset;        // Location of DMA buffer in region
    unsigned int size;          // Size of DMA buffer
    int fill_half;              // Half of buffer to fill next
} DMABuffer;

int DMABuffer_init(DMABuffer *dma_buf, unsigned int size);
void DMABuffer_free(DMABuffer *dma_buf);
unsigned char *DMABuffer_get_buffer_ptr(DMABuffer *dma_buf);
unsigned long DMABuffer_get_physical_address(DMABuffer *dma_buf);
int DMABuffer_fill_half_buffer(DMABuffer *dma_buf, FILE *file,
                               unsigned long *count);
void DMABuffer_print(DMABuffer *dma_buf);

#endif
