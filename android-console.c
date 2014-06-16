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

typedef struct {
    int is_udp;
    int host_port;
    int guest_port;
} RedirRec;

GList *redir_list;

void android_console_kill(Monitor *mon, const QDict *qdict)
{
    monitor_printf(mon, "OK: killing emulator, bye bye\n");
    monitor_suspend(mon);
    qmp_quit(NULL);
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

void android_console_redir(Monitor *mon, const QDict *qdict)
{
    const char *arg = qdict_get_try_str(qdict, "arg");

    if (!arg) {
        goto fail;
    }

    monitor_printf(mon, "redir: arg %s\n", arg);
    return;

fail:
    monitor_printf(mon, "allows you to add, list and remove and/or "
                   "PORT redirection from the host to the device\n"
                   "as an example, 'redir  tcp:5000:6000' will route "
                   "any packet sent to the host's TCP port 5000\n"
                   "to TCP port 6000 of the emulated device\n"
                   "\n"
                   "available sub-commands:\n"
                   "    list             list current redirections\n"
                   "    add              add new redirection\n"
                   "    del              remove existing redirection\n"
                   "\n"
                   "KO: missing sub-command\n");
}
