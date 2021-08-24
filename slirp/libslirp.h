#ifndef LIBSLIRP_H
#define LIBSLIRP_H

#include "qemu-common.h"

typedef struct Slirp Slirp;

int get_dns6_addr(struct in6_addr *pdns6_addr, uint32_t *scope_id);
Slirp *slirp_init(int restricted, bool in_enabled, struct in_addr vnetwork,
                  struct in_addr vnetmask, struct in_addr vhost,
                  bool in6_enabled,
                  struct in6_addr vprefix_addr6, uint8_t vprefix_len,
                  struct in6_addr vhost6, const char *vhostname,
                  const char *tftp_path, const char *bootfile,
                  struct in_addr vdhcp_start, struct in_addr vnameserver,
                  struct in6_addr vnameserver6, const char **vdnssearch,
                  void *opaque);
void slirp_cleanup(Slirp *slirp);

/* Setup custom DNS servers to be used. |dns_servers| is an array of
 * |num_dns_servers| IPv4 host addresses for DNS servers to be used
 * when translating guest IPv4 DNS addresses (typically from 10.0.2.3
 * to 10.0.2.(3 + num_dns_servers - 1). */
void slirp_init_custom_dns_servers(Slirp *slirp,
                                   const struct sockaddr_storage *dns_servers,
                                   int num_dns_servers);


/* Check whether |guest_ip| is the IPv4 address of a guest DNS server.
 * If that's the case, set |*host_ip| to the corresponding host DNS server
 * address and return 0. Otherwise, leave |host_ip| untouched and
 * return -1. NOTE: |*host_ip| can be an IPv6 host address in certain
 */
int slirp_translate_guest_dns(Slirp *slirp,
                              const struct sockaddr_in *guest_ip,
                              struct sockaddr_storage *host_ip);

/* Same for IPv6 addresses */
int slirp_translate_guest_dns6(Slirp *slirp,
                               const struct sockaddr_in6 *guest_ip,
                               struct sockaddr_storage *host_ip);

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
int slirp_add_ipv6_hostfwd(Slirp *slirp, int is_udp,
                           struct in6_addr host_addr, int host_port,
                           struct in6_addr guest_addr, int guest_port);
int slirp_remove_ipv6_hostfwd(Slirp *slirp, int is_udp, struct in6_addr host_addr, int host_port);
int slirp_add_exec(Slirp *slirp, int do_pty, const void *args,
                   struct in_addr *guest_addr, int guest_port);

void slirp_connection_info(Slirp *slirp, Monitor *mon);

void slirp_socket_recv(Slirp *slirp, struct in_addr guest_addr,
                       int guest_port, const uint8_t *buf, int size);
size_t slirp_socket_can_recv(Slirp *slirp, struct in_addr guest_addr,
                             int guest_port);

void slirp_set_cleanup_ip_on_load(bool enable);

void slirp_block_src_ethaddr(const uint8_t *ethaddr, bool enable);

#endif
