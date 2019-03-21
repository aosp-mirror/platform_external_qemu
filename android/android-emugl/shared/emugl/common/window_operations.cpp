#include "emugl/common/window_operations.h"

namespace {

QAndroidEmulatorWindowAgent g_window_operations;

}  // namespace

void set_emugl_window_operations(const QAndroidEmulatorWindowAgent &window_operations)
{
    g_window_operations = window_operations;
}

const QAndroidEmulatorWindowAgent &get_emugl_window_operations()
{
    return g_window_operations;
}
