/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef QEMU_NET_SLIRP_H
#define QEMU_NET_SLIRP_H


#ifdef CONFIG_SLIRP

void hmp_hostfwd_add(Monitor *mon, const QDict *qdict);
void hmp_hostfwd_remove(Monitor *mon, const QDict *qdict);
void hmp_ipv6_hostfwd_add(Monitor *mon, const QDict *qdict);
void hmp_ipv6_hostfwd_remove(Monitor *mon, const QDict *qdict);

int net_slirp_redir(const char *redir_str);

int net_slirp_smb(const char *exported_dir);

void hmp_info_usernet(Monitor *mon, const QDict *qdict);

/* Functions to be called by |out_send| and |in_send| implementations below */
/* |opaque| is the result of net_slirp_set_shapers(). */
void net_slirp_output_raw(void *opaque, const uint8_t *pkt, int pkt_len);
void net_slirp_receive_raw(void* opaque, const uint8_t *buf, size_t size);

/* Return a Slirp instance, or NULL if the network stack is not initialized */
void* net_slirp_state(void);

typedef void (*SlirpShaperSendFunc)(void* opaque, const void* data, int len, void* slirp_state);

void net_slirp_set_shapers(void* out_opaque,
                            SlirpShaperSendFunc out_send,
                            void* in_opaque,
                            SlirpShaperSendFunc in_send);

#endif

#endif /* QEMU_NET_SLIRP_H */
