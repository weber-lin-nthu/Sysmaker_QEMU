/*
 * STM32F429 SPI
 *
 * Copyright (c) {YEAR} {NAME} {EMAIL}
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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/ssi/stm32f429_spi.h"
#include "migration/vmstate.h"
#include "hw/qdev-properties.h"
#include "sysemu/cpu-timers.h"

#include "qapi/qmp/qjson.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qlist.h"
#include "qapi/qmp/qstring.h"
#include "qapi/qmp/qnum.h"

#ifndef STM_SPI_ERR_DEBUG
#define STM_SPI_ERR_DEBUG 100
#endif

#define DB_PRINT_L(lvl, fmt, args...)               \
    do {                                            \
        if (STM_SPI_ERR_DEBUG >= lvl) {             \
            qemu_log("%s: " fmt, __func__, ##args); \
        }                                           \
    } while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ##args)

#define TYPE_SPI_INTERFACE_CONFIG "spi-interface-config"
OBJECT_DECLARE_SIMPLE_TYPE(SPIInterfaceConfig, SPI_INTERFACE_CONFIG);

#define TYPE_SPI_DATA "spi-data"
OBJECT_DECLARE_SIMPLE_TYPE(SPIData, SPI_DATA);

#define QDICT_TRY_GET(dict, key, obj_type) (QOBJECT(qobject_to(obj_type, qdict_get(dict, key))))

struct SPIInterfaceConfig {
    SCInterfaceConfig parent;
    QDict *(*parent_to_dict)(SCInterfaceConfig *obj);
    void (*parent_from_dict)(SCInterfaceConfig *obj, QDict *d);

    // in Hz
    int clock_freq;
    // [1..0] = [CPOL, CPHA]
    uint8_t spi_mode;
    enum BitOrder {
        MSB,
        LSB
    } bit_order;
    int data_frame_size;
};

static QDict *spi_ic_to_dict(SCInterfaceConfig *obj)
{
    SPIInterfaceConfig *ic = SPI_INTERFACE_CONFIG(obj);
    QDict *res             = ic->parent_to_dict(obj);
    qdict_put_int(res, "Clock Frequency", ic->clock_freq);
    qdict_put_int(res, "SPI Mode", ic->spi_mode);
    if (ic->bit_order == MSB) {
        qdict_put_str(res, "Bit Order", "MSB");
    } else {
        qdict_put_str(res, "Bit Order", "LSB");
    }
    qdict_put_int(res, "Data Frame Size", ic->data_frame_size);
    return res;
}
static void spi_ic_from_dict(SCInterfaceConfig *obj, QDict *d)
{
    SPIInterfaceConfig *ic = SPI_INTERFACE_CONFIG(obj);
    ic->parent_from_dict(obj, d);
    QObject *tmp;
    if (!(tmp = QDICT_TRY_GET(d, "Clock Frequency", QNum))) {
        qemu_log("QDict missing key or wrong type: %s\n", "Clock Frequency");
    } else {
        ic->clock_freq = qnum_get_int(qobject_to(QNum, tmp));
    }
    if (!(tmp = QDICT_TRY_GET(d, "SPI Mode", QNum))) {
        qemu_log("QDict missing key or wrong type: %s\n", "SPI Mode");
    } else {
        ic->spi_mode = qnum_get_int(qobject_to(QNum, tmp));
    }
    if (!(tmp = QDICT_TRY_GET(d, "Bit Order", QString))) {
        qemu_log("QDict missing key or wrong type: %s\n", "Bit Order");
    } else {
        const char *bit_order = qstring_get_str(qobject_to(QString, tmp));
        ic->bit_order         = strncmp(bit_order, "MSB", 3) == 0 ? MSB : LSB;
    }
    if (!(tmp = QDICT_TRY_GET(d, "Data Frame Size", QNum))) {
        qemu_log("QDict missing key or wrong type: %s\n", "Data Frame Size");
    } else {
        ic->data_frame_size = qnum_get_int(qobject_to(QNum, tmp));
    }
}

static void spi_interface_config_init(Object *obj)
{
    SCInterfaceConfig *sc_ic   = SC_INTERFACE_CONFIG(obj);
    SPIInterfaceConfig *spi_ic = SPI_INTERFACE_CONFIG(obj);
    spi_ic->parent_to_dict     = sc_ic->to_dict;
    spi_ic->parent_from_dict   = sc_ic->from_dict;
    sc_ic->to_dict             = spi_ic_to_dict;
    sc_ic->from_dict           = spi_ic_from_dict;
    spi_ic->clock_freq         = 0;
    spi_ic->spi_mode           = 0;
    spi_ic->bit_order          = 0;
    spi_ic->data_frame_size    = 0;
}
static void spi_interface_config_finalize(Object *obj) {}
static const TypeInfo spi_interface_config_info = {
    .name              = TYPE_SPI_INTERFACE_CONFIG,
    .parent            = TYPE_SC_INTERFACE_CONFIG,
    .instance_size     = sizeof(SPIInterfaceConfig),
    .instance_init     = spi_interface_config_init,
    .instance_finalize = spi_interface_config_finalize,
};
static void spi_interface_config_register_types(void)
{
    type_register_static(&spi_interface_config_info);
}
type_init(spi_interface_config_register_types);

struct SPIData {
    SCData parent;
    QDict *(*parent_to_dict)(SCData *obj);
    void (*parent_from_dict)(SCData *obj, QDict *d);

    // list of bytes, represented in 16-bit integer
    QList *cs_data;
    QList *mosi_data;
    QList *miso_data;
};

static QDict *spi_data_to_dict(SCData *obj)
{
    SPIData *data = SPI_DATA(obj);
    QDict *res    = data->parent_to_dict(obj);
    if (data->cs_data)
        qdict_put(res, "CS", qlist_copy(data->cs_data));
    if (data->mosi_data)
        qdict_put(res, "MOSI", qlist_copy(data->mosi_data));
    if (data->miso_data)
        qdict_put(res, "MISO", qlist_copy(data->miso_data));
    return res;
}
static void spi_data_from_dict(SCData *obj, QDict *d)
{
    SPIData *data = SPI_DATA(obj);
    data->parent_from_dict(obj, d);
    QObject *tmp;
    if (!(tmp = QDICT_TRY_GET(d, "CS", QList))) {
        qemu_log("QDict missing key or wrong type: %s\n", "CS");
    } else {
        data->cs_data = qlist_copy(qobject_to(QList, tmp));
    }
    if (!(tmp = QDICT_TRY_GET(d, "MOSI", QList))) {
        qemu_log("QDict missing key or wrong type: %s\n", "MOSI");
    } else {
        data->mosi_data = qlist_copy(qobject_to(QList, tmp));
    }
    if (!(tmp = QDICT_TRY_GET(d, "MISO", QList))) {
        qemu_log("QDict missing key or wrong type: %s\n", "MISO");
    } else {
        data->miso_data = qlist_copy(qobject_to(QList, tmp));
    }
}

static void spi_data_init(Object *obj)
{
    SCData *scd         = SC_DATA(obj);
    SPIData *d          = SPI_DATA(obj);
    d->parent_to_dict   = scd->to_dict;
    d->parent_from_dict = scd->from_dict;
    scd->to_dict        = spi_data_to_dict;
    scd->from_dict      = spi_data_from_dict;
    d->cs_data          = NULL;
    d->mosi_data        = NULL;
    d->miso_data        = NULL;
}
static void spi_data_finalize(Object *obj)
{
    SPIData *d = SPI_DATA(obj);
    qlist_unref(d->cs_data);
    qlist_unref(d->mosi_data);
    qlist_unref(d->miso_data);
}
static const TypeInfo spi_data_info = {
    .name              = TYPE_SPI_DATA,
    .parent            = TYPE_SC_DATA,
    .instance_size     = sizeof(SPIData),
    .instance_init     = spi_data_init,
    .instance_finalize = spi_data_finalize,
};
static void spi_data_register_types(void)
{
    type_register_static(&spi_data_info);
}
type_init(spi_data_register_types);

static void stm32f429_spi_reset(DeviceState *dev)
{
    STM32F429SPIState *s = STM32F429_SPI(dev);

    s->spi_cr1     = 0x00000000;
    s->spi_cr2     = 0x00000000;
    s->spi_sr      = 0x00000002;
    s->spi_dr_tx   = 0x00000000;
    s->spi_dr_rx   = 0x00000000;
    s->spi_crcpr   = 0x00000007;
    s->spi_rxcrcr  = 0x00000000;
    s->spi_txcrcr  = 0x00000000;
    s->spi_i2scfgr = 0x00000000;
    s->spi_i2spr   = 0x00000002;
}

static void stm32f429_spi_transfer(STM32F429SPIState *s)
{
    DB_PRINT("SPI%s Data to send: 0x%x, ASCII: %c\n", s->name, s->spi_dr_tx, (char)s->spi_dr_tx);

    int CPOL = s->spi_cr1 & STM_SPI_CR1_CPOL;
    int CPHA = s->spi_cr1 & STM_SPI_CR1_CPHA;

    g_autoptr(SCDataPack) data_pack = sc_datapack_new("SPI", TYPE_SPI_INTERFACE_CONFIG, TYPE_SPI_DATA);
    SPIInterfaceConfig *ic          = SPI_INTERFACE_CONFIG(data_pack->interface_config);
    SPIData *data                   = SPI_DATA(data_pack->data);

    ic->clock_freq      = 4000000; // dummy value
    ic->spi_mode        = (CPOL << 1) | CPHA;
    ic->bit_order       = (s->spi_cr1 & (1u << 7)) ? LSB : MSB;
    ic->data_frame_size = (s->spi_cr1 & (1u << 11)) ? 16 : 8;

    if (!data->mosi_data)
        data->mosi_data = qlist_new();
    qlist_append_int(data->mosi_data, (uint16_t)s->spi_dr_tx);

    g_autofree char *perif_name = g_strdup_printf("SPI%s", s->name);
    PerifPinoutDeviceClass *k   = PERIF_PINOUT_DEVICE_GET_CLASS(s->ppd);
    k->transport(k, perif_name, data_pack, data_pack);

    // Deal with responding data pack

    // DB_PRINT("end time: %s\n", data_pack->end_time->str);
    STM32F429SPIFifoEntry *e = g_new(STM32F429SPIFifoEntry, 1);
    e->timeout               = atoll(data_pack->end_time->str);
    if (data->miso_data && !qlist_empty(data->miso_data)) {
        e->value = qnum_get_int(qobject_to(QNum, qlist_entry_obj(qlist_first(data->miso_data))));
    }
    timer_mod_anticipate_ns(s->timer, e->timeout);
    DB_PRINT("curr time: %ld ns, new timeout at: %ld ns, with data %d\n", icount_get(), e->timeout, e->value);
    QTAILQ_INSERT_TAIL(&s->ppd_fifo_head, e, entries);

    // s->spi_dr_rx = ssi_transfer(s->ssi, s->spi_dr_tx);

    // DB_PRINT("SPI%s Data received: 0x%x, ASCII: %c\n", s->name, s->spi_dr, (char)s->spi_dr);
}

static void transfer_callback(void *opaque)
{
    STM32F429SPIState *s = opaque;

    STM32F429SPIFifoEntry *e = QTAILQ_FIRST(&s->ppd_fifo_head);
    QTAILQ_REMOVE(&s->ppd_fifo_head, e, entries);
    DB_PRINT("callback called at: %ld ns, processing fifo data %d, ASCII: %c\n", icount_get(), e->value, (char)e->value);
    s->spi_dr_rx = e->value;
    s->spi_sr |= STM_SPI_SR_RXNE;
    s->spi_sr |= STM_SPI_SR_TXE;
    DB_PRINT("SPI_DR and SPI_SR_RXNE updated\n");
    g_free(e);
    /* TODO: fetch next event in queue*/
}

static uint64_t stm32f429_spi_read(void *opaque, hwaddr addr, unsigned int size)
{
    STM32F429SPIState *s = opaque;

    DB_PRINT("[%ld ns]: Address: 0x%" HWADDR_PRIx "\n", icount_get(), addr);

    switch (addr) {
    case STM_SPI_CR1:
        return s->spi_cr1;
    case STM_SPI_CR2:
        qemu_log_mask(LOG_UNIMP, "%s: Interrupts and DMA are not implemented\n",
                      __func__);
        return s->spi_cr2;
    case STM_SPI_SR:
        return s->spi_sr;
    case STM_SPI_DR:
        // stm32f429_spi_transfer(s);
        DB_PRINT("[%ld ns]: SPI%s read Data: 0x%x, ASCII: %c\n", icount_get(), s->name, s->spi_dr_rx, (char)s->spi_dr_rx);
        s->spi_sr &= ~STM_SPI_SR_RXNE;
        return s->spi_dr_rx;
    case STM_SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: CRC is not implemented, the registers "
                      "are included for compatibility\n",
                      __func__);
        return s->spi_crcpr;
    case STM_SPI_RXCRCR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: CRC is not implemented, the registers "
                      "are included for compatibility\n",
                      __func__);
        return s->spi_rxcrcr;
    case STM_SPI_TXCRCR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: CRC is not implemented, the registers "
                      "are included for compatibility\n",
                      __func__);
        return s->spi_txcrcr;
    case STM_SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: I2S is not implemented, the registers "
                      "are included for compatibility\n",
                      __func__);
        return s->spi_i2scfgr;
    case STM_SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: I2S is not implemented, the registers "
                      "are included for compatibility\n",
                      __func__);
        return s->spi_i2spr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }

    return 0;
}

static void stm32f429_spi_write(void *opaque, hwaddr addr, uint64_t val64,
                                unsigned int size)
{
    STM32F429SPIState *s = opaque;
    uint32_t value       = val64;

    DB_PRINT("[%ld ns]: Address: 0x%" HWADDR_PRIx ", Value: 0x%x\n", icount_get(), addr, value);

    switch (addr) {
    case STM_SPI_CR1:
        qemu_log("[SPI control register 1]:\n");
        // qemu_log("\tBidirectional data mode enable: %s\n", (value & (1u << 15)) ? "1-line bidirectional data mode selected" : "2-line unidirectional data mode selected");
        qemu_log("\tData frame format: %s\n", (value & (1u << 11)) ? "16-bit data frame format is selected for transmission/reception" : " 8-bit data frame format is selected for transmission/reception");
        qemu_log("\tFrame format: %s\n", (value & (1u << 7)) ? "LSB transmitted first" : "MSB transmitted first");
        qemu_log("\tSPI enable: %s\n", (value & (1u << 6)) ? "Peripheral enabled" : "Peripheral disabled");
        qemu_log("\tBaud rate control: %s/%d\n", "f_PCLK", (2u << ((value >> 3) & (0b111))));
        qemu_log("\tClock polarity: %s\n", (value & (1u << 1)) ? "CK to 1 when idle" : "CK to 0 when idle");
        qemu_log("\tClock phase: %s\n", (value & (1u << 0)) ? "1: The second clock transition is the first data capture edge" : "0: The first clock transition is the first data capture edge");
        s->spi_cr1 = value;
        return;
    case STM_SPI_CR2:
        qemu_log_mask(LOG_UNIMP,
                      "%s: "
                      "Interrupts and DMA are not implemented\n",
                      __func__);
        s->spi_cr2 = value;
        return;
    case STM_SPI_SR:
        /* Read only register, except for clearing the CRCERR bit, which
         * is not supported
         */
        return;
    case STM_SPI_DR:
        s->spi_dr_tx = value;
        s->spi_sr &= ~STM_SPI_SR_TXE;
        stm32f429_spi_transfer(s);
        return;
    case STM_SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP, "%s: CRC is not implemented\n", __func__);
        return;
    case STM_SPI_RXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Read only register: "
                      "0x%" HWADDR_PRIx "\n",
                      __func__, addr);
        return;
    case STM_SPI_TXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Read only register: "
                      "0x%" HWADDR_PRIx "\n",
                      __func__, addr);
        return;
    case STM_SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: "
                      "I2S is not implemented\n",
                      __func__);
        return;
    case STM_SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: "
                      "I2S is not implemented\n",
                      __func__);
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f429_spi_ops = {
    .read       = stm32f429_spi_read,
    .write      = stm32f429_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_stm32f429_spi = {
    .name               = TYPE_STM32F429_SPI,
    .version_id         = 1,
    .minimum_version_id = 1,
    .fields =
        (const VMStateField[]){
            VMSTATE_UINT32(spi_cr1, STM32F429SPIState),
            VMSTATE_UINT32(spi_cr2, STM32F429SPIState),
            VMSTATE_UINT32(spi_sr, STM32F429SPIState),
            VMSTATE_UINT32(spi_dr_tx, STM32F429SPIState),
            VMSTATE_UINT32(spi_dr_rx, STM32F429SPIState),
            VMSTATE_UINT32(spi_crcpr, STM32F429SPIState),
            VMSTATE_UINT32(spi_rxcrcr, STM32F429SPIState),
            VMSTATE_UINT32(spi_txcrcr, STM32F429SPIState),
            VMSTATE_UINT32(spi_i2scfgr, STM32F429SPIState),
            VMSTATE_UINT32(spi_i2spr, STM32F429SPIState),
            VMSTATE_END_OF_LIST(),
        },
};

static void stm32f429_spi_init(Object *obj)
{
    STM32F429SPIState *s = STM32F429_SPI(obj);
    DeviceState *dev     = DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f429_spi_ops, s,
                          TYPE_STM32F429_SPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    s->ssi   = ssi_create_bus(dev, "ssi");
    s->ppd   = PERIF_PINOUT_DEVICE(qdev_new(TYPE_PERIF_PINOUT_DEVICE));
    s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, transfer_callback, s);
    QTAILQ_INIT(&s->ppd_fifo_head);
}

static Property stm32f429_spi_properties[] = {
    DEFINE_PROP_STRING("name", STM32F429SPIState, name),
    DEFINE_PROP_END_OF_LIST(),
};
static void stm32f429_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, stm32f429_spi_properties);

    dc->reset = stm32f429_spi_reset;
    dc->vmsd  = &vmstate_stm32f429_spi;
}

static const TypeInfo stm32f429_spi_info = {
    .name          = TYPE_STM32F429_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F429SPIState),
    .instance_init = stm32f429_spi_init,
    .class_init    = stm32f429_spi_class_init,
};

static void stm32f429_spi_register_types(void)
{
    type_register_static(&stm32f429_spi_info);
}

type_init(stm32f429_spi_register_types)

#undef QDICT_TRY_GET