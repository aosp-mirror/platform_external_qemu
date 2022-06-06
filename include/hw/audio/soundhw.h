#ifndef HW_AUDIO_H
#define HW_AUDIO_H

void isa_register_soundhw(const char *name, const char *descr,
                          int (*init_isa)(ISABus *bus));

void pci_register_soundhw(const char *name, const char *descr,
                          int (*init_pci)(PCIBus *bus, const char *arg));

bool soundhw_init(const char *arg);
void select_soundhw(const char *dev, int len);

#endif
