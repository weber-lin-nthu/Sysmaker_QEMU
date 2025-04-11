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

#ifndef HW_STM32F429_GPIO_H
#define HW_STM32F429_GPIO_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_STM32F429_GPIO "stm32f429-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F429GpioState, STM32F429_GPIO)

#define NUM_GPIOS     11
#define GPIO_NUM_PINS 16

struct STM32F429GpioState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    /* GPIO registers */
    uint32_t moder;
    uint32_t otyper;
    uint32_t ospeedr;
    uint32_t pupdr;
    uint32_t idr;
    uint32_t odr;
    uint32_t lckr;
    uint32_t afrl;
    uint32_t afrh;
    uint32_t ascr;

    /* GPIO registers reset values */
    uint32_t moder_reset;
    uint32_t ospeedr_reset;
    uint32_t pupdr_reset;

    /*
     * External driving of pins.
     * The pins can be set externally through the device
     * anonymous input GPIOs lines under certain conditions.
     * The pin must not be in push-pull output mode,
     * and can't be set high in open-drain mode.
     * Pins driven externally and configured to
     * output mode will in general be "disconnected"
     * (see `get_gpio_pinmask_to_disconnect()`)
     */
    uint16_t disconnected_pins;
    uint16_t pins_connected_high;

    char *name;
    Clock *clk;
    qemu_irq pin[GPIO_NUM_PINS];
};

#endif
