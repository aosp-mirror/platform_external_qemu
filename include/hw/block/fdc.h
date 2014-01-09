/* fdc.c */
#define MAX_FD 2

typedef struct fdctrl_t fdctrl_t;

fdctrl_t *fdctrl_init (qemu_irq irq, int dma_chann, int mem_mapped,
                       hwaddr io_base,
                       BlockDriverState **fds);
fdctrl_t *sun4m_fdctrl_init (qemu_irq irq, hwaddr io_base,
                             BlockDriverState **fds, qemu_irq *fdc_tc);
int fdctrl_get_drive_type(fdctrl_t *fdctrl, int drive_num);
