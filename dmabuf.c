//
// dmabuf.c
// A data type for managing DMA buffers.
//

#include "dmabuf.h"
#include <stdlib.h>
#include <i86.h>

//
// Allocate a DMA buffer used to store the audio data. Note that the buffer must
// *not* cross a 64KB boundary.
//
int DMABuffer_init(DMABuffer *dma_buf, unsigned int size)
{
    unsigned long first, second;
    unsigned long first_page, second_page;

    // Allocate a memory region twice the requested size of the DMA buffer. If
    // one half of the region is not page-aligned, then the other half certainly
    // is.
    dma_buf->region = (unsigned char *) malloc(size * 2);
    if (dma_buf->region == NULL)
        return 0;

    dma_buf->size = size;
    dma_buf->fill_half = 0;

    // Get physical addresses of first and second halves of memory region
    first = ((unsigned long) FP_SEG(dma_buf->region) << 4) +
            (unsigned long) FP_OFF(dma_buf->region);
    second = first + size;

    // Page number is upper nibble of physical address
    first_page = first >> 16;
    second_page = second >> 16;

    if (first_page == second_page) {
        // First half is page-aligned
        dma_buf->offset = 0;
    } else {
        // First half not page-aligned, so second half is
        dma_buf->offset = size;
    }

    return 1;
}

//
// Frees the memory associated with a DMA buffer.
//
void DMABuffer_free(DMABuffer *dma_buf)
{
    free(dma_buf->region);
}

//
// Returns a pointer to the *actual* DMA buffer memory.
//
unsigned char *DMABuffer_get_buffer_ptr(DMABuffer *dma_buf)
{
    return dma_buf->region + dma_buf->offset;
}

//
// Returns the physical address of the DMA buffer.
//
unsigned long DMABuffer_get_physical_address(DMABuffer *dma_buf)
{
    unsigned char *buffer = DMABuffer_get_buffer_ptr(dma_buf);
    unsigned long phys_addr =
        ((unsigned long) FP_SEG(buffer) << 4) + (unsigned long) FP_OFF(buffer);
    return phys_addr;
}

//
// Fills the half of the DMA buffer with sound data from the given file. Return
// value indicates whether read was successful.
//
int DMABuffer_fill_half_buffer(DMABuffer *dma_buf, FILE *file,
                               unsigned long *count)
{
    unsigned char *buffer;
    unsigned long tmp;

    buffer = DMABuffer_get_buffer_ptr(dma_buf);
    if (dma_buf->fill_half == 1)
        buffer += dma_buf->size / 2;    // Point to upper half

    tmp = fread(buffer, 1, dma_buf->size / 2, file);
    if (ferror(file)) {
        // File I/O error
        return 1;
    }

    *count = tmp;   // Read succeeded, so record count of bytes read

    dma_buf->fill_half ^= 1;    // Toggle half of buffer to fill next

    return 0;
}

//
// Writes the attributes of the DMA buffer to stdout.
//
void DMABuffer_print(DMABuffer *dma_buf)
{
    printf("Region (seg:off): %x:%x\n",
           FP_SEG(dma_buf->region), FP_OFF(dma_buf->region));
    printf("Offset:           %u\n", dma_buf->offset);
    printf("Size:             %u\n", dma_buf->size);
    printf("Fill half:        %d\n", dma_buf->fill_half);
}
