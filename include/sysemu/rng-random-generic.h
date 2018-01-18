/*
 * QEMU Random Number Generator Backend for Windows
 *
 * Copyright 2018 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * callback for setting up user provided random function
 */
typedef void (*QemuRngRandomGenericRandomFunc)(void *, int size);
void qemu_set_rng_random_generic_random_func(QemuRngRandomGenericRandomFunc setup_func);

