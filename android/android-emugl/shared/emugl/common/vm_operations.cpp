#include "emugl/common/vm_operations.h"

namespace {

QAndroidVmOperations g_vm_operations;

}  // namespace

void set_emugl_vm_operations(const QAndroidVmOperations &vm_operations)
{
    g_vm_operations = vm_operations;
}

const QAndroidVmOperations &get_emugl_vm_operations()
{
    return g_vm_operations;
}
