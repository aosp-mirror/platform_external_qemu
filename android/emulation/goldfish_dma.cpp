#include "android/emulation/goldfish_dma.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static void* sGoldfishDMA = NULL;

void* getGoldfishDMA() { return sGoldfishDMA; }
void setGoldfishDMA(void* x) { sGoldfishDMA = x; }

