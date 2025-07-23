/*
 * STM32F429 GPIO (General Purpose Input/Ouput)
 *
 * Copyright (c) {YEAR} {NAME} {EMAIL}
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

/*
 * The reference used is the STMicroElectronics RM0351 Reference manual
 * for STM32F429 and STM32L4x6 advanced Arm ® -based 32-bit MCUs.
 * https://www.st.com/en/microcontrollers-microprocessors/stm32f429/documentation.html
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/gpio/stm32f429_gpio.h"
#include "hw/irq.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties.h"
#include "qapi/visitor.h"
#include "qapi/error.h"
#include "migration/vmstate.h"
#include "trace.h"

#define GPIO_MODER   0x00
#define GPIO_OTYPER  0x04
#define GPIO_OSPEEDR 0x08
#define GPIO_PUPDR   0x0C
#define GPIO_IDR     0x10
#define GPIO_ODR     0x14
#define GPIO_BSRR    0x18
#define GPIO_LCKR    0x1C
#define GPIO_AFRL    0x20
#define GPIO_AFRH    0x24
#define GPIO_BRR     0x28
#define GPIO_ASCR    0x2C

/* 0b11111111_11111111_00000000_00000000 */
#define RESERVED_BITS_MASK 0xFFFF0000

// regex should have `perif_name` in 1st capture group, `func_name` in 2nd capture group
static const char *af_name_regex[NUM_AF] = {
    "((.*))",                                           // SYS
    "((?:TIM)\\d+)_([^/]+)",                            // TIM1/2
    "((?:TIM)\\d+)_([^/]+)",                            // TIM3/4/5
    "((?:TIM)\\d+)_([^/]+)",                            // TIM8/9/10/11
    "((?:I2C)\\d+)_([^/]+)",                            // I2C1/2/3
    "((?:SPI)\\d+)_([^/]+)",                            // SPI1/2/3/4/5/6
    "((?:SPI|SAI|I2S)\\d+)(?:ext)?_([^/]+)",            // SPI2/3/SAI1
    "((?:SPI|SAI|I2S|UART|USART)\\d+)(?:ext)?_([^/]+)", // SPI3/USART1/2/3
    "((?:UART|USART)\\d+)_([^/]+)",                     // USART6/UART4/5/7/8
    "((?:TIM|CAN|LCD)\\d*)_([^/]+)",                    // CAN1/2/TIM12/13/14/LCD
    "((?:OTG_HS|OTG_FS)\\d*)_([^/]+)",                  // OTG2_HS/OTH1_FS
    "((?:ETH))_([^/]+)",                                // ETH
    "((?:OTG_HS|OTG_FS|FMC|SDIO)\\d*)_([^/]+)",         // FMC/SDIO/OTG2_FS
    "((?:DCMI)\\d*)_([^/]+)",                           // DCMI
    "((?:LCD)\\d*)_([^/]+)",                            // LCD
    "((.*))",                                           // SYS
};
typedef const char *port_af_map_type[GPIO_NUM_PINS][NUM_AF];
static const port_af_map_type porta_af_map = {
    {[1] = "TIM2_CH1/TIM2_ETR", [2] = "TIM5_CH1", [3] = "TIM8_ETR", [7] = "USART2_CTS", [8] = "UART4_TX", [11] = "ETH_MII_CRS", [15] = "EVENTOUT"},
    {[1] = "TIM2_CH2", [2] = "TIM5_CH2", [7] = "USART2_RTS", [8] = "UART4_RX", [11] = "ETH_MII_RX_CLK/ETH_RMII_REF_CLK", [15] = "EVENTOUT"},
    {[1] = "TIM2_CH3", [2] = "TIM5_CH3", [3] = "TIM9_CH1", [7] = "USART2_TX", [11] = "ETH_MDIO", [15] = "EVENTOUT"},
    {[1] = "TIM2_CH4", [2] = "TIM5_CH4", [3] = "TIM9_CH2", [7] = "USART2_RX", [10] = "OTG_HS_ULPI_D0", [11] = "ETH_MII_COL", [14] = "LCD_B5", [15] = "EVENTOUT"},
    {[5] = "SPI1_NSS", [6] = "SPI3_NSS/I2S3_WS", [7] = "USART2_CK", [12] = "OTG_HS_SOF", [13] = "DCMI_HSYNC", [14] = "LCD_VSYNC", [15] = "EVENTOUT"},
    {[1] = "TIM2_CH1/TIM2_ETR", [3] = "TIM8_CH1N", [5] = "SPI1_SCK", [10] = "OTG_HS_ULPI_CK", [15] = "EVENTOUT"},
    {[1] = "TIM1_BKIN", [2] = "TIM3_CH1", [3] = "TIM8_BKIN", [5] = "SPI1_MISO", [9] = "TIM13_CH1", [13] = "DCMI_PIXCLK", [14] = "LCD_G2", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH1N", [2] = "TIM3_CH2", [3] = "TIM8_CH1N", [5] = "SPI1_MOSI", [9] = "TIM14_CH1", [11] = "ETH_MII_RX_DV/ETH_RMII_CRS_DV", [15] = "EVENTOUT"},
    {[0] = "MCO1", [1] = "TIM1_CH1", [4] = "I2C3_SCL", [7] = "USART1_CK", [10] = "OTG_FS_SOF", [14] = "LCD_R6", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH2", [4] = "I2C3_SMBA", [7] = "USART1_TX", [13] = "DCMI_D0", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH3", [7] = "USART1_RX", [10] = "OTG_FS_ID", [13] = "DCMI_D1", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH4", [7] = "USART1_CTS", [9] = "CAN1_RX", [10] = "OTG_FS_DM", [14] = "LCD_R4", [15] = "EVENTOUT"},
    {[1] = "TIM1_ETR", [7] = "USART1_RTS", [9] = "CAN1_TX", [10] = "OTG_FS_DP", [14] = "LCD_R5", [15] = "EVENTOUT"},
    {[0] = "JTMSSWDIO", [15] = "EVENTOUT"},
    {[0] = "JTCKSWCLK", [15] = "EVENTOUT"},
    {[0] = "JTDI", [1] = "TIM2_CH1/TIM2_ETR", [5] = "SPI1_NSS", [6] = "SPI3_NSS/I2S3_WS", [15] = "EVENTOUT"},
};
static const port_af_map_type porte_af_map = {
    {[2] = "TIM4_ETR", [8] = "UART8_Rx", [12] = "FMC_NBL0", [13] = "DCMI_D2", [15] = "EVENTOUT"},
    {[8] = "UART8_Tx", [12] = "FMC_NBL1", [13] = "DCMI_D3", [15] = "EVENTOUT"},
    {[0] = "TRACECLK", [5] = "SPI4_SCK", [6] = "SAI1_MCLK_A", [11] = "ETH_MII_TXD3", [12] = "FMC_A23", [15] = "EVENTOUT"},
    {[0] = "TRACED0", [6] = "SAI1_SD_B", [12] = "FMC_A19", [15] = "EVENTOUT"},
    {[0] = "TRACED1", [5] = "SPI4_NSS", [6] = "SAI1_FS_A", [12] = "FMC_A20", [13] = "DCMI_D4", [14] = "LCD_B0", [15] = "EVENTOUT"},
    {[0] = "TRACED2", [3] = "TIM9_CH1", [5] = "SPI4_MISO", [6] = "SAI1_SCK_A", [12] = "FMC_A21", [13] = "DCMI_D6", [14] = "LCD_G0", [15] = "EVENTOUT"},
    {[0] = "TRACED3", [3] = "TIM9_CH2", [5] = "SPI4_MOSI", [6] = "SAI1_SD_A", [12] = "FMC_A22", [13] = "DCMI_D7", [14] = "LCD_G1", [15] = "EVENTOUT"},
    {[1] = "TIM1_ETR", [8] = "UART7_Rx", [12] = "FMC_D4", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH1N", [8] = "UART7_Tx", [12] = "FMC_D5", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH1", [12] = "FMC_D6", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH2N", [12] = "FMC_D7", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH2", [5] = "SPI4_NSS", [12] = "FMC_D8", [14] = "LCD_G3", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH3N", [5] = "SPI4_SCK", [12] = "FMC_D9", [14] = "LCD_B4", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH3", [5] = "SPI4_MISO", [12] = "FMC_D10", [14] = "LCD_DE", [15] = "EVENTOUT"},
    {[1] = "TIM1_CH4", [5] = "SPI4_MOSI", [12] = "FMC_D11", [14] = "LCD_CLK", [15] = "EVENTOUT"},
    {[1] = "TIM1_BKIN", [12] = "FMC_D12", [14] = "LCD_R7", [15] = "EVENTOUT"},
};
static const port_af_map_type portg_af_map = {
    {[12] = "FMC_A10", [15] = "EVENTOUT"},
    {[12] = "FMC_A11", [15] = "EVENTOUT"},
    {[12] = "FMC_A12", [15] = "EVENTOUT"},
    {[12] = "FMC_A13", [15] = "EVENTOUT"},
    {[12] = "FMC_A14/FMC_BA0", [15] = "EVENTOUT"},
    {[12] = "FMC_A15/FMC_BA1", [15] = "EVENTOUT"},
    {[12] = "FMC_INT2", [13] = "DCMI_D12", [14] = "LCD_R7", [15] = "EVENTOUT"},
    {[8] = "USART6_CK", [12] = "FMC_INT3", [13] = "DCMI_D13", [14] = "LCD_CLK", [15] = "EVENTOUT"},
    {[5] = "SPI6_NSS", [8] = "USART6_RTS", [11] = "ETH_PPS_OUT", [12] = "FMC_SDCLK", [15] = "EVENTOUT"},
    {[8] = "USART6_RX", [12] = "FMC_NE2/FMC_NCE3", [13] = "DCMI_VSYNC", [15] = "EVENTOUT"},
    {[9] = "LCD_G3", [12] = "FMC_NCE4_1/FMC_NE3", [13] = "DCMI_D2", [14] = "LCD_B2", [15] = "EVENTOUT"},
    {[11] = "ETH_MII_TX_EN/ETH_RMII_TX_EN", [12] = "FMC_NCE4_2", [13] = "DCMI_D3", [14] = "LCD_B3", [15] = "EVENTOUT"},
    {[5] = "SPI6_MISO", [8] = "USART6_RTS", [9] = "LCD_B4", [12] = "FMC_NE4", [14] = "LCD_B1", [15] = "EVENTOUT"},
    {[5] = "SPI6_SCK", [8] = "USART6_CTS", [11] = "ETH_MII_TXD0/ETH_RMII_TXD0", [12] = "FMC_A24", [15] = "EVENTOUT"},
    {[5] = "SPI6_MOSI", [8] = "USART6_TX", [11] = "ETH_MII_TXD1/ETH_RMII_TXD1", [12] = "FMC_A25", [15] = "EVENTOUT"},
    {[8] = "USART6_CTS", [12] = "FMC_SDNCAS", [13] = "DCMI_D13", [15] = "EVENTOUT"},
};
static const port_af_map_type *const port_af_map[NUM_GPIOS] = {
    &porta_af_map,
    NULL, // Unimplemented
    NULL, // Unimplemented
    NULL, // Unimplemented
    &porte_af_map,
    NULL, // Unimplemented
    &portg_af_map,
    NULL, // Unimplemented
    NULL, // Unimplemented
    NULL, // Unimplemented
    NULL, // Unimplemented
};

static void update_gpio_idr(STM32F429GpioState *s);
static void update_ppd_pin_val(STM32F429GpioState *s);

static bool is_pull_up(STM32F429GpioState *s, unsigned pin)
{
    return extract32(s->pupdr, 2 * pin, 2) == 1;
}

static bool is_pull_down(STM32F429GpioState *s, unsigned pin)
{
    return extract32(s->pupdr, 2 * pin, 2) == 2;
}

static bool is_output(STM32F429GpioState *s, unsigned pin)
{
    return extract32(s->moder, 2 * pin, 2) == 1;
}

static bool is_open_drain(STM32F429GpioState *s, unsigned pin)
{
    return extract32(s->otyper, pin, 1) == 1;
}

static bool is_push_pull(STM32F429GpioState *s, unsigned pin)
{
    return extract32(s->otyper, pin, 1) == 0;
}

static void stm32f429_gpio_reset_hold(Object *obj)
{
    STM32F429GpioState *s = STM32F429_GPIO(obj);

    s->moder   = s->moder_reset;
    s->otyper  = 0x00000000;
    s->ospeedr = s->ospeedr_reset;
    s->pupdr   = s->pupdr_reset;
    s->idr     = 0x00000000;
    s->odr     = 0x00000000;
    s->lckr    = 0x00000000;
    s->afrl    = 0x00000000;
    s->afrh    = 0x00000000;
    s->ascr    = 0x00000000;

    s->disconnected_pins   = 0xFFFF;
    s->pins_connected_high = 0x0000;
    update_gpio_idr(s);
}

static void stm32f429_gpio_set(void *opaque, int line, int level)
{
    STM32F429GpioState *s = opaque;
    /*
     * The pin isn't set if line is configured in output mode
     * except if level is 0 and the output is open-drain.
     * This way there will be no short-circuit prone situations.
     */
    if (is_output(s, line) && !(is_open_drain(s, line) && (level == 0))) {
        qemu_log_mask(LOG_GUEST_ERROR, "Line %d can't be driven externally\n",
                      line);
        return;
    }

    s->disconnected_pins &= ~(1 << line);
    if (level) {
        s->pins_connected_high |= (1 << line);
    } else {
        s->pins_connected_high &= ~(1 << line);
    }
    trace_stm32f429_gpio_pins(s->name, s->disconnected_pins,
                              s->pins_connected_high);
    update_gpio_idr(s);
}

static void update_gpio_idr(STM32F429GpioState *s)
{
    uint32_t new_idr_mask = 0;
    uint32_t new_idr      = s->odr;
    uint32_t old_idr      = s->idr;
    int new_pin_state, old_pin_state;

    for (int i = 0; i < GPIO_NUM_PINS; i++) {
        if (is_output(s, i)) {
            if (is_push_pull(s, i)) {
                new_idr_mask |= (1 << i);
            } else if (!(s->odr & (1 << i))) {
                /* open-drain ODR 0 */
                new_idr_mask |= (1 << i);
                /* open-drain ODR 1 */
            } else if (!(s->disconnected_pins & (1 << i)) &&
                       !(s->pins_connected_high & (1 << i))) {
                /* open-drain ODR 1 with pin connected low */
                new_idr_mask |= (1 << i);
                new_idr &= ~(1 << i);
                /* open-drain ODR 1 with unactive pin */
            } else if (is_pull_up(s, i)) {
                new_idr_mask |= (1 << i);
            } else if (is_pull_down(s, i)) {
                new_idr_mask |= (1 << i);
                new_idr &= ~(1 << i);
            }
            /*
             * The only case left is for open-drain ODR 1
             * with unactive pin without pull-up or pull-down :
             * the value is floating.
             */
            /* input or analog mode with connected pin */
        } else if (!(s->disconnected_pins & (1 << i))) {
            if (s->pins_connected_high & (1 << i)) {
                /* pin high */
                new_idr_mask |= (1 << i);
                new_idr |= (1 << i);
            } else {
                /* pin low */
                new_idr_mask |= (1 << i);
                new_idr &= ~(1 << i);
            }
            /* input or analog mode with disconnected pin */
        } else {
            if (is_pull_up(s, i)) {
                /* pull-up */
                new_idr_mask |= (1 << i);
                new_idr |= (1 << i);
            } else if (is_pull_down(s, i)) {
                /* pull-down */
                new_idr_mask |= (1 << i);
                new_idr &= ~(1 << i);
            }
            /*
             * The only case left is for a disconnected pin
             * without pull-up or pull-down :
             * the value is floating.
             */
        }
    }

    s->idr = (old_idr & ~new_idr_mask) | (new_idr & new_idr_mask);
    trace_stm32f429_gpio_update_idr(s->name, old_idr, s->idr);

    for (int i = 0; i < GPIO_NUM_PINS; i++) {
        if (new_idr_mask & (1 << i)) {
            new_pin_state = (new_idr & (1 << i)) > 0;
            old_pin_state = (old_idr & (1 << i)) > 0;
            if (new_pin_state > old_pin_state) {
                qemu_irq_raise(s->pin[i]);
            } else if (new_pin_state < old_pin_state) {
                qemu_irq_lower(s->pin[i]);
            }
        }
    }

    update_ppd_pin_val(s);
}

/*
 * Return mask of pins that are both configured in output
 * mode and externally driven (except pins in open-drain
 * mode externally set to 0).
 */
static uint32_t get_gpio_pinmask_to_disconnect(STM32F429GpioState *s)
{
    uint32_t pins_to_disconnect = 0;
    for (int i = 0; i < GPIO_NUM_PINS; i++) {
        /* for each connected pin in output mode */
        if (!(s->disconnected_pins & (1 << i)) && is_output(s, i)) {
            /* if either push-pull or high level */
            if (is_push_pull(s, i) || s->pins_connected_high & (1 << i)) {
                pins_to_disconnect |= (1 << i);
                qemu_log_mask(LOG_GUEST_ERROR,
                              "Line %d can't be driven externally\n", i);
            }
        }
    }
    return pins_to_disconnect;
}

/*
 * Set field `disconnected_pins` and call `update_gpio_idr()`
 */
static void disconnect_gpio_pins(STM32F429GpioState *s, uint16_t lines)
{
    s->disconnected_pins |= lines;
    trace_stm32f429_gpio_pins(s->name, s->disconnected_pins,
                              s->pins_connected_high);
    update_gpio_idr(s);
}

static void disconnected_pins_set(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    STM32F429GpioState *s = STM32F429_GPIO(obj);
    uint16_t value;
    if (!visit_type_uint16(v, name, &value, errp)) {
        return;
    }
    disconnect_gpio_pins(s, value);
}

static void disconnected_pins_get(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    visit_type_uint16(v, name, (uint16_t *)opaque, errp);
}

static void clock_freq_get(Object *obj, Visitor *v, const char *name,
                           void *opaque, Error **errp)
{
    STM32F429GpioState *s  = STM32F429_GPIO(obj);
    uint32_t clock_freq_hz = clock_get_hz(s->clk);
    visit_type_uint32(v, name, &clock_freq_hz, errp);
}

static void update_ppd_perif(STM32F429GpioState *s)
{
    PerifPinoutDeviceClass *k = PERIF_PINOUT_DEVICE_GET_CLASS(s->ppd);
    for (int pin_num = 0; pin_num < GPIO_NUM_PINS; ++pin_num) {
        int mode = (s->moder >> (pin_num * 2)) & 0b11;
        if (mode == 0b00) { // 00: Input (reset state)

        } else if (mode == 0b01) { // 01: General purpose output mode
            g_autofree gchar *pin_name   = g_strdup_printf("P%c%d", *s->name, pin_num);
            g_autofree gchar *perif_name = g_strdup_printf("GPIO%c", *s->name);
            k->register_perif_pin(k, perif_name, pin_name, "General purpose output");
        } else if (mode == 0b10) { // 10: Alternate function mode
            int af_num =
                (pin_num >= 8)
                    ? (af_num = (s->afrh >> ((pin_num - 8) * 4) & 0b1111))
                    : (af_num = (s->afrl >> (pin_num * 4) & 0b1111));

            const port_af_map_type *cur_af_map = port_af_map[(int)(*s->name - 'A')];
            if (!cur_af_map)
                continue;
            const char *func_desc = (*cur_af_map)[pin_num][af_num];
            if (!func_desc)
                continue;
            g_autoptr(GRegex) regex = g_regex_new(af_name_regex[af_num], 0, 0, NULL);
            g_autoptr(GMatchInfo) match_info;
            g_regex_match(regex, func_desc, 0, &match_info);

            g_autofree gchar *pin_name = g_strdup_printf("P%c%d", *s->name, pin_num);
            while (g_match_info_matches(match_info)) {
                g_autofree gchar *perif_name = g_match_info_fetch(match_info, 1);
                g_autofree gchar *func_name  = g_match_info_fetch(match_info, 2);
                g_match_info_next(match_info, NULL);
                k->register_perif_pin(k, perif_name, pin_name, func_name);
            }
        } else { // 11: Analog mode
            g_autofree gchar *pin_name   = g_strdup_printf("P%c%d", *s->name, pin_num);
            g_autofree gchar *perif_name = g_strdup_printf("GPIO%c", *s->name);
            k->register_perif_pin(k, perif_name, pin_name, "Analog");
        }
    }

    GString *setting = qobject_to_json_pretty(QOBJECT(k->peripheral_pins), true);
    qemu_log("peripheral_pins: \n%s\n", setting->str);
    g_string_free(setting, true);
}

static void update_ppd_pin_val(STM32F429GpioState *s)
{
    PerifPinoutDeviceClass *k = PERIF_PINOUT_DEVICE_GET_CLASS(s->ppd);

    g_autoptr(SCDataPack) data_pack = sc_datapack_new("PCB", TYPE_SC_INTERFACE_CONFIG, TYPE_SC_DATA);
    g_autofree char *perif_name     = g_strdup_printf("GPIO%s", s->name);

    for (int pin_num = 0; pin_num < GPIO_NUM_PINS; ++pin_num) {
        g_autofree char *pin_name = g_strdup_printf("P%c%d", *s->name, pin_num);
        const char *value         = ((s->idr >> pin_num) & 0b1) ? "5V" : "0V";
        k->set_pin_value(k, pin_name, value);
    }

    GString *setting = qobject_to_json_pretty(QOBJECT(k->pin_value), true);
    qemu_log("pin_value: \n%s\n", setting->str);
    g_string_free(setting, true);

    // force initiate a transaction to update GPIO values
    k->transport(k, perif_name, data_pack, data_pack);

    qemu_log("gpio transaction done\n");
}

static void stm32f429_gpio_write(void *opaque, hwaddr addr, uint64_t val64,
                                 unsigned int size)
{
    STM32F429GpioState *s = opaque;

    uint32_t value = val64;
    trace_stm32f429_gpio_write(s->name, addr, val64);

    qemu_log("stm32f429_gpio_write GPIO%c Addr: 0x%lx w/ 0x%lx\n", *s->name,
             addr, val64);

    switch (addr) {
    case GPIO_MODER:
        s->moder = value;
        disconnect_gpio_pins(s, get_gpio_pinmask_to_disconnect(s));
        update_ppd_perif(s);
        qemu_log_mask(LOG_UNIMP, "%s: Analog and AF modes aren't supported\n\
                       Analog and AF mode behave like input mode\n",
                      __func__);
        return;
    case GPIO_OTYPER:
        s->otyper = value & ~RESERVED_BITS_MASK;
        disconnect_gpio_pins(s, get_gpio_pinmask_to_disconnect(s));
        return;
    case GPIO_OSPEEDR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Changing I/O output speed isn't supported\n\
                       I/O speed is already maximal\n",
                      __func__);
        s->ospeedr = value;
        return;
    case GPIO_PUPDR:
        s->pupdr = value;
        update_gpio_idr(s);
        return;
    case GPIO_IDR:
        qemu_log_mask(LOG_UNIMP, "%s: GPIO->IDR is read-only\n", __func__);
        return;
    case GPIO_ODR:
        s->odr = value & ~RESERVED_BITS_MASK;
        update_gpio_idr(s);
        qemu_log("GPIO(%s) ODR sets to: 0x%016x\n", s->name, s->odr);
        return;
    case GPIO_BSRR: {
        uint32_t bits_to_reset = (value & RESERVED_BITS_MASK) >> GPIO_NUM_PINS;
        uint32_t bits_to_set   = value & ~RESERVED_BITS_MASK;
        /* If both BSx and BRx are set, BSx has priority.*/
        s->odr &= ~bits_to_reset;
        s->odr |= bits_to_set;
        update_gpio_idr(s);
        qemu_log("GPIO(%s) BSRR writes: 0x%x\n", s->name, value);
        qemu_log("GPIO(%s) ODR now is: 0x%x\n", s->name, s->odr);
        return;
    }
    case GPIO_LCKR:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Locking port bits configuration isn't supported\n",
                      __func__);
        s->lckr = value & ~RESERVED_BITS_MASK;
        return;
    case GPIO_AFRL:
        qemu_log_mask(LOG_UNIMP, "%s: Alternate functions aren't supported\n",
                      __func__);
        s->afrl = value;
        update_ppd_perif(s);
        qemu_log("GPIO%s sets P%s[%d~%d] alternate functions to AF[%08x]\n", s->name, s->name, 7, 0, s->afrl);
        return;
    case GPIO_AFRH:
        qemu_log_mask(LOG_UNIMP, "%s: Alternate functions aren't supported\n",
                      __func__);
        s->afrh = value;
        update_ppd_perif(s);
        qemu_log("GPIO%s sets P%s[%d~%d] alternate functions to AF[%08x]\n", s->name, s->name, 15, 8, s->afrl);
        return;
    case GPIO_BRR: {
        uint32_t bits_to_reset = value & ~RESERVED_BITS_MASK;
        s->odr &= ~bits_to_reset;
        update_gpio_idr(s);
        return;
    }
    case GPIO_ASCR:
        qemu_log_mask(LOG_UNIMP, "%s: ADC function isn't supported\n",
                      __func__);
        s->ascr = value & ~RESERVED_BITS_MASK;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }
}

static uint64_t stm32f429_gpio_read(void *opaque, hwaddr addr,
                                    unsigned int size)
{
    STM32F429GpioState *s = opaque;

    trace_stm32f429_gpio_read(s->name, addr);

    switch (addr) {
    case GPIO_MODER:
        return s->moder;
    case GPIO_OTYPER:
        return s->otyper;
    case GPIO_OSPEEDR:
        return s->ospeedr;
    case GPIO_PUPDR:
        return s->pupdr;
    case GPIO_IDR:
        return s->idr;
    case GPIO_ODR:
        return s->odr;
    case GPIO_BSRR:
        return 0;
    case GPIO_LCKR:
        return s->lckr;
    case GPIO_AFRL:
        return s->afrl;
    case GPIO_AFRH:
        return s->afrh;
    case GPIO_BRR:
        return 0;
    case GPIO_ASCR:
        return s->ascr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
        return 0;
    }
}

static const MemoryRegionOps stm32f429_gpio_ops = {
    .read       = stm32f429_gpio_read,
    .write      = stm32f429_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl =
        {
            .min_access_size = 4,
            .max_access_size = 4,
            .unaligned       = false,
        },
    .valid =
        {
            .min_access_size = 4,
            .max_access_size = 4,
            .unaligned       = false,
        },
};

static void stm32f429_gpio_init(Object *obj)
{
    STM32F429GpioState *s = STM32F429_GPIO(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f429_gpio_ops, s,
                          TYPE_STM32F429_GPIO, 0x400);

    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    qdev_init_gpio_out(DEVICE(obj), s->pin, GPIO_NUM_PINS);
    qdev_init_gpio_in(DEVICE(obj), stm32f429_gpio_set, GPIO_NUM_PINS);

    s->clk = qdev_init_clock_in(DEVICE(s), "clk", NULL, s, 0);

    object_property_add(obj, "disconnected-pins", "uint16",
                        disconnected_pins_get, disconnected_pins_set, NULL,
                        &s->disconnected_pins);
    object_property_add(obj, "clock-freq-hz", "uint32", clock_freq_get, NULL,
                        NULL, NULL);

    s->ppd = PERIF_PINOUT_DEVICE(qdev_new(TYPE_PERIF_PINOUT_DEVICE));
}

static void stm32f429_gpio_realize(DeviceState *dev, Error **errp)
{
    STM32F429GpioState *s = STM32F429_GPIO(dev);
    if (!clock_has_source(s->clk)) {
        error_setg(errp, "GPIO: clk input must be connected");
        return;
    }
}

static const VMStateDescription vmstate_stm32f429_gpio = {
    .name               = TYPE_STM32F429_GPIO,
    .version_id         = 1,
    .minimum_version_id = 1,
    .fields             = (VMStateField[]){
        VMSTATE_UINT32(moder, STM32F429GpioState),
        VMSTATE_UINT32(otyper, STM32F429GpioState),
        VMSTATE_UINT32(ospeedr, STM32F429GpioState),
        VMSTATE_UINT32(pupdr, STM32F429GpioState),
        VMSTATE_UINT32(idr, STM32F429GpioState),
        VMSTATE_UINT32(odr, STM32F429GpioState),
        VMSTATE_UINT32(lckr, STM32F429GpioState),
        VMSTATE_UINT32(afrl, STM32F429GpioState),
        VMSTATE_UINT32(afrh, STM32F429GpioState),
        VMSTATE_UINT32(ascr, STM32F429GpioState),
        VMSTATE_UINT16(disconnected_pins, STM32F429GpioState),
        VMSTATE_UINT16(pins_connected_high, STM32F429GpioState),
        VMSTATE_END_OF_LIST()}};

static Property stm32f429_gpio_properties[] = {
    DEFINE_PROP_STRING("name", STM32F429GpioState, name),
    DEFINE_PROP_UINT32("mode-reset", STM32F429GpioState, moder_reset, 0),
    DEFINE_PROP_UINT32("ospeed-reset", STM32F429GpioState, ospeedr_reset, 0),
    DEFINE_PROP_UINT32("pupd-reset", STM32F429GpioState, pupdr_reset, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f429_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc     = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    device_class_set_props(dc, stm32f429_gpio_properties);
    dc->vmsd        = &vmstate_stm32f429_gpio;
    dc->realize     = stm32f429_gpio_realize;
    rc->phases.hold = stm32f429_gpio_reset_hold;
}

static const TypeInfo stm32f429_gpio_types[] = {
    {
        .name          = TYPE_STM32F429_GPIO,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(STM32F429GpioState),
        .instance_init = stm32f429_gpio_init,
        .class_init    = stm32f429_gpio_class_init,
    },
};

DEFINE_TYPES(stm32f429_gpio_types)
