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
#include "hw/misc/goldfish_battery.h"
#include "hw/input/goldfish_events.h"
#include "sysemu/sysemu.h"
#include "hmp.h"

#ifdef USE_ANDROID_EMU

#include "android/globals.h"

typedef struct AndroidConsoleRec_ {
    // Interfaces to call into QEMU specific code.
    QAndroidBatteryAgent battery_agent;
    QAndroidFingerAgent finger_agent;
    QAndroidLocationAgent location_agent;
    QAndroidUserEventAgent user_event_agent;
    QAndroidVmOperations vm_operations;
    QAndroidNetAgent net_agent;
} AndroidConsoleRec;

static AndroidConsoleRec _g_global;

void qemu2_android_console_setup(const QAndroidBatteryAgent* battery_agent,
                                 const QAndroidFingerAgent* finger_agent,
                                 const QAndroidLocationAgent* location_agent,
                                 const QAndroidUserEventAgent* user_event_agent,
                                 const QAndroidVmOperations* vm_operations,
                                 const QAndroidNetAgent* net_agent) {
    AndroidConsoleRec* global = &_g_global;
    memset(global, 0, sizeof(*global));
    // Copy the QEMU specific interfaces passed in to make lifetime management
    // simpler.
    global->battery_agent = *battery_agent;
    global->finger_agent = *finger_agent;
    global->location_agent = *location_agent;
    global->user_event_agent = *user_event_agent;
    global->vm_operations = *vm_operations;
    global->net_agent = *net_agent;
}
#endif

typedef struct {
    int is_udp;
    int host_port;
    int guest_port;
} RedirRec;

GList* redir_list;

void android_monitor_print_error(Monitor* mon, const char* fmt, ...) {
    /* Print an error (typically a syntax error from the parser), with
     * the required "KO: " prefix.
     */
    va_list ap;

    monitor_printf(mon, "KO: ");
    va_start(ap, fmt);
    monitor_vprintf(mon, fmt, ap);
    va_end(ap);
}

void android_console_kill(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "OK: killing emulator, bye bye\n");
    monitor_suspend(mon);
    qmp_quit(NULL);
}

void android_console_quit(Monitor* mon, const QDict* qdict) {
    /* Don't print an OK response for success, just close the connection */
    if (monitor_disconnect(mon)) {
        monitor_printf(mon, "KO: this connection doesn't support quitting\n");
    }
}

#ifdef CONFIG_SLIRP
void android_console_redir_list(Monitor* mon, const QDict* qdict) {
    if (!redir_list) {
        monitor_printf(mon, "no active redirections\n");
    } else {
        GList* l;

        for (l = redir_list; l; l = l->next) {
            RedirRec* r = l->data;

            monitor_printf(mon,
                           "%s:%-5d => %-5d\n",
                           r->is_udp ? "udp" : "tcp",
                           r->host_port,
                           r->guest_port);
        }
    }
    monitor_printf(mon, "OK\n");
}

static int parse_proto(const char* s) {
    if (!strcmp(s, "tcp")) {
        return 0;
    } else if (!strcmp(s, "udp")) {
        return 1;
    } else {
        return -1;
    }
}

static int parse_port(const char* s) {
    char* end;
    int port;

    port = strtol(s, &end, 10);
    if (*end != 0 || port < 1 || port > 65535) {
        return 0;
    }
    return port;
}

void android_console_redir_add(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_str(qdict, "arg");
    char** tokens;
    int is_udp, host_port, guest_port;
    Slirp* slirp;
    Error* err = NULL;
    struct in_addr host_addr = {.s_addr = htonl(INADDR_LOOPBACK)};
    struct in_addr guest_addr = {.s_addr = 0};
    RedirRec* redir;

    slirp = net_slirp_lookup(NULL, NULL, &err);
    if (err) {
        monitor_printf(mon, "KO: %s\n", error_get_pretty(err));
        error_free(err);
        return;
    }

    /* Argument syntax: "proto:hostport:guestport" */
    tokens = g_strsplit(arg, ":", 0);

    if (g_strv_length(tokens) != 3) {
        goto fail_syntax;
    }

    is_udp = parse_proto(tokens[0]);
    host_port = parse_port(tokens[1]);
    guest_port = parse_port(tokens[2]);

    if (is_udp < 0 || host_port == 0 || guest_port == 0) {
        goto fail_syntax;
    }

    g_strfreev(tokens);

    if (slirp_add_hostfwd(
                slirp, is_udp, host_addr, host_port, guest_addr, guest_port) <
        0) {
        monitor_printf(mon,
                       "KO: can't setup redirection, "
                       "port probably used by another program on host\n");
        return;
    }

    redir = g_new0(RedirRec, 1);
    redir->is_udp = is_udp;
    redir->host_port = host_port;
    redir->guest_port = guest_port;
    redir_list = g_list_append(redir_list, redir);

    monitor_printf(mon, "OK\n");
    return;

fail_syntax:
    monitor_printf(mon,
                   "KO: bad redirection format, try "
                   "(tcp|udp):hostport:guestport\n");
    g_strfreev(tokens);
}

static gint redir_cmp(gconstpointer a, gconstpointer b) {
    const RedirRec* ra = a;
    const RedirRec* rb = b;

    /* For purposes of list deletion, only protocol and host port matter */
    if (ra->is_udp != rb->is_udp) {
        return ra->is_udp - rb->is_udp;
    }
    return ra->host_port - rb->host_port;
}

void android_console_redir_del(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_str(qdict, "arg");
    char** tokens;
    int is_udp, host_port;
    Slirp* slirp;
    Error* err = NULL;
    struct in_addr host_addr = {.s_addr = INADDR_ANY};
    RedirRec rr;
    GList* entry;

    slirp = net_slirp_lookup(NULL, NULL, &err);
    if (err) {
        monitor_printf(mon, "KO: %s\n", error_get_pretty(err));
        error_free(err);
        return;
    }

    /* Argument syntax: "proto:hostport" */
    tokens = g_strsplit(arg, ":", 0);

    if (g_strv_length(tokens) != 2) {
        goto fail_syntax;
    }

    is_udp = parse_proto(tokens[0]);
    host_port = parse_port(tokens[1]);

    if (is_udp < 0 || host_port == 0) {
        goto fail_syntax;
    }

    g_strfreev(tokens);

    rr.is_udp = is_udp;
    rr.host_port = host_port;
    entry = g_list_find_custom(redir_list, &rr, redir_cmp);

    if (!entry || slirp_remove_hostfwd(slirp, is_udp, host_addr, host_port)) {
        monitor_printf(mon,
                       "KO: can't remove unknown redirection (%s:%d)\n",
                       is_udp ? "udp" : "tcp",
                       host_port);
        return;
    }

    g_free(entry->data);
    redir_list = g_list_delete_link(redir_list, entry);

    monitor_printf(mon, "OK\n");
    return;

fail_syntax:
    monitor_printf(mon, "KO: bad redirection format, try (tcp|udp):hostport\n");
    g_strfreev(tokens);
}

#else /* not CONFIG_SLIRP */
void android_console_redir_list(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}

void android_console_redir_add(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}

void android_console_redir_remove(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}
#endif

enum {
    CMD_REDIR,
    CMD_REDIR_LIST,
    CMD_REDIR_ADD,
    CMD_REDIR_DEL,
};

static const char* redir_help[] = {
        /* CMD_REDIR */
        "allows you to add, list and remove UDP and/or PORT redirection "
        "from the host to the device\n"
        "as an example, 'redir  tcp:5000:6000' will route any packet sent "
        "to the host's TCP port 5000\n"
        "to TCP port 6000 of the emulated device\n"
        "\n"
        "available sub-commands:\n"
        "    list             list current redirections\n"
        "    add              add new redirection\n"
        "    del              remove existing redirection\n",
        /* CMD_REDIR_LIST */
        "list current port redirections. use 'redir add' and 'redir del' to "
        "add "
        "and remove them",
        /* CMD_REDIR_ADD */
        "add a new port redirection, arguments must be:\n"
        "\n"
        "  redir add <protocol>:<host-port>:<guest-port>\n"
        "\n"
        "where:   <protocol>     is either 'tcp' or 'udp'\n"
        "         <host-port>    a number indicating which "
        "port on the host to open\n"
        "         <guest-port>   a number indicating which "
        "port to route to on the device\n"
        "\n"
        "as an example, 'redir  tcp:5000:6000' will allow any packets sent to\n"
        "the host's TCP port 5000 to be routed to TCP port 6000 of the "
        "emulated device",
        /* CMD_REDIR_DEL */
        "remove a port redirecion that was created with 'redir add', "
        "arguments must be:\n"
        "  redir  del <protocol>:<host-port>\n\n"
        "see the 'help redir add' for the meaning of <protocol> and "
        "<host-port>",
};

void android_console_redir(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_REDIR;

    if (helptext) {
        if (strstr(helptext, "add")) {
            cmd = CMD_REDIR_ADD;
        } else if (strstr(helptext, "del")) {
            cmd = CMD_REDIR_DEL;
        } else if (strstr(helptext, "list")) {
            cmd = CMD_REDIR_LIST;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   redir_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

void android_console_power_display(Monitor* mon, const QDict* qdict) {
    goldfish_battery_display(mon);

    monitor_printf(mon, "OK\n");
}

void android_console_power_ac(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "on") == 0) {
            goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, 1);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "off") == 0) {
            goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, 0);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon, "KO: Usage: \"ac on\" or \"ac off\"\n");
}

void android_console_power_status(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "unknown") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_STATUS, POWER_SUPPLY_STATUS_UNKNOWN);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "charging") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_STATUS, POWER_SUPPLY_STATUS_CHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "discharging") == 0) {
            goldfish_battery_set_prop(0,
                                      POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_DISCHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "not-charging") == 0) {
            goldfish_battery_set_prop(0,
                                      POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_NOT_CHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "full") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_STATUS, POWER_SUPPLY_STATUS_FULL);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon,
                   "KO: Usage: \"status unknown|charging|"
                   "discharging|not-charging|full\"\n");
}

void android_console_power_present(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "true") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_PRESENT, 1);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "false") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_PRESENT, 0);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon, "KO: Usage: \"present true\" or \"present false\"\n");
}

void android_console_power_health(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "unknown") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_HEALTH, POWER_SUPPLY_HEALTH_UNKNOWN);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "good") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_HEALTH, POWER_SUPPLY_HEALTH_GOOD);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "overheat") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_HEALTH, POWER_SUPPLY_HEALTH_OVERHEAT);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "dead") == 0) {
            goldfish_battery_set_prop(
                    0, POWER_SUPPLY_PROP_HEALTH, POWER_SUPPLY_HEALTH_DEAD);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "overvoltage") == 0) {
            goldfish_battery_set_prop(0,
                                      POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_OVERVOLTAGE);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "failure") == 0) {
            goldfish_battery_set_prop(0,
                                      POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon,
                   "KO: Usage: \"health unknown|good|overheat|"
                   "dead|overvoltage|failure\"\n");
}

void android_console_power_capacity(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        int capacity;

        if (sscanf(arg, "%d", &capacity) == 1 && capacity >= 0 &&
            capacity <= 100) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, capacity);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon, "KO: Usage: \"capacity <percentage>\"\n");
}

enum {
    CMD_POWER,
    CMD_POWER_DISPLAY,
    CMD_POWER_AC,
    CMD_POWER_STATUS,
    CMD_POWER_PRESENT,
    CMD_POWER_HEALTH,
    CMD_POWER_CAPACITY,
};

static const char* power_help[] = {
        /* CMD_POWER */
        "allows to change battery and AC power status\n"
        "\n"
        "available sub-commands:\n"
        "   power display          display battery and charger state\n"
        "   power ac               set AC charging state\n"
        "   power status           set battery status\n"
        "   power present          set battery present state\n"
        "   power health           set battery health state\n"
        "   power capacity         set battery capacity state\n",
        /* CMD_POWER_DISPLAY */
        "display battery and charger state",
        /* CMD_POWER_AC */
        "'ac on|off' allows you to set the AC charging state to on or off",
        /* CMD_POWER_STATUS */
        "'status unknown|charging|discharging|not-charging|full' allows you "
        "to set battery status",
        /* CMD_POWER_PRESENT */
        "'present true|false' allows you to set battery present state to true "
        "or false",
        /* CMD_POWER_HEALTH */
        "'health unknown|good|overheat|dead|overvoltage|failure' allows you "
        "to set battery health state",
        /* CMD_POWER_CAPACITY */
        "'capacity <percentage>' allows you to set battery capacity to a "
        "value 0 - 100",
};

void android_console_power(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_POWER;

    /* In the below command name parsing, "capacity" has to precede "ac"
     * otherwise we will hit on "ac" first.
     */
    if (helptext) {
        if (strstr(helptext, "display")) {
            cmd = CMD_POWER_DISPLAY;
        } else if (strstr(helptext, "capacity")) {
            cmd = CMD_POWER_CAPACITY;
        } else if (strstr(helptext, "ac")) {
            cmd = CMD_POWER_AC;
        } else if (strstr(helptext, "status")) {
            cmd = CMD_POWER_STATUS;
        } else if (strstr(helptext, "present")) {
            cmd = CMD_POWER_PRESENT;
        } else if (strstr(helptext, "health")) {
            cmd = CMD_POWER_HEALTH;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   power_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

enum {
    CMD_EVENT,
    CMD_EVENT_TYPES,
    CMD_EVENT_CODES,
    CMD_EVENT_SEND,
    CMD_EVENT_TEXT,
};

static const char* event_help[] = {
        /* CMD_EVENT */
        "allows you to send fake hardware events to the kernel\n"
        "\n"
        "available sub-commands:\n"
        "   event send             send a series of events to the kernel\n"
        "   event types            list all <type> aliases\n"
        "   event codes            list all <code> aliases for a given <type>\n"
        "   event text             simulate keystrokes from a given text\n",
        /* CMD_EVENT_TYPES */
        "'event types' list all <type> string aliases supported by the "
        "'event' subcommands",
        /* CMD_EVENT_CODES */
        "'event codes <type>' lists all <code> string aliases for a given "
        "event <type>",
        /* CMD_EVENT_SEND */
        "'event send <type>:<code>:<value> ...' allows your to send one or "
        "more hardware events\nto the Android kernel. you can use text names "
        "or integers for <type> and <code>",
        /* CMD_EVENT_TEXT */
        "'event text <message>' allows you to simulate keypresses to generate "
        "a given text\nmessage. <message> must be an utf-8 string. Unicode "
        "points will be reverse-mapped\naccording to the current device "
        "keyboard. unsupported characters will be discarded\nsilently",
};

void android_console_event_types(Monitor* mon, const QDict* qdict) {
    int count = gf_get_event_type_count();
    int nn;

    monitor_printf(mon,
                   "event <type> can be an integer or one of the"
                   "following aliases\n");

    /* Loop through and print out each type found along with the count of alias
     * codes if any.
     */
    for (nn = 0; nn < count; nn++) {
        char tmp[16];
        char* p = tmp;
        int code_count;
        gf_get_event_type_name(nn, p);

        code_count = gf_get_event_code_count(p);

        monitor_printf(mon, "    %-8s", p);

        if (code_count > 0) {
            monitor_printf(mon, "  (%d code aliases)", code_count);
        }

        monitor_printf(mon, "\n");
    }

    monitor_printf(mon, "OK\n");
}

void android_console_event_codes(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    int count, nn;

    if (!arg) {
        monitor_printf(mon, "KO: argument missing, try 'event codes <type>'\n");
        return;
    }

    count = gf_get_event_code_count(arg);

    /* If the type is invalid then bail */
    if (count < 0) {
        monitor_printf(
                mon, "KO: bad argument, see 'event types' for valid values\n");
        return;
    }

    if (count == 0) {
        monitor_printf(mon, "no code aliases defined for this type\n");
    } else {
        monitor_printf(
                mon, "type '%s' accepts the following <code> aliases:\n", arg);
        for (nn = 0; nn < count; nn++) {
            char temp[20], *p = temp;
            gf_get_event_code_name(arg, nn, p);
            monitor_printf(mon, "    %-12s\r\n", p);
        }
    }

    monitor_printf(mon, "OK\n");
}

void android_console_event_send(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");
    char** substr;
    int type, code, value = -1;

    if (!arg) {
        monitor_printf(mon,
                       "KO: Usage: event send <type>:<code>:<value> ...\n");
        return;
    }

    substr = g_strsplit(arg, ":", 3);

    /* The event type can be a symbol or number.  Check that we have a valid
     * type string and get the value depending on its format.
     */
    if (g_ascii_isdigit(*substr[0])) {
        type = g_ascii_strtoull(substr[0], NULL, 0);
    } else {
        type = gf_get_event_type_value(substr[0]);
    }
    if (type == -1) {
        monitor_printf(mon,
                       "KO: invalid event type in '%s', try 'event "
                       "list types' for valid values\n",
                       arg);
        goto out;
    }

    /* The event code can be a symbol or number.  Check that we have a valid
     * code string and get the value depending on its format.
     */
    if (g_ascii_isdigit(*substr[1])) {
        code = g_ascii_strtoull(substr[1], NULL, 0);
    } else {
        code = gf_get_event_code_value(type, substr[1]);
    }
    if (code == -1) {
        monitor_printf(mon,
                       "KO: invalid event code in '%s', try 'event list "
                       "codes <type>' for valid values\n",
                       arg);
        goto out;
    }

    /* The event value can only be a numeric value.  Check that the value
     * string is value and convert it.
     */
    if (!substr[2] || !g_ascii_isdigit(*substr[2])) {
        monitor_printf(mon,
                       "KO: invalid event value in '%s', must be an "
                       "integer\n",
                       arg);
        goto out;
    }
    value = g_ascii_strtoull(substr[2], NULL, 0);

    gf_event_send(type, code, value);

    monitor_printf(mon, "OK\n");

out:
    g_strfreev(substr);
}

void android_console_event_text(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (!arg) {
        monitor_printf(mon,
                       "KO: argument missing, try 'event text <message>'\n");
        return;
    }

    monitor_printf(mon, "KO: 'event text' is currently unsupported\n");
}

void android_console_event(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_EVENT;

    if (helptext) {
        if (strstr(helptext, "types")) {
            cmd = CMD_EVENT_TYPES;
        } else if (strstr(helptext, "codes")) {
            cmd = CMD_EVENT_CODES;
        } else if (strstr(helptext, "send")) {
            cmd = CMD_EVENT_SEND;
        } else if (strstr(helptext, "text")) {
            cmd = CMD_EVENT_TEXT;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   event_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

enum {
    CMD_AVD,
    CMD_AVD_STOP,
    CMD_AVD_START,
    CMD_AVD_STATUS,
    CMD_AVD_NAME,
    CMD_AVD_SNAPSHOT,
    CMD_AVD_SNAPSHOT_LIST,
    CMD_AVD_SNAPSHOT_SAVE,
    CMD_AVD_SNAPSHOT_LOAD,
    CMD_AVD_SNAPSHOT_DEL,
};

static const char* avd_help[] = {
        /* CMD_AVD */
        "allows you to control (e.g. start/stop) the execution of the virtual "
        "device\n"
        "\n"
        "available sub-commands:\n"
        "   avd stop             stop the virtual device\n"
        "   avd start            start/restart the virtual device\n"
        "   avd status           query virtual device status\n"
        "   avd name             query virtual device name\n"
        "   avd snapshot         state snapshot commands\n",
        /* CMD_AVD_STOP */
        "'avd stop' stops the virtual device immediately, use 'avd start' to "
        "continue execution",
        /* CMD_AVD_START */
        "'avd start' will start or continue the virtual device, use 'avd stop' "
        "to "
        "stop it",
        /* CMD_AVD_STATUS */
        "'avd status' will indicate whether the virtual device is running or "
        "not",
        /* CMD_AVD_NAME */
        "'avd name' will return the name of this virtual device",
        /* CMD_AVD_SNAPSHOT */
        "allows you to save and restore the virtual device state in snapshots\n"
        "\n"
        "available sub-commands:\n"
        "   avd snapshot list             list available state snapshots\n"
        "   avd snapshot save             save state snapshot\n"
        "   avd snapshot load             load state snapshot\n"
        "   avd snapshot del              delete state snapshot\n",
        /* CMD_AVD_SNAPSHOT_LIST */
        "'avd snapshot list' will show a list of all state snapshots that can "
        "be "
        "loaded",
        /* CMD_AVD_SNAPSHOT_SAVE */
        "'avd snapshot save <name>' will save the current (run-time) state to "
        "a "
        "snapshot with the given name",
        /* CMD_AVD_SNAPSHOT_LOAD */
        "'avd snapshot load <name>' will load the state snapshot of the given "
        "name",
        /* CMD_AVD_SNAPSHOT_DEL */
        "'avd snapshot del <name>' will delete the state snapshot with the "
        "given "
        "name",
};

void android_console_avd_stop(Monitor* mon, const QDict* qdict) {
    if (!runstate_is_running()) {
        monitor_printf(mon, "KO: virtual device already stopped\n");
        return;
    }

    qmp_stop(NULL);

    monitor_printf(mon, "OK\n");
}

void android_console_avd_start(Monitor* mon, const QDict* qdict) {
    if (runstate_is_running()) {
        monitor_printf(mon, "KO: virtual device already running\n");
        return;
    }

    qmp_cont(NULL);

    monitor_printf(mon, "OK\n");
}

void android_console_avd_status(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon,
                   "virtual device is %s\n",
                   runstate_is_running() ? "running" : "stopped");

    monitor_printf(mon, "OK\n");
}

void android_console_avd_name(Monitor* mon, const QDict* qdict) {
#ifdef USE_ANDROID_EMU
    monitor_printf(mon, "%s\n", android_hw->avd_name);
#else
    monitor_printf(mon, "%s\n", "android");
#endif
    monitor_printf(mon, "OK\n");
}

void android_console_avd_snapshot(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the snapshot help message */
    int cmd = CMD_AVD_SNAPSHOT;

    if (helptext) {
        if (strstr(helptext, "list")) {
            cmd = CMD_AVD_SNAPSHOT_LIST;
        } else if (strstr(helptext, "save")) {
            cmd = CMD_AVD_SNAPSHOT_SAVE;
        } else if (strstr(helptext, "load")) {
            cmd = CMD_AVD_SNAPSHOT_LOAD;
        } else if (strstr(helptext, "del")) {
            cmd = CMD_AVD_SNAPSHOT_DEL;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   avd_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

void android_console_avd_snapshot_list(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: 'avd snapshot list' is currently unsupported\n");
}

void android_console_avd_snapshot_save(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: 'avd snapshot save' is currently unsupported\n");
}

void android_console_avd_snapshot_load(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: 'avd snapshot load' is currently unsupported\n");
}

void android_console_avd_snapshot_del(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: 'avd snapshot del' is currently unsupported\n");
}

void android_console_avd(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_AVD;

    if (helptext) {
        if (strstr(helptext, "stop")) {
            cmd = CMD_AVD_STOP;
        } else if (strstr(helptext, "start")) {
            cmd = CMD_AVD_START;
        } else if (strstr(helptext, "status")) {
            cmd = CMD_AVD_STATUS;
        } else if (strstr(helptext, "name")) {
            cmd = CMD_AVD_NAME;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   avd_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

enum { CMD_FINGER = 0, CMD_FINGER_TOUCH = 1, CMD_FINGER_REMOVE = 2 };

static const char* finger_help[] = {
        /* CMD_FINGER */
        "manage emulator fingerprint,"
        "allows you to touch the emulator fingerprint sensor\n"
        "\n"
        "available sub-commands:\n"
        "   finger touch           touch fingerprint sensor with <fingerid>\n"
        "   finger remove          remove finger from the fingerprint sensor\n",
        /* CMD_FINGER_TOUCH */
        "touch fingerprint sensor with <fingerid>",
        /* CMD_FINGER_REMOVE */
        "remove finger from the fingerprint sensor"};

void android_console_finger(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_FINGER;

    if (helptext) {
        if (strstr(helptext, "touch")) {
            cmd = CMD_FINGER_TOUCH;
        } else if (strstr(helptext, "remove")) {
            cmd = CMD_FINGER_REMOVE;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   finger_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

#ifdef USE_ANDROID_EMU
void android_console_finger_touch(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (!arg) {
        monitor_printf(mon, "KO: argument missing, try 'finger touch <id>'\n");
    } else {
        char* endptr;
        int fingerid = strtol(arg, &endptr, 0);
        if (endptr != arg) {
            _g_global.finger_agent.setTouch(true, fingerid);
            monitor_printf(mon, "OK\n");
        } else {
            monitor_printf(mon, "KO: invalid fingerid\n");
        }
    }
}

void android_console_finger_remove(Monitor* mon, const QDict* qdict) {
    _g_global.finger_agent.setTouch(false, -1);
    monitor_printf(mon, "OK\n");
}
#else /* not USE_ANDROID_EMU */
void android_console_finger_touch(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
void android_console_finger_remove(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
#endif

enum { CMD_SMS = 0, CMD_SMS_SEND = 1, CMD_SMS_PDU = 2 };

static const char* sms_help[] = {
        /* CMD_SMS */
        "SMS related commands, allows you to simulate an inbound SMS\n"
        "\n"
        "available sub-commands:\n"
        "   sms send               send inbound SMS text message\n"
        "   sms pdu                send inbound SMS text message\n",
        /* CMD_SMS_SEND */
        "send inbound SMS text message\n"
        "'sms send <phonenumber> <message>' allows you to simulate a new "
        "inbound sms message\n",
        /* CMD_SMS_PDU */
        "send inbound SMS PDU\n"
        "'sms pdu <hexstring>' allows you to simulate a new inbound sms PDU\n"
        "(used internally when one emulator sends SMS messages to another "
        "instance).\n"
        "you probably don't want to play with this at all\n"};

void android_console_sms(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_SMS;

    if (helptext) {
        if (strstr(helptext, "send")) {
            cmd = CMD_SMS_SEND;
        } else if (strstr(helptext, "pdu")) {
            cmd = CMD_SMS_PDU;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   sms_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

#ifdef USE_ANDROID_EMU
void android_console_sms_send(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    char* p;
    int textlen;
    SmsAddressRec sender;
    SmsPDU* pdus;
    int nn;

    /* check that we have a phone number made of digits */
    if (!args) {
        monitor_printf(mon,
                       "KO: missing argument, try 'sms send <phonenumber> "
                       "<text message>'\r\n");
        return;
    }
    p = strchr(args, ' ');
    if (!p) {
        monitor_printf(mon,
                       "KO: missing argument, try 'sms send <phonenumber> "
                       "<text message>'\r\n");
        return;
    }

    if (sms_address_from_str(&sender, args, p - args) < 0) {
        monitor_printf(mon,
                       "KO: bad phone number format, must be [+](0-9)*\r\n");
        return;
    }

    /* un-escape message text into proper utf-8 (conversion happens in-site) */
    p += 1;
    textlen = strlen(p);
    textlen = sms_utf8_from_message_str(p, textlen, (unsigned char*)p, textlen);
    if (textlen < 0) {
        monitor_printf(
                mon,
                "message must be utf8 and can use the following escapes:\r\n"
                "    \\n      for a newline\r\n"
                "    \\xNN    where NN are two hexadecimal numbers\r\n"
                "    \\uNNNN  where NNNN are four hexadecimal numbers\r\n"
                "    \\\\     to send a '\\' character\r\n\r\n"
                "    anything else is an error\r\n"
                "KO: badly formatted text\r\n");
        return;
    }

    if (!android_modem) {
        monitor_printf(mon, "KO: modem emulation not running\r\n");
        return;
    }

    /* create a list of SMS PDUs, then send them */
    pdus = smspdu_create_deliver_utf8((cbytes_t)p, textlen, &sender, NULL);
    if (pdus == NULL) {
        monitor_printf(mon,
                       "KO: internal error when creating SMS-DELIVER PDUs\n");
        return;
    }

    for (nn = 0; pdus[nn] != NULL; nn++)
        amodem_receive_sms(android_modem, pdus[nn]);

    smspdu_free_list(pdus);
    monitor_printf(mon, "OK\n");
}

void android_console_sms_pdu(Monitor* mon, const QDict* qdict) {
    SmsPDU pdu;
    char* args = (char*)qdict_get_try_str(qdict, "arg");

    /* check that we have a phone number made of digits */
    if (!args) {
        monitor_printf(
                mon, "KO: missing argument, try 'sms sendpdu <hexstring>'\r\n");
        return;
    }

    if (!android_modem) {
        monitor_printf(mon, "KO: modem emulation not running\r\n");
        return;
    }

    pdu = smspdu_create_from_hex(args, strlen(args));
    if (pdu == NULL) {
        monitor_printf(mon, "KO: badly formatted <hexstring>\r\n");
        return;
    }

    amodem_receive_sms(android_modem, pdu);
    smspdu_free(pdu);
    monitor_printf(mon, "OK\n");
}
#else
void android_console_sms_send(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
void android_console_sms_pdu(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
#endif

enum { CMD_CDMA = 0, CMD_CDMA_SSOURCE = 1, CMD_CDMA_PRL_VERSION = 2 };

static const char* cdma_help[] = {
        /* CMD_CDMA */
        "CDMA related commands, allows you to change CDMA-related settings\n"
        "\n"
        "available sub-commands:\n"
        "   cdma ssource           set the current CDMA subscription source\n"
        "   cdma prl_version       dump the current PRL version\n",
        /* CMD_CDMA_SSOURCE */
        "set the current CDMA subscription source\n"
        "'cdma ssource <ssource>' allows you to specify where to read the "
        "subscription from\n",
        /* CMD_CDMA_PRL_VERSION */
        "dump the current PRL version\n"
        "'cdma prl_version <version>' allows you to dump the current PRL "
        "version\n"};

void android_console_cdma(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_CDMA;

    if (helptext) {
        if (strstr(helptext, "ssource")) {
            cmd = CMD_CDMA_SSOURCE;
        } else if (strstr(helptext, "prl_version")) {
            cmd = CMD_CDMA_PRL_VERSION;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   cdma_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

#ifdef USE_ANDROID_EMU
static const struct {
    const char* name;
    const char* display;
    ACdmaSubscriptionSource source;
} _cdma_subscription_sources[] = {
          {"nv",
           "Read subscription from non-volatile RAM",
           A_SUBSCRIPTION_NVRAM},
          {"ruim", "Read subscription from RUIM", A_SUBSCRIPTION_RUIM},
};

void android_console_cdma_ssource(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    int nn;
    if (!args) {
        monitor_printf(mon,
                       "KO: missing argument, try 'cdma ssource <source>'\n");
        return;
    }

    for (nn = 0; nn < sizeof(_cdma_subscription_sources) /
                                 sizeof(_cdma_subscription_sources[0]);
         nn++) {
        const char* name = _cdma_subscription_sources[nn].name;
        ACdmaSubscriptionSource ssource = _cdma_subscription_sources[nn].source;

        if (!name)
            break;

        if (!strcasecmp(args, name)) {
            amodem_set_cdma_subscription_source(android_modem, ssource);
            monitor_printf(mon, "OK\n");
            return;
        }
    }
    monitor_printf(mon, "KO: Don't know source %s\n", args);
}

void android_console_cdma_prl_version(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    int version = 0;
    char* endptr;

    if (!args) {
        monitor_printf(
                mon,
                "KO: missing argument, try 'cdma prl_version <version>'\r\n");
        return;
    }

    version = strtol(args, &endptr, 0);
    if (endptr != args) {
        amodem_set_cdma_prl_version(android_modem, version);
    }
    monitor_printf(mon, "OK\n");
}
#else
void android_console_cdma_ssource(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_cdma_prl_version(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

#endif

enum {
    CMD_GSM = 0,
    CMD_GSM_LIST = 1,
    CMD_GSM_CALL = 2,
    CMD_GSM_BUSY = 3,
    CMD_GSM_HOLD = 4,
    CMD_GSM_ACCEPT = 5,
    CMD_GSM_CANCEL = 6,
    CMD_GSM_DATA = 7,
    CMD_GSM_VOICE = 8,
    CMD_GSM_STATUS = 9,
    CMD_GSM_SIGNAL = 10
};

static const char* gsm_help[] = {
        /* CMD_GSM */
        "GSM related commands,"
        "allows you to change GSM-related settings, or to make new inbound "
        "phone call\n"
        "\n"
        "available sub-commands:\n"
        "   gsm list               list current phone calls\n"
        "   gsm call               create inbound phone call\n"
        "   gsm busy               close waiting outbound call as busy\n"
        "   gsm hold               change the state of an outbound call to "
        "'held'\n"
        "   gsm accept             change the state of an outbound call to "
        "'active'\n"
        "   gsm cancel             disconnect an inbound or outbound phone "
        "call\n"
        "   gsm data               modify data connection state\n"
        "   gsm voice              modify voice connection state\n"
        "   gsm status             display GSM state\n"
        "   gsm signal             sets the rssi and ber\n",
        /* CMD_GSM_LIST */
        "list current phone calls\n"
        "'gsm list' lists all inbound and outbound calls and their state\n",
        /* CMD_GSM_CALL */
        "create inbound phone call\n"
        "'gsm call <phonenumber>' allows you to simulate a new inbound call\n",
        /* CMD_GSM_BUSY */
        "close waiting outbound call as busy\n"
        "'gsm busy <phonenumber>' closes an outbound call, reporting\n"
        "the remote phone as busy. only possible if the call is 'waiting'.\n",
        /* CMD_GSM_HOLD */
        "change the state of an outbound call to 'held'\n"
        "'gsm hold <remoteNumber>' change the state of a call to 'held'. this "
        "is only possible\n"
        "if the call in the 'waiting' or 'active' state\n",
        /* CMD_GSM_ACCEPT */
        "change the state of an outbound call to 'active'\n"
        "'gsm accept <remoteNumber>' change the state of a call to 'active'. "
        "this is only possible\n"
        "if the call is in the 'waiting' or 'held' state\n",
        /* CMD_GSM_CANCEL */
        "disconnect an inbound or outbound phone call\n"
        "'gsm cancel <phonenumber>' allows you to simulate the end of an "
        "inbound or outbound call\n",
        /* CMD_GSM_DATA */
        "modify data connection state\n"
        "'gsm data <state>' allows you to modify the data connection state\n",
        /* CMD_GSM_VOICE */
        "modify voice connection state\n"
        "'gsm voice <state>' allows you to modify the voice connection state\n",
        /* CMD_GSM_STATUS */
        "display GSM state\n"
        "'gsm status' displays the current state of the GSM emulation\n",
        /* CMD_GSM_SIGNAL */
        "sets the rssi and ber\n"
        "'gsm signal <rssi> [<ber>]' changes the reported strength and error "
        "rate on next (15s) update.\n"
        "rssi range is 0..31 and 99 for unknown\n"
        "ber range is 0..7 percent and 99 for unknown\n",
};

void android_console_gsm(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_GSM;

    if (helptext) {
        if (strstr(helptext, "list")) {
            cmd = CMD_GSM_LIST;
        } else if (strstr(helptext, "call")) {
            cmd = CMD_GSM_CALL;
        } else if (strstr(helptext, "busy")) {
            cmd = CMD_GSM_BUSY;
        } else if (strstr(helptext, "hold")) {
            cmd = CMD_GSM_HOLD;
        } else if (strstr(helptext, "accept")) {
            cmd = CMD_GSM_ACCEPT;
        } else if (strstr(helptext, "cancel")) {
            cmd = CMD_GSM_CANCEL;
        } else if (strstr(helptext, "data")) {
            cmd = CMD_GSM_DATA;
        } else if (strstr(helptext, "voice")) {
            cmd = CMD_GSM_VOICE;
        } else if (strstr(helptext, "status")) {
            cmd = CMD_GSM_STATUS;
        } else if (strstr(helptext, "signal")) {
            cmd = CMD_GSM_SIGNAL;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   gsm_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

#ifdef USE_ANDROID_EMU
static const struct {
    const char* name;
    const char* display;
    ARegistrationState state;
} _gsm_states[] = {
          {"unregistered", "no network available", A_REGISTRATION_UNREGISTERED},
          {"home", "on local network, non-roaming", A_REGISTRATION_HOME},
          {"roaming", "on roaming network", A_REGISTRATION_ROAMING},
          {"searching", "searching networks", A_REGISTRATION_SEARCHING},
          {"denied", "emergency calls only", A_REGISTRATION_DENIED},
          {"off", "same as 'unregistered'", A_REGISTRATION_UNREGISTERED},
          {"on", "same as 'home'", A_REGISTRATION_HOME},
          {NULL, NULL, A_REGISTRATION_UNREGISTERED}};

static const char* gsm_state_to_string(ARegistrationState state) {
    int nn;
    for (nn = 0; _gsm_states[nn].name != NULL; nn++) {
        if (state == _gsm_states[nn].state)
            return _gsm_states[nn].name;
    }
    return "<unknown>";
}

static const char* call_state_to_string(ACallState state) {
    switch (state) {
        case A_CALL_ACTIVE:
            return "active";
        case A_CALL_HELD:
            return "held";
        case A_CALL_ALERTING:
            return "ringing";
        case A_CALL_WAITING:
            return "waiting";
        case A_CALL_INCOMING:
            return "incoming";
        default:
            return "unknown";
    }
}

void android_console_gsm_list(Monitor* mon, const QDict* qdict) {
    /* check that we have a phone number made of digits */
    int count = amodem_get_call_count(android_modem);
    int nn;
    for (nn = 0; nn < count; nn++) {
        ACall call = amodem_get_call(android_modem, nn);
        const char* dir;

        if (call == NULL)
            continue;

        if (call->dir == A_CALL_OUTBOUND)
            dir = "outbound to ";
        else
            dir = "inbound from";

        monitor_printf(mon,
                       "%s %-10s : %s\r\n",
                       dir,
                       call->number,
                       call_state_to_string(call->state));
    }
    monitor_printf(mon, "OK\n");
}

static int gsm_check_number(char* args) {
    int nn;

    for (nn = 0; args[nn] != 0; nn++) {
        int c = args[nn];
        if (!isdigit(c) && c != '+' && c != '#') {
            return -1;
        }
    }
    if (nn == 0)
        return -1;

    return 0;
}

void android_console_gsm_call(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    /* check that we have a phone number made of digits */
    if (!args) {
        monitor_printf(
                mon, "KO: missing argument, try 'gsm call <phonenumber>'\r\n");
        return;
    }

    if (gsm_check_number(args)) {
        monitor_printf(
                mon,
                "KO: bad phone number format, use digits, # and + only\r\n");
        return;
    }

    if (!android_modem) {
        monitor_printf(mon, "KO: modem emulation not running\r\n");
        return;
    }
    amodem_add_inbound_call(android_modem, args);
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_busy(Monitor* mon, const QDict* qdict) {
    ACall call;

    char* args = (char*)qdict_get_try_str(qdict, "arg");
    if (!args) {
        monitor_printf(
                mon, "KO: missing argument, try 'gsm busy <phonenumber>'\r\n");
        return;
    }
    call = amodem_find_call_by_number(android_modem, args);
    if (call == NULL || call->dir != A_CALL_OUTBOUND) {
        monitor_printf(
                mon,
                "KO: no current outbound call to number '%s' (call %p)\r\n",
                args,
                call);
        return;
    }
    if (amodem_disconnect_call(android_modem, args) < 0) {
        monitor_printf(mon, "KO: could not cancel this number\r\n");
        return;
    }
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_hold(Monitor* mon, const QDict* qdict) {
    ACall call;

    char* args = (char*)qdict_get_try_str(qdict, "arg");
    if (!args) {
        monitor_printf(
                mon,
                "KO: missing argument, try 'gsm out hold <phonenumber>'\r\n");
        return;
    }
    call = amodem_find_call_by_number(android_modem, args);
    if (call == NULL) {
        monitor_printf(
                mon, "KO: no current call to/from number '%s'\r\n", args);
        return;
    }
    if (amodem_update_call(android_modem, args, A_CALL_HELD) < 0) {
        monitor_printf(mon, "KO: could put this call on hold\r\n");
        return;
    }
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_accept(Monitor* mon, const QDict* qdict) {
    ACall call;

    char* args = (char*)qdict_get_try_str(qdict, "arg");
    if (!args) {
        monitor_printf(
                mon,
                "KO: missing argument, try 'gsm accept <phonenumber>'\r\n");
        return;
    }
    call = amodem_find_call_by_number(android_modem, args);
    if (call == NULL) {
        monitor_printf(
                mon, "KO: no current call to/from number '%s'\r\n", args);
        return;
    }
    if (amodem_update_call(android_modem, args, A_CALL_ACTIVE) < 0) {
        monitor_printf(mon, "KO: could not activate this call\r\n");
        return;
    }
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_cancel(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    if (!args) {
        monitor_printf(
                mon, "KO: missing argument, try 'gsm call <phonenumber>'\r\n");
        return;
    }
    if (gsm_check_number(args)) {
        monitor_printf(
                mon,
                "KO: bad phone number format, use digits, # and + only\r\n");
        return;
    }
    if (!android_modem) {
        monitor_printf(mon, "KO: modem emulation not running\r\n");
        return;
    }
    if (amodem_disconnect_call(android_modem, args) < 0) {
        monitor_printf(mon, "KO: could not cancel this number\r\n");
        return;
    }
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_data(Monitor* mon, const QDict* qdict) {
    int nn;
    monitor_printf(mon,
                   "the 'gsm data <state>' allows you to change the state of "
                   "your GPRS connection\r\n"
                   "valid values for <state> are the following:\r\n\r\n");
    for (nn = 0;; nn++) {
        const char* name = _gsm_states[nn].name;
        const char* display = _gsm_states[nn].display;

        if (!name)
            break;

        monitor_printf(mon, "  %-15s %s\r\n", name, display);
    }
    monitor_printf(mon, "\r\n");
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_voice(Monitor* mon, const QDict* qdict) {
    int nn;
    monitor_printf(mon,
                   "the 'gsm voice <state>' allows you to change the state of "
                   "your GPRS connection\r\n"
                   "valid values for <state> are the following:\r\n\r\n");
    for (nn = 0;; nn++) {
        const char* name = _gsm_states[nn].name;
        const char* display = _gsm_states[nn].display;

        if (!name)
            break;

        monitor_printf(mon, "  %-15s %s\r\n", name, display);
    }
    monitor_printf(mon, "\r\n");
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_status(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    if (args) {
        monitor_printf(mon, "KO: no argument required\r\n");
        return;
    }
    if (!android_modem) {
        monitor_printf(mon, "KO: modem emulation not running\r\n");
        return;
    }
    monitor_printf(
            mon,
            "gsm voice state: %s\r\n",
            gsm_state_to_string(amodem_get_voice_registration(android_modem)));
    monitor_printf(
            mon,
            "gsm data state:  %s\r\n",
            gsm_state_to_string(amodem_get_data_registration(android_modem)));
    monitor_printf(mon, "OK\n");
}

void android_console_gsm_signal(Monitor* mon, const QDict* qdict) {
    char* args = (char*)qdict_get_try_str(qdict, "arg");
    enum { SIGNAL_RSSI = 0, SIGNAL_BER, NUM_SIGNAL_PARAMS };
    char* p = args;
    int top_param = -1;
    int params[NUM_SIGNAL_PARAMS];

    static int last_ber = 99;

    if (!p)
        p = "";

    /* tokenize */
    while (*p) {
        char* end;
        int val = strtol(p, &end, 10);

        if (end == p) {
            monitor_printf(mon, "KO: argument '%s' is not a number\n", p);
            return;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_SIGNAL_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < SIGNAL_RSSI) {
        monitor_printf(mon,
                       "KO: not enough arguments: see 'help gsm signal' for "
                       "details\r\n");
        return;
    }

    int rssi = params[SIGNAL_RSSI];
    if ((rssi < 0 || rssi > 31) && rssi != 99) {
        monitor_printf(mon, "KO: invalid RSSI - must be 0..31 or 99\r\n");
        return;
    }

    /* check ber is 0..7 or 99 */
    if (top_param >= SIGNAL_BER) {
        int ber = params[SIGNAL_BER];
        if ((ber < 0 || ber > 7) && ber != 99) {
            monitor_printf(mon, "KO: invalid BER - must be 0..7 or 99\r\n");
            return;
        }
        last_ber = ber;
    }

    amodem_set_signal_strength(android_modem, rssi, last_ber);

    monitor_printf(mon, "OK\n");
}

#else
void android_console_gsm_list(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_call(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_busy(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_hold(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_accept(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_cancel(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_data(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_voice(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_status(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}

void android_console_gsm_signal(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
#endif

enum { CMD_GEO = 0, CMD_GEO_NMEA = 1, CMD_GEO_FIX = 2 };

static const char* geo_help[] = {
        /* CMD_GEO */
        "Geo-location commands,"
        "allows you to change Geo-related settings, or to send GPS NMEA "
        "sentences\n"
        "\n"
        "available sub-commands:\n"
        "   geo nmea               send a GPS NMEA sentence\n"
        "   geo fix                send a simple GPS fix\n",
        /* CMD_GEO_NMEA */
        "send a GPS NMEA sentence\n"
        "'geo nema <sentence>' sends an NMEA 0183 sentence to the emulated "
        "device, as\n"
        "if it came from an emulated GPS modem. <sentence> must begin with "
        "'$GP'. Only\n"
        "'$GPGGA' and '$GPRCM' sentences are supported at the moment.",
        /* CMD_GEO_FIX */
        "send a simple GPS fix\n"
        "'geo fix <longitude> <latitude> [<altitude> [<satellites>]]'\n"
        "allows you to send a simple GPS fix to the emulated system.\n"
        "The parameters are:\n\n"
        "   <longitude>   longitude, in decimal degrees\n"
        "   <latitude>    latitude, in decimal degrees\n"
        "   <altitude>    optional altitude in meters\n"
        "   <satellites>  number of satellites being tracked (1-12)"};

void android_console_geo(Monitor* mon, const QDict* qdict) {
    /* This only gets called for bad subcommands and help requests */
    const char* helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_GEO;

    if (helptext) {
        if (strstr(helptext, "nmea")) {
            cmd = CMD_GEO_NMEA;
        } else if (strstr(helptext, "fix")) {
            cmd = CMD_GEO_FIX;
        }
    }

    /* If this is not a help request then we are here with a bad sub-command */
    monitor_printf(mon,
                   "%s\n%s\n",
                   geo_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

#ifdef USE_ANDROID_EMU
void android_console_geo_nmea(Monitor* mon, const QDict* qdict) {
    const char* arg = qdict_get_try_str(qdict, "arg");

    if (!arg) {
        monitor_printf(mon, "KO: argument missing, try 'geo nmea <mesg>'\n");
    } else if (_g_global.location_agent.gpsIsSupported()) {
        _g_global.location_agent.gpsSendNmea(arg);
        monitor_printf(mon, "OK\n");
    } else {
        monitor_printf(mon, "KO: no GPS emulation in this virtual device\n");
    }
}

void android_console_geo_fix(Monitor* mon, const QDict* qdict) {
    /* GEO_SAT2 provides bug backwards compatibility. */
    enum { GEO_LONG = 0, GEO_LAT, GEO_ALT, GEO_SAT, GEO_SAT2, NUM_GEO_PARAMS };
    const char* arg = qdict_get_try_str(qdict, "arg");
    char* p = (char*)arg;
    int top_param = -1;
    double altitude;
    double params[NUM_GEO_PARAMS];
    int n_satellites = 1;

    struct timeval tVal;

    if (!p)
        p = "";

    /* tokenize */
    while (*p) {
        char* end;
        double val = strtod(p, &end);

        if (end == p) {
            monitor_printf(mon, "KO: argument '%s' is not a number\n", p);
            return;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_GEO_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < GEO_LAT) {
        monitor_printf(
                mon,
                "KO: not enough arguments: see 'help geo fix' for details\n");
        return;
    }

    /* check number of satellites, must be integer between 1 and 12 */
    if (top_param >= GEO_SAT) {
        int sat_index = (top_param >= GEO_SAT2) ? GEO_SAT2 : GEO_SAT;
        n_satellites = (int)params[sat_index];
        if (n_satellites != params[sat_index] || n_satellites < 1 ||
            n_satellites > 12) {
            monitor_printf(
                    mon,
                    "KO: invalid number of satellites. Must be an integer "
                    "between 1 and 12\n");
            return;
        }
    }

    altitude = 0.0;
    if (top_param < GEO_ALT) {
        altitude = params[GEO_ALT];
    }

    memset(&tVal, 0, sizeof(tVal));
    gettimeofday(&tVal, NULL);

    _g_global.location_agent.gpsCmd(
            params[GEO_LAT], params[GEO_LONG], altitude, n_satellites, &tVal);
    monitor_printf(mon, "OK\n");
}
#else /* not USE_ANDROID_EMU */
void android_console_geo_nmea(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
void android_console_geo_fix(Monitor* mon, const QDict* qdict) {
    monitor_printf(mon, "KO: emulator not built with USE_ANDROID_EMU\n");
}
#endif
