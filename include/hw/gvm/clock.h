/*
 * QEMU GVM support, paravirtual clock device
 *
 * Copyright (C) 2011 Siemens AG
 *
 * Authors:
 *  Jan Kiszka        <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2.
 * See the COPYING file in the top-level directory.
 *
 */

#ifdef CONFIG_GVM

void gvmclock_create(void);

#else /* CONFIG_GVM */

static inline void gvmclock_create(void)
{
}

#endif /* !CONFIG_GVM */
