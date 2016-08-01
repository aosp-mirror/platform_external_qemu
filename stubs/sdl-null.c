
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "ui/console.h"

bool sdl_display_init(DisplayState *ds, int full_screen, int no_frame) {
    (void)ds;
    (void)full_screen;
    (void)no_frame;
    return false;
}

void sdl_display_early_init(int opengl) {
    (void)opengl;
}
