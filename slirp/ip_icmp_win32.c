/**
 * slirp ping support - Windows ICMP API based ping proxy.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 * Copyright (C) 2017 Google Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "slirp.h"
#include "ip_icmp.h"

#ifndef PIO_APC_ROUTINE_DEFINED
#define PIO_APC_ROUTINE_DEFINED 1
#endif

typedef long NTSTATUS;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID(NTAPI* PIO_APC_ROUTINE)(IN PVOID ApcContext,
                                     IN PIO_STATUS_BLOCK IoStatusBlock,
                                     IN ULONG Reserved);

#include <icmpapi.h>
#include <iphlpapi.h>

/*
 * A header of ICMP ECHO.  Intended for storage, unlike struct icmp
 * which is intended to be overlayed onto a buffer.
 */
struct icmp_echo {
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_echo_id;
    uint16_t icmp_echo_seq;
};

struct pong {
    Slirp* slirp;

    TAILQ_ENTRY(pong) queue_entry;

    struct ip reqiph;
    struct icmp_echo reqicmph;

    size_t bufsize;
    uint8_t buf[1];
};

static VOID WINAPI icmpwin_callback_apc(void* ctx,
                                        PIO_STATUS_BLOCK iob,
                                        ULONG reserved);
static VOID WINAPI icmpwin_callback_old(void* ctx);

static void icmpwin_callback(struct pong* pong);
static void icmpwin_pong(struct pong* pong);

static struct mbuf* icmpwin_get_error(struct pong* pong, int type, int code);
static struct mbuf* icmpwin_get_mbuf(Slirp* slirp, size_t reqsize);

/*
 * On Windows XP and Windows Server 2003 IcmpSendEcho2() callback
 * is FARPROC, but starting from Vista it's PIO_APC_ROUTINE with
 * two extra arguments.  Callbacks use WINAPI (stdcall) calling
 * convention with callee responsible for popping the arguments,
 * so to avoid stack corruption we check windows version at run
 * time and provide correct callback.
 *
 */
static PIO_APC_ROUTINE g_pfnIcmpCallback;

/*
 * Append the specified data to the indicated mbuf chain,
 * Extend the mbuf chain if the new data does not fit in
 * existing space.
 *
 * Return 1 if able to complete the job; otherwise 0.
 */
static int m_append(Slirp* slirp, struct mbuf* m0, int len, caddr_t cp) {
    struct mbuf *m, *n;
    int remainder, space;

    /* this causes infinite loop
    for (m = m0; m->m_next != NULL; m = m->m_next)
            ;
    */
    m = m0;
    remainder = len;
    space = M_TRAILINGSPACE(m);
    if (space > 0) {
        /*
         * Copy into available space.
         */
        if (space > remainder)
            space = remainder;
        memcpy(mtod(m, caddr_t) + m->m_len, cp, space);
        m->m_len += space;
        cp += space, remainder -= space;
    }
    while (remainder > 0) {
        /*
         * Allocate a new mbuf; could check space
         * and allocate a cluster instead.
         */
        n = m_get(slirp);

        if (n == NULL)
            break;
        n->m_len = min(MINCSIZE, remainder);
        memcpy(mtod(n, caddr_t), cp, n->m_len);
        cp += n->m_len, remainder -= n->m_len;
        m->m_next = n;
        m = n;
    }

    return (remainder == 0);
}

int icmpwin_init(Slirp* slirp) {
    if (g_pfnIcmpCallback == NULL) {
        OSVERSIONINFO osvi;
        int status;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        status = GetVersionEx(&osvi);
        if (status == 0)
            return 1;

        if (osvi.dwMajorVersion >= 6)
            g_pfnIcmpCallback = icmpwin_callback_apc;
        else
            g_pfnIcmpCallback = (PIO_APC_ROUTINE)icmpwin_callback_old;
    }

    TAILQ_INIT(&slirp->pongs_expected);
    TAILQ_INIT(&slirp->pongs_received);

    slirp->icmp_handle = IcmpCreateFile();
    slirp->icmp_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    return 0;
}

void icmpwin_finit(Slirp* slirp) {
    IcmpCloseHandle(slirp->icmp_handle);
    CloseHandle(slirp->icmp_event);
 
    while (!TAILQ_EMPTY(&slirp->pongs_received)) {
        struct pong* pong = TAILQ_FIRST(&slirp->pongs_received);
        TAILQ_REMOVE(&slirp->pongs_received, pong, queue_entry);
        free(pong);
    }

    /* this should be empty */
    while (!TAILQ_EMPTY(&slirp->pongs_expected)) {
        struct pong* pong = TAILQ_FIRST(&slirp->pongs_expected);
        TAILQ_REMOVE(&slirp->pongs_expected, pong, queue_entry);
        pong->slirp = NULL;
    }
}

/*
 * Outgoing ping from guest.
 */
void icmpwin_ping(Slirp* slirp, struct mbuf* m, int hlen) {
    struct ip* ip = mtod(m, struct ip*);
    size_t reqsize, pongsize;
    uint8_t ttl;
    size_t bufsize;
    struct pong* pong;
    IPAddr dst;
    IP_OPTION_INFORMATION opts;
    void* reqdata;
    int status;

    dst = ip->ip_dst.s_addr;
    ttl = ip->ip_ttl;
    assert(ttl > 0);

    reqsize = ip->ip_len - hlen - sizeof(struct icmp_echo);

    bufsize = sizeof(ICMP_ECHO_REPLY);
    if (reqsize < sizeof(IO_STATUS_BLOCK) + sizeof(struct icmp_echo))
        bufsize += sizeof(IO_STATUS_BLOCK) + sizeof(struct icmp_echo);
    else
        bufsize += reqsize;
    bufsize += 16; /* whatever that is; empirically at least XP needs it */

    pongsize = offsetof(struct pong, buf) + bufsize;
    if (slirp->cbIcmpPending + pongsize > 1024 * 1024)
        return;

    pong = (struct pong*)malloc(pongsize);
    if (pong == NULL)
        return;

    pong->slirp = slirp;
    pong->bufsize = bufsize;
    m_copydata(m, 0, hlen, (char*)&pong->reqiph);
    m_copydata(m, hlen, sizeof(struct icmp_echo), (char*)&pong->reqicmph);
    assert(pong->reqicmph.icmp_type == ICMP_ECHO);

    if (m->m_next == NULL) {
        /* already in single contiguous buffer */
        reqdata = mtod(m, char*) + sizeof(struct ip) + sizeof(struct icmp_echo);
    } else {
        /* use reply buffer as temporary storage */
        reqdata = pong->buf;
        m_copydata(m, sizeof(struct ip) + sizeof(struct icmp_echo),
                   (int)reqsize, reqdata);
    }

    dst = ip->ip_dst.s_addr;

    opts.Ttl = ttl;
    opts.Tos = ip->ip_tos; /* affected by DisableUserTOSSetting key */
    opts.Flags = (ip->ip_off & IP_DF) != 0 ? IP_FLAG_DF : 0;
    opts.OptionsSize = 0;
    opts.OptionsData = 0;

    status = IcmpSendEcho2(slirp->icmp_handle, NULL, g_pfnIcmpCallback, pong,
                           dst, reqdata, (WORD)reqsize, &opts, pong->buf,
                           (DWORD)pong->bufsize, 5 * 1000 /* ms */);
    if (status != 0) {
        error_report("Slirp: IcmpSendEcho2: unexpected status %d\n", status);
    } else if ((status = GetLastError()) != ERROR_IO_PENDING) {
        int code;

        error_report("Slirp: IcmpSendEcho2: error %d\n", status);
        switch (status) {
            case ERROR_NETWORK_UNREACHABLE:
                code = ICMP_UNREACH_NET;
                break;
            case ERROR_HOST_UNREACHABLE:
                code = ICMP_UNREACH_HOST;
                break;
            default:
                code = -1;
                break;
        }

        if (code != -1) /* send icmp error */
        {
            struct mbuf* em = icmpwin_get_error(pong, ICMP_UNREACH, code);
            if (em != NULL) {
                struct ip* eip = mtod(em, struct ip*);
                // eip->ip_src = alias_addr;
                ip_output((struct socket*)NULL, em);
            }
        }
    } else /* success */
    {
        slirp->cbIcmpPending += pongsize;
        TAILQ_INSERT_TAIL(&slirp->pongs_expected, pong, queue_entry);
        pong = NULL; /* callback owns it now */
    }

    if (pong != NULL)
        free(pong);
}

static VOID WINAPI icmpwin_callback_apc(void* ctx,
                                        PIO_STATUS_BLOCK iob,
                                        ULONG reserved) {
    struct pong* pong = (struct pong*)ctx;
    if (pong != NULL)
        icmpwin_callback(pong);
    (void)iob;
    (void)reserved;
}

static VOID WINAPI icmpwin_callback_old(void* ctx) {
    struct pong* pong = (struct pong*)ctx;
    if (pong != NULL)
        icmpwin_callback(pong);
}

/*
 * Actual callback code for IcmpSendEcho2().  OS version specific
 * trampoline will free "pong" argument for us.
 *
 * Since async callback can be called anytime the thread is alertable,
 * it's not safe to do any processing here.  Instead queue it and
 * notify the main loop.
 */
static void icmpwin_callback(struct pong* pong) {
    Slirp* slirp = pong->slirp;

    if (slirp == NULL) {
        free(pong);
        return;
    }

#ifdef DEBUG
    {
        struct pong *expected, *already;

        TAILQ_FOREACH(expected, &slirp->pongs_expected, queue_entry) {
            if (expected == pong)
                break;
        }
        assert(expected);

        TAILQ_FOREACH(already, &slirp->pongs_received, queue_entry) {
            if (already == pong)
                break;
        }
        assert(!already);
    }
#endif

    TAILQ_REMOVE(&slirp->pongs_expected, pong, queue_entry);
    TAILQ_INSERT_TAIL(&slirp->pongs_received, pong, queue_entry);

    WSASetEvent(slirp->icmp_event);
}

void icmpwin_process(Slirp* slirp) {
    struct pong_tailq pongs;

    if (TAILQ_EMPTY(&slirp->pongs_received))
        return;

    TAILQ_INIT(&pongs);
    TAILQ_CONCAT(&pongs, &slirp->pongs_received, queue_entry);

    while (!TAILQ_EMPTY(&pongs)) {
        struct pong* pong = TAILQ_FIRST(&pongs);
        size_t sz;

        sz = offsetof(struct pong, buf) + pong->bufsize;
        assert(slirp->cbIcmpPending >= sz);
        slirp->cbIcmpPending -= sz;

        icmpwin_pong(pong);

        TAILQ_REMOVE(&pongs, pong, queue_entry);
        free(pong);
    }
}

void icmpwin_pong(struct pong* pong) {
    Slirp* slirp;
    DWORD nreplies;
    ICMP_ECHO_REPLY* reply;
    struct mbuf* m;
    struct ip* ip;
    struct icmp_echo* icmp;
    size_t reqsize;

    slirp = pong->slirp; /* to make slirp_state.h macro hackery work */

    nreplies = IcmpParseReplies(pong->buf, (DWORD)pong->bufsize);
    if (nreplies == 0) {
        DWORD error = GetLastError();
        if (error == IP_REQ_TIMED_OUT)
            error_report("Slirp: ping %p timed out\n", (void*)pong);
        else
            error_report("Slirp: ping %p: IcmpParseReplies: error %d\n",
                         (void*)pong, (int)error);
        return;
    }

    reply = (ICMP_ECHO_REPLY*)pong->buf;

    if (reply->Status == IP_SUCCESS) {
        if (reply->Options.OptionsSize != 0) /* don't do options */
            return;

        /* need to remap &reply->Address ? */
        if (/* not a mapped loopback */ 1) {
            if (reply->Options.Ttl <= 1)
                return;
            --reply->Options.Ttl;
        }

        reqsize = reply->DataSize;
        if ((reply->Options.Flags & IP_FLAG_DF) != 0 &&
            sizeof(struct ip) + sizeof(struct icmp_echo) + reqsize > IF_MTU) {
            return;
        }

        m = icmpwin_get_mbuf(slirp, reqsize);
        if (m == NULL) {
            return;
        }

        ip = mtod(m, struct ip*);
        icmp = (struct icmp_echo*)(mtod(m, char*) + sizeof(*ip));

        /* fill in ip (ip_output0() does the boilerplate for us) */
        ip->ip_tos = reply->Options.Tos;
        ip->ip_len = sizeof(*ip) + sizeof(*icmp) + (int)reqsize;
        ip->ip_off = 0;
        ip->ip_ttl = reply->Options.Ttl;
        ip->ip_p = IPPROTO_ICMP;
        ip->ip_src.s_addr = reply->Address;
        ip->ip_dst = pong->reqiph.ip_src;

        icmp->icmp_type = ICMP_ECHOREPLY;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_echo_id = pong->reqicmph.icmp_echo_id;
        icmp->icmp_echo_seq = pong->reqicmph.icmp_echo_seq;

        m_append(slirp, m, (int)reqsize, reply->Data);

        m->m_data += sizeof(*ip);
        m->m_len -= sizeof(*ip);
        icmp->icmp_cksum = cksum(m, ip->ip_len - sizeof(*ip));
        m->m_data -= sizeof(*ip);
        m->m_len += sizeof(*ip);
    } else {
        uint8_t type, code;

        switch (reply->Status) {
            case IP_DEST_NET_UNREACHABLE:
                type = ICMP_UNREACH;
                code = ICMP_UNREACH_NET;
                break;
            case IP_DEST_HOST_UNREACHABLE:
                type = ICMP_UNREACH;
                code = ICMP_UNREACH_HOST;
                break;
            case IP_DEST_PROT_UNREACHABLE:
                type = ICMP_UNREACH;
                code = ICMP_UNREACH_PROTOCOL;
                break;
            case IP_PACKET_TOO_BIG:
                type = ICMP_UNREACH;
                code = ICMP_UNREACH_NEEDFRAG;
                break;
            case IP_SOURCE_QUENCH:
                type = ICMP_SOURCEQUENCH;
                code = 0;
                break;
            case IP_TTL_EXPIRED_TRANSIT:
                type = ICMP_TIMXCEED;
                code = ICMP_TIMXCEED_INTRANS;
                break;
            case IP_TTL_EXPIRED_REASSEM:
                type = ICMP_TIMXCEED;
                code = ICMP_TIMXCEED_REASS;
                break;
            default:
                error_report("Slirp: ping reply status %d, dropped\n",
                             (int)reply->Status);
                return;
        }

        /*
         * XXX: we don't know the TTL of the request at the time this
         * ICMP error was generated (we can guess it was 1 for ttl
         * exceeded, but don't bother faking it).
         */
        m = icmpwin_get_error(pong, type, code);
        if (m == NULL)
            return;

        ip = mtod(m, struct ip*);

        ip->ip_tos = reply->Options.Tos;
        ip->ip_ttl = reply->Options.Ttl; /* XXX: decrement */
        ip->ip_src.s_addr = reply->Address;
    }

    assert(ip->ip_len == m_length(m, NULL));
    ip_output((struct socket*)NULL, m);
}

/*
 * Prepare mbuf with ICMP error type/code.
 * IP source must be filled by the caller.
 */
static struct mbuf* icmpwin_get_error(struct pong* pong, int type, int code) {
    Slirp* slirp = pong->slirp;
    struct mbuf* m;
    struct ip* ip;
    struct icmp_echo* icmp;
    size_t reqsize;

    reqsize = sizeof(pong->reqiph) + sizeof(pong->reqicmph);

    m = icmpwin_get_mbuf(slirp, reqsize);
    if (m == NULL)
        return NULL;

    ip = mtod(m, struct ip*);
    icmp = (struct icmp_echo*)(mtod(m, char*) + sizeof(*ip));

    ip->ip_tos = 0;
    ip->ip_len = sizeof(*ip) + sizeof(*icmp) + (int)reqsize;
    ip->ip_off = 0;
    ip->ip_ttl = IPDEFTTL;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_src.s_addr = 0; /* NB */
    ip->ip_dst = pong->reqiph.ip_src;

    icmp->icmp_type = type;
    icmp->icmp_code = code;
    icmp->icmp_cksum = 0;
    icmp->icmp_echo_id = 0;
    icmp->icmp_echo_seq = 0;

    m_append(slirp, m, sizeof(pong->reqiph), (char*)&pong->reqiph);
    m_append(slirp, m, sizeof(pong->reqicmph), (char*)&pong->reqicmph);

    m->m_data += sizeof(*ip);
    m->m_len -= sizeof(*ip);
    icmp->icmp_cksum = cksum(m, ip->ip_len - sizeof(*ip));
    m->m_data -= sizeof(*ip);
    m->m_len += sizeof(*ip);

    return m;
}

static struct mbuf* icmpwin_get_mbuf(Slirp* slirp, size_t reqsize) {
    struct mbuf* m;
    int if_maxlinkhdr = 14;

    if (reqsize) {
    }

    reqsize += if_maxlinkhdr;
    reqsize += sizeof(struct ip) + sizeof(struct icmp_echo);

    m = m_get(slirp);

    if (m == NULL)
        return NULL;

    m_inc(m, reqsize);

    m->m_data += if_maxlinkhdr; /* reserve leading space for ethernet header */

    m->m_len = sizeof(struct ip) + sizeof(struct icmp_echo);

    return m;
}
