/*
 * ARM Android emulator 'ranchu' board.
 *
 * Copyright (c) 2014 Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Emulate a virtual board for use as part of the Android emulator.
 * We create a device tree to pass to the kernel.
 * The board has a mixture of virtio devices and some Android-specific
 * devices inherited from the 32 bit 'goldfish' board.
 *
 * We only support 64-bit ARM CPUs.
 */

/**
 * callback for special handling of device tree
 */
typedef void (*QemuDeviceTreeSetupFunc)(void *);
void qemu_device_tree_setup_callback(QemuDeviceTreeSetupFunc setup_func);

typedef void (*QemuDeviceTreeSetupFunc)(void *);
void qemu_device_tree_setup_callback2(QemuDeviceTreeSetupFunc setup_func);

