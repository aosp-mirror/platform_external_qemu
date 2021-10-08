/*
 * QTests for Nuvoton Host PCI Mailbox Modules.
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
#include "libqos/pci.h"
#include "libqos/pci-pc.h"
#include "hw/pci/pci_regs.h"

#define PCI_MBOX_BAR        0

#define CODE_OK             0x00
#define CODE_INVALID_OP     0xa0
#define CODE_INVALID_SIZE   0xa1
#define CODE_ERROR          0xff

#define OP_READ             0x01
#define OP_WRITE            0x02
#define OP_INVALID          0x41

typedef struct {
    int sock_ipmi;
    int sock_pcimbx;
    int fd_ipmi;
    int fd_pcimbx;

    QPCIBar io_bar;
    QPCIBus *bus;
    QPCIDevice *dev;
    QTestState *qts;
} NuvotonTestState;

NuvotonTestState s;

/* Ceate a local TCP socket with any port, then save the port we got */
static in_port_t open_socket(int *sock)
{
    struct sockaddr_in myaddr;
    socklen_t addrlen;

    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    myaddr.sin_port = 0;
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    g_assert(*sock != -1);
    g_assert(bind(*sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) != -1);
    addrlen = sizeof(myaddr);
    g_assert(getsockname(*sock, (struct sockaddr *) &myaddr , &addrlen) != -1);
    g_assert(listen(*sock, 1) != -1);
    return ntohs(myaddr.sin_port);
}

static void setup_fd(int *fd, int sock)
{
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    g_assert(select(sock + 1, &readfds, NULL, NULL, NULL) == 1);

    *fd = accept(sock, NULL, 0);
    g_assert(fd >= 0);
}

static uint8_t read_response(uint8_t *buf, size_t len)
{
    uint8_t code;
    ssize_t ret = read(s.fd_pcimbx, &code, 1);

    if (ret == -1) {
        return CODE_ERROR;
    }
    if (code != CODE_OK) {
        return code;
    }
    g_test_message("response code: %x", code);
    if (len > 0) {
        ret = read(s.fd_pcimbx, buf, len);
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
        rv = write(s.fd_pcimbx, &op, 1);
        g_assert_cmpint(rv, ==, 1);
        /* Write offset */
        rv = write(s.fd_pcimbx, (uint8_t *)&offset, sizeof(uint64_t));
        g_assert_cmpint(rv, ==, sizeof(uint64_t));
        /* Write size */
        g_assert_cmpint(write(s.fd_pcimbx, &size, 1), ==, 1);

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
        rv = write(s.fd_pcimbx, &op, 1);
        g_assert_cmpint(rv, ==, 1);
        /* Write offset */
        rv = write(s.fd_pcimbx, (uint8_t *)&offset, sizeof(uint64_t));
        g_assert_cmpint(rv, ==, sizeof(uint64_t));
        /* Write size */
        g_assert_cmpint(write(s.fd_pcimbx, &size, 1), ==, 1);
        /* Write data */
        g_assert_cmpint(write(s.fd_pcimbx, buf, size), ==, size);

        /* Expect response */
        code = read_response(NULL, 0);
        g_assert_cmphex(code, ==, CODE_OK);

        buf += size;
        offset += size;
        len -= size;
    }
}

static void pcimbx_bar_read(char *data, uint64_t addr, size_t len)
{
    uint8_t read_size;

    while (len > 0) {
        if (len >= 8) {
            *(uint64_t *)data = qpci_io_readq(s.dev, s.io_bar, addr);
            read_size = 8;
        } else if (len >= 4) {
            *(uint32_t *)data = qpci_io_readl(s.dev, s.io_bar, addr);
            read_size = 4;
        } else if (len >= 2) {
            *(uint16_t *)data = qpci_io_readw(s.dev, s.io_bar, addr);
            read_size = 2;
        } else {
            *data = qpci_io_readb(s.dev, s.io_bar, addr);
            read_size = 1;
        }

        data += read_size;
        addr += read_size;
        len -= read_size;
    }
}

static void pcimbx_bar_write(const char *data, uint64_t addr, size_t len)
{
    uint8_t write_size;

    while (len > 0) {
        if (len >= 8) {
            qpci_io_writeq(s.dev, s.io_bar, addr, *(const uint64_t *)data);
            write_size = 8;
        } else if (len >= 4) {
            qpci_io_writel(s.dev, s.io_bar, addr, *(const uint32_t *)data);
            write_size = 4;
        } else if (len >= 2) {
            qpci_io_writew(s.dev, s.io_bar, addr, *(const uint16_t *)data);
            write_size = 2;
        } else {
            qpci_io_writeb(s.dev, s.io_bar, addr, *data);
            write_size = 1;
        }

        data += write_size;
        addr += write_size;
        len -= write_size;
    }
}

/*
 * When we write data to the BAR RAM region, the device writes it to the chardev
 * expecting the BMC device to read it and respond.
 * Since we can't hook it up to the BMC and then validate that way, we'll
 * read back the write request the host device sends, along with data, and
 * make sure that everything is formatted correctly.
 */
static void pcimbx_read_bmc_write(const char *data, size_t len,
                                    uint64_t offset)
{
    char buf[100];
    uint64_t cmd_offset;
    uint8_t cmd_size;
    uint8_t op;
    uint8_t ack = 0x00;
    ssize_t ret;
    char *p_buf = buf;
    const char *p_data = data;

    while (len > 0) {
        /* Read op */
        ret = read(s.fd_pcimbx, &op, 1);
        g_assert_cmpint(ret, ==, 1);
        g_assert_cmpint(op, ==, OP_WRITE);
        /* Read offset */
        ret = read(s.fd_pcimbx, (uint8_t *)&cmd_offset, sizeof(cmd_offset));
        g_assert_cmpint(ret, ==, sizeof(cmd_offset));
        g_assert_cmpint(cmd_offset, ==, offset);
        /* Read size */
        g_assert_cmpint(read(s.fd_pcimbx, &cmd_size, 1), ==, 1);

        /* Read data */
        g_test_message("receiving %u byte%s", cmd_size,
                       cmd_size > 1 ? "s" : "");
        ret = read(s.fd_pcimbx, p_buf, cmd_size);
        g_assert_cmpint(ret, ==, cmd_size);
        g_assert_cmpint(strncmp(p_data, p_buf, cmd_size), ==, 0);

        /* Send ack */
        g_assert_cmpint(write(s.fd_pcimbx, &ack, 1), ==, 1);

        len -= cmd_size;
        offset += cmd_size;
        p_buf += cmd_size;
        p_data += cmd_size;
    }
}

static void test_invalid_op(void)
{
    uint8_t op = OP_INVALID;
    uint8_t code;
    uint8_t buf[1];

    g_assert_cmpint(write(s.fd_pcimbx, &op, 1), ==, 1);
    code = read_response(buf, 1);
    g_assert_cmphex(code, ==, CODE_INVALID_OP);
}

/* Send data via chardev and read them in guest. */
static void test_guest_read(void)
{
    const char *data = "Hello World!";
    uint64_t offset = 0x1230;
    char buf[100];
    size_t len = strlen(data);

    send_data(offset, (uint8_t *)data, len);
    pcimbx_bar_read(buf, offset, len);
    g_assert_cmpint(strncmp(data, buf, len), ==, 0);
}

/* Write data in guest and read out via chardev. */
static void test_guest_write(void)
{
    const char *data = "Hello World!";
    char buf[100];
    uint64_t offset = 0x1230;
    size_t len = strlen(data);

    pcimbx_bar_write(data, offset, len);
    pcimbx_read_bmc_write(data, len, offset);
    receive_data(offset, (uint8_t *)buf, len);
    g_assert_cmpint(strncmp(data, buf, len), ==, 0);
}

static void pcimbx_init(void)
{
    s.bus = qpci_new_pc(s.qts, NULL);
    s.dev = qpci_device_find(s.bus, QPCI_DEVFN(3, 0));
    assert(s.dev != NULL);
    g_assert_cmpint(qpci_config_readw(s.dev, PCI_VENDOR_ID), ==, 0x1050);
    g_assert_cmpint(qpci_config_readw(s.dev, PCI_DEVICE_ID), ==, 0x0750);

    qpci_device_enable(s.dev);
    s.io_bar = qpci_iomap(s.dev, PCI_MBOX_BAR, NULL);
}

int main(int argc, char **argv)
{
    int ret;
    int pcimbx_port;
    int ipmichr_port;

    g_test_init(&argc, &argv, NULL);
    pcimbx_port = open_socket(&s.sock_pcimbx);
    ipmichr_port = open_socket(&s.sock_ipmi);
    g_test_message("port=%d", pcimbx_port);
    g_test_message("port=%d", ipmichr_port);
    s.qts = qtest_initf("-machine q35 "
        "-chardev socket,id=ipmichr0,host=localhost,port=%d,reconnect=10 "
        "-chardev socket,id=nuvoton-pcimbox-chr,host=localhost,"
        "port=%d,reconnect=10 "
        "-device ipmi-bmc-extern,chardev=ipmichr0,id=bmc0 "
        "-device nuvoton-ipmi-kcs,id=ipmi-kcs,bmc=bmc0 "
        "-device nuvoton-pci-ipmi,ipmi-kcs=ipmi-kcs,"
        "bmc-chardev=nuvoton-pcimbox-chr ",
        ipmichr_port, pcimbx_port);

    /* We only need to set it up for the PCI mailbox */
    setup_fd(&s.fd_pcimbx, s.sock_pcimbx);

    pcimbx_init();

    /* Writes an invalid op */
    qtest_add_func("/nuvoton_host_pci_mbox/invalid_op", test_invalid_op);
    /* Writes through chardev and reads from BAR */
    qtest_add_func("/nuvoton_host_pci_mbox/bar_read", test_guest_read);
    /*
     * Writes through bar, reads response (host forwarding data to BMC)
     * from chardev, then reads data from chardev.
     */
    qtest_add_func("/nuvoton_host_pci_mbox/bar_write", test_guest_write);
    ret = g_test_run();
    qtest_quit(s.qts);

    return ret;
}
