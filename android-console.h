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
#ifdef USE_ANDROID_EMU
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/net_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/vm_operations.h"
#endif
#include "qemu-common.h"

void android_console_kill(Monitor *mon, const QDict *qdict);
void android_console_quit(Monitor *mon, const QDict *qdict);
void android_console_redir(Monitor *mon, const QDict *qdict);
void android_console_redir_list(Monitor *mon, const QDict *qdict);
void android_console_redir_add(Monitor *mon, const QDict *qdict);
void android_console_redir_del(Monitor *mon, const QDict *qdict);

void android_console_power_display(Monitor *mon, const QDict *qdict);
void android_console_power_ac(Monitor *mon, const QDict *qdict);
void android_console_power_status(Monitor *mon, const QDict *qdict);
void android_console_power_present(Monitor *mon, const QDict *qdict);
void android_console_power_health(Monitor *mon, const QDict *qdict);
void android_console_power_capacity(Monitor *mon, const QDict *qdict);
void android_console_power(Monitor *mon, const QDict *qdict);

void android_console_event_types(Monitor *mon, const QDict *qdict);
void android_console_event_codes(Monitor *mon, const QDict *qdict);
void android_console_event_send(Monitor *mon, const QDict *qdict);
void android_console_event_text(Monitor *mon, const QDict *qdict);
void android_console_event(Monitor *mon, const QDict *qdict);

void android_console_avd_stop(Monitor *mon, const QDict *qdict);
void android_console_avd_start(Monitor *mon, const QDict *qdict);
void android_console_avd_status(Monitor *mon, const QDict *qdict);
void android_console_avd_name(Monitor *mon, const QDict *qdict);
void android_console_avd_snapshot(Monitor *mon, const QDict *qdict);
void android_console_avd_snapshot_list(Monitor *mon, const QDict *qdict);
void android_console_avd_snapshot_save(Monitor *mon, const QDict *qdict);
void android_console_avd_snapshot_load(Monitor *mon, const QDict *qdict);
void android_console_avd_snapshot_del(Monitor *mon, const QDict *qdict);
void android_console_avd(Monitor *mon, const QDict *qdict);

void android_console_finger_touch(Monitor *mon, const QDict *qdict);
void android_console_finger_remove(Monitor *mon, const QDict *qdict);
void android_console_finger(Monitor *mon, const QDict *qdict);

void android_console_geo_nmea(Monitor *mon, const QDict *qdict);
void android_console_geo_fix(Monitor *mon, const QDict *qdict);
void android_console_geo(Monitor *mon, const QDict *qdict);

void android_monitor_print_error(Monitor *mon, const char *fmt, ...);

#ifdef USE_ANDROID_EMU
void qemu2_android_console_setup( const QAndroidBatteryAgent* battery_agent,
        const QAndroidFingerAgent* finger_agent,
        const QAndroidLocationAgent* location_agent,
        const QAndroidUserEventAgent* user_event_agent,
        const QAndroidVmOperations* vm_operations,
        const QAndroidNetAgent* net_agent);
#endif
#endif
