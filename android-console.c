/* Android console command implementation.
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
 */

#include "android-console.h"
#include "monitor/monitor.h"
#include "qemu/sockets.h"
#include "net/slirp.h"
#include "slirp/libslirp.h"
#include "qmp-commands.h"

void android_console_kill(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "OK: killing emulator, bye bye\n");
    monitor_suspend(mon);
    qmp_quit(NULL);
}
