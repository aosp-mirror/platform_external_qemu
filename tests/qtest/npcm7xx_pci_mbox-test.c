/*
 * QTests for Nuvoton NPCM7xx PCI Mailbox Modules.
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "qemu/bitops.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "libqtest-single.h"

#define PCI_MBOX_BA         0xf0848000
#define PCI_MBOX_IRQ        8

/* register offset */
#define PCI_MBOX_STAT       0x00
#define PCI_MBOX_CTL        0x04
#define PCI_MBOX_CMD        0x08

#define CODE_OK             0x00
#define CODE_INVALID_OP     0xa0
#define CODE_INVALID_SIZE   0xa1
#define CODE_ERROR          0xff

#define OP_READ             0x01
#define OP_WRITE            0x02
#define OP_INVALID          0x41


static int sock;
static int fd;

/*
 * Create a local TCP socket with any port, then save off the port we got.
 */
static in_port_t open_socket(void)
{
    struct sockaddr_in myaddr;
    socklen_t addrlen;

    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    myaddr.sin_port = 0;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    g_assert(sock != -1);
    g_assert(bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) != -1);
    addrlen = sizeof(myaddr);
    g_assert(getsockname(sock, (struct sockaddr *) &myaddr , &addrlen) != -1);
    g_assert(listen(sock, 1) != -1);
    return ntohs(myaddr.sin_port);
}

static void setup_fd(void)
{
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    g_assert(select(sock + 1, &readfds, NULL, NULL, NULL) == 1);

    fd = accept(sock, NULL, 0);
    g_assert(fd >= 0);
}

static uint8_t read_response(uint8_t *buf, size_t len)
{
    uint8_t code;
    ssize_t ret = read(fd, &code, 1);

    if (ret == -1) {
        return CODE_ERROR;
    }
    if (code != CODE_OK) {
        return code;
    }
    g_test_message("response code: %x", code);
    if (len > 0) {
        ret = read(fd, buf, len);
        if (ret < len) {
            return CODE_ERROR;
        }
    }
    return CODE_OK;
}

static void receive_data(uint64_t offset, uint8_t *buf, size_t len)
{
    uint8_t op = OP_READ;
    uint8_t code;
    ssize_t rv;

    while (len > 0) {
        uint8_t size;

        if (len >= 8) {
            size = 8;
        } else if (len >= 4) {
            size = 4;
        } else if (len >= 2) {
            size = 2;
        } else {
            size = 1;
        }

        g_test_message("receiving %u bytes", size);
        /* Write op */
        rv = write(fd, &op, 1);
        g_assert_cmpint(rv, ==, 1);
        /* Write offset */
        rv = write(fd, (uint8_t *)&offset, sizeof(uint64_t));
        g_assert_cmpint(rv, ==, sizeof(uint64_t));
        /* Write size */
        g_assert_cmpint(write(fd, &size, 1), ==, 1);

        /* Read data and Expect response */
        code = read_response(buf, size);
        g_assert_cmphex(code, ==, CODE_OK);

        buf += size;
        offset += size;
        len -= size;
    }
}

static void send_data(uint64_t offset, const uint8_t *buf, size_t len)
{
    uint8_t op = OP_WRITE;
    uint8_t code;
    ssize_t rv;

    while (len > 0) {
        uint8_t size;

        if (len >= 8) {
            size = 8;
        } else if (len >= 4) {
            size = 4;
        } else if (len >= 2) {
            size = 2;
        } else {
            size = 1;
        }

        g_test_message("sending %u bytes", size);
        /* Write op */
        rv = write(fd, &op, 1);
        g_assert_cmpint(rv, ==, 1);
        /* Write offset */
        rv = write(fd, (uint8_t *)&offset, sizeof(uint64_t));
        g_assert_cmpint(rv, ==, sizeof(uint64_t));
        /* Write size */
        g_assert_cmpint(write(fd, &size, 1), ==, 1);
        /* Write data */
        g_assert_cmpint(write(fd, buf, size), ==, size);

        /* Expect response */
        code = read_response(NULL, 0);
        g_assert_cmphex(code, ==, CODE_OK);

        buf += size;
        offset += size;
        len -= size;
    }
}

static void test_invalid_op(void)
{
    uint8_t op = OP_INVALID;
    uint8_t code;
    uint8_t buf[1];

    g_assert_cmpint(write(fd, &op, 1), ==, 1);
    code = read_response(buf, 1);
    g_assert_cmphex(code, ==, CODE_INVALID_OP);
}

/* Send data via chardev and read them in guest. */
static void test_guest_read(void)
{
    const char *data = "Hello World!";
    uint64_t offset = 0xa0;
    char buf[100];
    size_t len = strlen(data);

    send_data(offset, (uint8_t *)data, len);
    memread(PCI_MBOX_BA + offset, buf, len);
    g_assert_cmpint(strncmp(data, buf, len), ==, 0);
}

/* Write data in guest and read out via chardev. */
static void test_guest_write(void)
{
    const char *data = "Hello World!";
    uint64_t offset = 0xa0;
    char buf[100];
    size_t len = strlen(data);

    memwrite(PCI_MBOX_BA + offset, data, len);
    receive_data(offset, (uint8_t *)buf, len);
    g_assert_cmpint(strncmp(data, buf, len), ==, 0);
}

int main(int argc, char **argv)
{
    int ret;
    int port;

    g_test_init(&argc, &argv, NULL);
    port = open_socket();
    g_test_message("port=%d", port);
    global_qtest = qtest_initf("-machine npcm750-evb "
        "-chardev socket,id=npcm7xx-pcimbox-chr,host=localhost,"
        "port=%d,reconnect=10 "
        "-global driver=npcm7xx-pci-mbox,property=chardev,"
        "value=npcm7xx-pcimbox-chr",
        port);
    setup_fd();
    qtest_irq_intercept_in(global_qtest, "/machine/soc/a9mpcore/gic");

    qtest_add_func("/npcm7xx_pci_mbox/invalid_op", test_invalid_op);
    qtest_add_func("/npcm7xx_pci_mbox/read", test_guest_read);
    qtest_add_func("/npcm7xx_pci_mbox/write", test_guest_write);
    ret = g_test_run();
    qtest_quit(global_qtest);

    return ret;
}
