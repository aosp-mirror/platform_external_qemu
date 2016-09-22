
#include "android/emulation/goldfish_dma.h"
#include "emugl/common/dma_device.h"

void* (*dma_ptr_getter)();

void set_dma_ptr_getter(void* (*f)()) {

    dma_ptr_getter = f;
}
