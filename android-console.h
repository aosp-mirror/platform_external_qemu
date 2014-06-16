/* Android console interface
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
 */

#ifndef ANDROID_CONSOLE_H
#define ANDROID_CONSOLE_H

#include "qemu-common.h"

void android_console_kill(Monitor *mon, const QDict *qdict);
void android_console_quit(Monitor *mon, const QDict *qdict);
void android_console_redir(Monitor *mon, const QDict *qdict);
void android_console_redir_list(Monitor *mon, const QDict *qdict);
void android_console_redir_add(Monitor *mon, const QDict *qdict);
void android_console_redir_del(Monitor *mon, const QDict *qdict);

#endif
