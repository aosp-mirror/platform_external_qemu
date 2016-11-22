#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"

uint64_t io_mem_read(int io_index, hwaddr addr, unsigned size)
{
    return _io_mem_read[io_index][ctzl(size)](io_mem_opaque[io_index],
                                              addr);
}

void io_mem_write(int io_index, hwaddr addr,
                  uint64_t val, unsigned size)
{
    _io_mem_write[io_index][ctzl(size)](io_mem_opaque[io_index],
                                        addr, val);
}
