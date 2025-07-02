/*
 * STM32F429 SPI
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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

#ifndef HW_STM32F429_SPI_H
#define HW_STM32F429_SPI_H

#include "hw/sysbus.h"
#include "qom/object.h"
#include "hw/ssi/ssi.h"
#include "hw/gpio/perif_pinout.h"

#define STM_SPI_CR1     0x00
#define STM_SPI_CR2     0x04
#define STM_SPI_SR      0x08
#define STM_SPI_DR      0x0C
#define STM_SPI_CRCPR   0x10
#define STM_SPI_RXCRCR  0x14
#define STM_SPI_TXCRCR  0x18
#define STM_SPI_I2SCFGR 0x1C
#define STM_SPI_I2SPR   0x20

/*
DFF -> Data Frame Format: 0 = 8-bit, 1 = 16-bit; Set only when SPI is disabled (SPE = 0)
SPE -> SPI enable: 0 = Peripheral disabled, 1 = Peripheral enabled;
                   When disabling the SPI, follow the procedure described in Section 28.3.8.
MSTR -> Master selection: 0 = slave, 1 = master
CPOL -> Clock polarity
CPHA -> Clock phase
 */
#define STM_SPI_CR1_DFF  (1 << 11)
#define STM_SPI_CR1_SPE  (1 << 6)
#define STM_SPI_CR1_MSTR (1 << 2)
#define STM_SPI_CR1_CPOL (1 << 1)
#define STM_SPI_CR1_CPHA (1 << 0)

#define STM_SPI_SR_RXNE (1 << 0)
#define STM_SPI_SR_TXE  (1 << 1)

#define TYPE_STM32F429_SPI "stm32f429-spi"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F429SPIState, STM32F429_SPI)

typedef struct STM32F429SPIFifoEntry {
    uint64_t timeout;
    uint32_t value;
    /* TODO: config the following in .clang-format*/
    // clang-format off
    QTAILQ_ENTRY(STM32F429SPIFifoEntry) entries;
    // clang-format on
} STM32F429SPIFifoEntry;

struct STM32F429SPIState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t spi_cr1;
    uint32_t spi_cr2;
    uint32_t spi_sr;
    uint32_t spi_dr_tx;     // The data register is split into 2 buffers, write to the data register writes into the Tx buffer
    uint32_t spi_dr_rx;     // and a read from the data register returns the value held in the Rx buffer
    uint32_t spi_crcpr;
    uint32_t spi_rxcrcr;
    uint32_t spi_txcrcr;
    uint32_t spi_i2scfgr;
    uint32_t spi_i2spr;

    char *name;
    qemu_irq irq;
    SSIBus *ssi;
    PerifPinoutDevice *ppd;
    // list<pair<uint64_t, uint32_t>>: list of timeout val to data val
    /* TODO: config the following in .clang-format*/
    // clang-format off
    QTAILQ_HEAD(, STM32F429SPIFifoEntry) ppd_fifo_head;
    // clang-format on
    QEMUTimer *timer;
};

#endif /* HW_STM32F429_SPI_H */
