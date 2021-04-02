/*
 * QTests for Nuvoton NPCM7xx LPC Modules.
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
#include "libqtest.h"
#include "qapi/qmp/qdict.h"

#define BASE_ADDR       0xf0007000
#define KCS_HIB_IRQ     9

/* BPC registers */
#define BPCFA2L         0x42
#define BPCFA2M         0x44
#define BPCFEN          0x46
#define BPCFSTAT        0x48
#define BPCFDATA        0x4a
#define BPCFMSTAT       0x4c
#define BPCFA1L         0x50
#define BPCFA1M         0x52
#define BPCSDR          0x54

#define BPCFEN_ADDR1EN  BIT(7)
#define BPCFEN_ADDR2EN  BIT(6)
#define BPCFEN_HRIE     BIT(4)
#define BPCFEN_FRIE     BIT(3)
#define BPCFEN_DWCAP    BIT(2)

#define BPCFSTAT_FIFO_V     BIT(7)
#define BPCFSTAT_FIFO_OF    BIT(5)
#define BPCFSTAT_AD         BIT(0)

#define BPC_DEFAULT_ADDR 0xadd2
#define BPC_BAD_ADDR 0xdead

static const uint8_t bpc_byte_data[] = {0xaa, 0xbd, 0xcf, 0x0, 0x44};
static const uint32_t bpc_dword_data[] = {0xabcdef12, 0x0, 0x765a, 0x18c3eb};

static void bpc_qom_set(QTestState *qts, const char *name, uint32_t value)
{
    QDict *response;
    const char *path = "/machine/soc/kcs/bpc";

    g_test_message("Setting properties %s of %s with value %u",
            name, path, value);
    response = qtest_qmp(qts, "{ 'execute': 'qom-set',"
            " 'arguments': { 'path': %s, 'property': %s, 'value': %u}}",
            path, name, value);
    /* The qom set message returns successfully. */
    g_assert_true(qdict_haskey(response, "return"));
}

static inline uint8_t lpc_read(QTestState *qts, uint8_t reg)
{
    return qtest_readb(qts, BASE_ADDR + reg);
}

static inline void lpc_write(QTestState *qts, uint8_t reg, uint8_t value)
{
    qtest_writeb(qts, BASE_ADDR + reg, value);
}

static void bpc_set_addr(QTestState *qts, uint16_t base, uint16_t value)
{
    /*
     * The addr has 2 bytes, Least significant byte (LSB) and Most significant
     * byte (MSB). They are separate by 2 in the register space.
     */
    lpc_write(qts, base, value & 0xff);
    lpc_write(qts, base + 2, value >> 8);
}

static bool bpc_valid(QTestState *qts)
{
    return lpc_read(qts, BPCFSTAT) & BPCFSTAT_FIFO_V;
}

static bool bpc_full(QTestState *qts)
{
    return lpc_read(qts, BPCFSTAT) & BPCFSTAT_FIFO_OF;
}

static uint8_t bpc_decoded_chan(QTestState *qts)
{
    return lpc_read(qts, BPCFSTAT) & BPCFSTAT_AD;
}

static uint8_t bpc_read_data(QTestState *qts)
{
    return lpc_read(qts, BPCFDATA);
}

static uint32_t bpc_read_dword(QTestState *qts)
{
    int count = 0;
    uint32_t result = 0;

    while (bpc_decoded_chan(qts) == 0) {
        result += bpc_read_data(qts) << (8 * count);
        ++count;
    }
    /* Read in last byte. */
    g_assert_cmpint(count, <, sizeof(uint32_t));
    result += bpc_read_data(qts) << (8 * count);

    return result;
}

static void test_bpc_byte(const void *data)
{
    QTestState *qts = qtest_init("-machine npcm750-evb");
    uint64_t chan = (uint64_t)data;
    size_t byte_data_size = ARRAY_SIZE(bpc_byte_data);

    g_assert_true(chan == 0 || chan == 1);
    qtest_irq_intercept_in(qts, "/machine/soc/a9mpcore/gic");
    /* Set init state - addr match and interrupt. */
    bpc_set_addr(qts, (chan == 0) ? BPCFA1L : BPCFA2L, BPC_DEFAULT_ADDR);
    lpc_write(qts, BPCFEN,
              BPCFEN_FRIE | ((chan == 0) ? BPCFEN_ADDR1EN : BPCFEN_ADDR2EN));

    /* Check empty state. */
    g_assert_false(bpc_valid(qts));
    g_assert_false(bpc_full(qts));

    /* Verify that writing to non matched addr won't affect the FIFO state. */
    bpc_qom_set(qts, "addr", BPC_BAD_ADDR);
    bpc_qom_set(qts, "data", 0xde);
    g_assert_false(bpc_valid(qts));
    g_assert_false(qtest_get_irq(qts, KCS_HIB_IRQ));

    /* Write a few data to matched addr. */
    for (size_t i = 0; i < byte_data_size; ++i) {
        bpc_qom_set(qts, "addr", BPC_DEFAULT_ADDR);
        bpc_qom_set(qts, "data", bpc_byte_data[i]);
        g_assert_false(bpc_full(qts));
    }

    /* Check all data can be read back. */
    for (size_t i = 0; i < byte_data_size; ++i) {
        g_assert_true(bpc_valid(qts));
        g_assert_true(qtest_get_irq(qts, KCS_HIB_IRQ));
        g_assert_cmphex(bpc_decoded_chan(qts), ==, chan);
        g_assert_cmphex(bpc_read_data(qts), ==, bpc_byte_data[i]);
    }

    /* Check FIFO is empty again. */
    g_assert_false(bpc_valid(qts));
    g_assert_false(qtest_get_irq(qts, KCS_HIB_IRQ));

    qtest_quit(qts);
}

static void test_bpc_dwcapture(void)
{
    QTestState *qts = qtest_init("-machine npcm750-evb");
    size_t dword_data_size = ARRAY_SIZE(bpc_dword_data);

    qtest_irq_intercept_in(qts, "/machine/soc/a9mpcore/gic");
    /* Set init state - addr match and interrupt. */
    bpc_set_addr(qts, BPCFA1L, BPC_DEFAULT_ADDR);
    lpc_write(qts, BPCFEN, BPCFEN_FRIE | BPCFEN_DWCAP);

    /* Check empty state. */
    g_assert_false(bpc_valid(qts));
    g_assert_false(bpc_full(qts));

    /* Verify that writing to non matched addr won't affect the FIFO state. */
    bpc_qom_set(qts, "addr", BPC_BAD_ADDR);
    bpc_qom_set(qts, "data", 0xdeadbeef);
    g_assert_false(bpc_valid(qts));
    g_assert_false(qtest_get_irq(qts, KCS_HIB_IRQ));

    /* Write a few data to matched addr. */
    for (size_t i = 0; i < dword_data_size; ++i) {
        bpc_qom_set(qts, "addr", BPC_DEFAULT_ADDR);
        bpc_qom_set(qts, "data", bpc_dword_data[i]);
        g_assert_false(bpc_full(qts));
    }

    /* Check all data can be read back. */
    for (size_t i = 0; i < dword_data_size; ++i) {
        g_assert_cmphex(bpc_read_dword(qts), ==, bpc_dword_data[i]);
    }

    /* Check FIFO is empty again. */
    g_assert_false(bpc_valid(qts));
    g_assert_false(qtest_get_irq(qts, KCS_HIB_IRQ));

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_data_func("bpc_addr1", (const void *)0, test_bpc_byte);
    qtest_add_data_func("bpc_addr2", (const void *)1, test_bpc_byte);
    qtest_add_func("bpc_dwcapture", test_bpc_dwcapture);

    return g_test_run();
}
