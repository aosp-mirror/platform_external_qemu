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

typedef struct {
    int is_udp;
    int host_port;
    int guest_port;
} RedirRec;

GList *redir_list;

void android_monitor_print_error(Monitor *mon, const char *fmt, ...)
{
    /* Print an error (typically a syntax error from the parser), with
     * the required "KO: " prefix.
     */
    va_list ap;

    monitor_printf(mon, "KO: ");
    va_start(ap, fmt);
    monitor_vprintf(mon, fmt, ap);
    va_end(ap);
}

void android_console_kill(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "OK: killing emulator, bye bye\n");
    monitor_suspend(mon);
    qmp_quit(NULL);
}

void android_console_quit(Monitor *mon, const QDict *qdict)
{
    /* Don't print an OK response for success, just close the connection */
    if (monitor_disconnect(mon)) {
        monitor_printf(mon, "KO: this connection doesn't support quitting\n");
    }
}

#ifdef CONFIG_SLIRP
void android_console_redir_list(Monitor *mon, const QDict *qdict)
{
    if (!redir_list) {
        monitor_printf(mon, "no active redirections\n");
    } else {
        GList *l;

        for (l = redir_list; l; l = l->next) {
            RedirRec *r = l->data;

            monitor_printf(mon, "%s:%-5d => %-5d\n", r->is_udp ? "udp" : "tcp",
                           r->host_port, r->guest_port);
        }
    }
    monitor_printf(mon, "OK\n");
}

static int parse_proto(const char *s)
{
    if (!strcmp(s, "tcp")) {
        return 0;
    } else if (!strcmp(s, "udp")) {
        return 1;
    } else {
        return -1;
    }
}

static int parse_port(const char *s)
{
    char *end;
    int port;

    port = strtol(s, &end, 10);
    if (*end != 0 || port < 1 || port > 65535) {
        return 0;
    }
    return port;
}

void android_console_redir_add(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_str(qdict, "arg");
    char **tokens;
    int is_udp, host_port, guest_port;
    Slirp *slirp;
    Error *err = NULL;
    struct in_addr host_addr = { .s_addr = htonl(INADDR_LOOPBACK) };
    struct in_addr guest_addr = { .s_addr = 0 };
    RedirRec *redir;

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

    if (slirp_add_hostfwd(slirp, is_udp, host_addr, host_port,
                          guest_addr, guest_port) < 0) {
        monitor_printf(mon, "KO: can't setup redirection, "
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
    monitor_printf(mon, "KO: bad redirection format, try "
                   "(tcp|udp):hostport:guestport\n");
    g_strfreev(tokens);
}

static gint redir_cmp(gconstpointer a, gconstpointer b)
{
    const RedirRec *ra = a;
    const RedirRec *rb = b;

    /* For purposes of list deletion, only protocol and host port matter */
    if (ra->is_udp != rb->is_udp) {
        return ra->is_udp - rb->is_udp;
    }
    return ra->host_port - rb->host_port;
}

void android_console_redir_del(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_str(qdict, "arg");
    char **tokens;
    int is_udp, host_port;
    Slirp *slirp;
    Error *err = NULL;
    struct in_addr host_addr = { .s_addr = INADDR_ANY };
    RedirRec rr;
    GList *entry;

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
        monitor_printf(mon, "KO: can't remove unknown redirection (%s:%d)\n",
                       is_udp ? "udp" : "tcp", host_port);
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
void android_console_redir_list(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}

void android_console_redir_add(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}

void android_console_redir_remove(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "KO: emulator not built with CONFIG_SLIRP\n");
}
#endif

enum {
    CMD_REDIR,
    CMD_REDIR_LIST,
    CMD_REDIR_ADD,
    CMD_REDIR_DEL,
};

static const char *redir_help[] = {
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
    "list current port redirections. use 'redir add' and 'redir del' to add "
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
    "see the 'help redir add' for the meaning of <protocol> and <host-port>",
};

void android_console_redir(Monitor *mon, const QDict *qdict)
{
    /* This only gets called for bad subcommands and help requests */
    const char *helptext = qdict_get_try_str(qdict, "helptext");

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
    monitor_printf(mon, "%s\n%s\n", redir_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");
}

void android_console_power_display(Monitor *mon, const QDict *qdict)
{
    goldfish_battery_display(mon);

    monitor_printf(mon, "OK\n");
}

void android_console_power_ac(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_try_str(qdict, "arg");

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

void android_console_power_status(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "unknown") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_UNKNOWN);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "charging") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_CHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "discharging") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_DISCHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "not-charging") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_NOT_CHARGING);
            monitor_printf(mon, "OK\n");
            return;
        } else if (strcasecmp(arg, "full") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS,
                                      POWER_SUPPLY_STATUS_FULL);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon, "KO: Usage: \"status unknown|charging|"
                   "discharging|not-charging|full\"\n");
}

void android_console_power_present(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_try_str(qdict, "arg");

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

void android_console_power_health(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_try_str(qdict, "arg");

    if (arg) {
        if (strcasecmp(arg, "unknown") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_UNKNOWN);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "good") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_GOOD);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "overheat") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_OVERHEAT);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "dead") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_DEAD);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "overvoltage") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_OVERVOLTAGE);
            monitor_printf(mon, "OK\n");
            return;
        }
        if (strcasecmp(arg, "failure") == 0) {
            goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH,
                                      POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
            monitor_printf(mon, "OK\n");
            return;
        }
    }

    monitor_printf(mon, "KO: Usage: \"health unknown|good|overheat|"
                   "dead|overvoltage|failure\"\n");
}

enum {
    CMD_POWER,
    CMD_POWER_DISPLAY,
    CMD_POWER_AC,
    CMD_POWER_STATUS,
    CMD_POWER_PRESENT,
    CMD_POWER_HEALTH,
};

static const char *power_help[] = {
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
};

void android_console_power(Monitor *mon, const QDict *qdict)
{
    /* This only gets called for bad subcommands and help requests */
    const char *helptext = qdict_get_try_str(qdict, "helptext");

    /* Default to the first entry which is the parent help message */
    int cmd = CMD_POWER;

    if (helptext) {
        if (strstr(helptext, "display")) {
            cmd = CMD_POWER_DISPLAY;
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
    monitor_printf(mon, "%s\n%s\n", power_help[cmd],
                   helptext ? "OK" : "KO: missing sub-command");

}
