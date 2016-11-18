#ifndef HW_MIPS_H
#define HW_MIPS_H
/* Definitions for mips board emulation.  */

/* gt64xxx.c */
PCIBus *pci_gt64120_init(qemu_irq *pic);

/* ds1225y.c */
void *ds1225y_init(hwaddr mem_base, const char *filename);

/* g364fb.c */
int g364fb_mm_init(hwaddr vram_base,
                   hwaddr ctrl_base, int it_shift,
                   qemu_irq irq);

/* mipsnet.c */
void mipsnet_init(int base, qemu_irq irq, NICInfo *nd);

/* jazz_led.c */
extern void jazz_led_init(hwaddr base);

/* mips_int.c */
extern void cpu_mips_irq_init_cpu(CPUOldState *env);

/* mips_timer.c */
extern void cpu_mips_clock_init(CPUOldState *);

/* rc4030.c */
typedef struct rc4030DMAState *rc4030_dma;
void rc4030_dma_memory_rw(void *opaque, hwaddr addr, uint8_t *buf, int len, int is_write);
void rc4030_dma_read(void *dma, uint8_t *buf, int len);
void rc4030_dma_write(void *dma, uint8_t *buf, int len);

void *rc4030_init(qemu_irq timer, qemu_irq jazz_bus,
                  qemu_irq **irqs, rc4030_dma **dmas);

/* dp8393x.c */
void dp83932_init(NICInfo *nd, hwaddr base, int it_shift,
                  qemu_irq irq, void* mem_opaque,
                  void (*memory_rw)(void *opaque, hwaddr addr, uint8_t *buf, int len, int is_write));

#endif
