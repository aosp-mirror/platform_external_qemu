#ifndef _LIBSLIRP_H
#define _LIBSLIRP_H

#include "qemu-common.h"

int is_dns_addr(Slirp* slirp, const struct in_addr* guest_addr);

int get_dns_addr(const struct in_addr* guest_addr, struct in_addr* host_addr);

int slirp_get_system_dns_servers(void);

int slirp_parse_dns_servers(const char* servers);

int slirp_get_max_dns_servers(void);

Slirp *slirp_init(int restricted, struct in_addr vnetwork,
                  struct in_addr vnetmask, struct in_addr vhost,
                  const char *vhostname, const char *tftp_path,
                  const char *bootfile, struct in_addr vdhcp_start,
                  struct in_addr vnameserver, const char **vdnssearch,
                  void *opaque);
void slirp_cleanup(Slirp *slirp);

void slirp_pollfds_fill(GArray *pollfds, uint32_t *timeout);

void slirp_pollfds_poll(GArray *pollfds, int select_error);

void slirp_input(Slirp *slirp, const uint8_t *pkt, int pkt_len);

/* you must provide the following functions: */
void slirp_output(void *opaque, const uint8_t *pkt, int pkt_len);

int slirp_add_hostfwd(Slirp *slirp, int is_udp,
                      struct in_addr host_addr, int host_port,
                      struct in_addr guest_addr, int guest_port);
int slirp_remove_hostfwd(Slirp *slirp, int is_udp,
                         struct in_addr host_addr, int host_port);
int slirp_add_exec(Slirp *slirp, int do_pty, const void *args,
                   struct in_addr *guest_addr, int guest_port);

void slirp_connection_info(Slirp *slirp, Monitor *mon);

void slirp_socket_recv(Slirp *slirp, struct in_addr guest_addr,
                       int guest_port, const uint8_t *buf, int size);
size_t slirp_socket_can_recv(Slirp *slirp, struct in_addr guest_addr,
                             int guest_port);

#endif
