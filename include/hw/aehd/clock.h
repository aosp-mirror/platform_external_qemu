/*
 * QEMU AEHD support, paravirtual clock device
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

#ifdef CONFIG_AEHD

void aehdclock_create(void);

#else /* CONFIG_AEHD */

static inline void aehdclock_create(void)
{
}

#endif /* !CONFIG_AEHD */
