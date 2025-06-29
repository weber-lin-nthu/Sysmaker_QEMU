/*
 * STM32F429 RCC (Reset and clock control)
 *
 * Copyright (c) 2023 Arnaud Minier <arnaud.minier@telecom-paris.fr>
 * Copyright (c) 2023 Inès Varhol <ines.varhol@telecom-paris.fr>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 * The reference used is the STMicroElectronics RM0351 Reference manual
 * for STM32F429 and STM32L4x6 advanced Arm ® -based 32-bit MCUs.
 *
 * Inspired by the BCM2835 CPRMAN clock manager implementation by Luc Michel.
 */

#ifndef HW_STM32F429_RCC_INTERNALS_H
#define HW_STM32F429_RCC_INTERNALS_H

#include "hw/registerfields.h"
#include "hw/misc/stm32f429_rcc.h"

#define TYPE_RCC_CLOCK_MUX "stm32f429-rcc-clock-mux"
#define TYPE_RCC_PLL       "stm32f429-rcc-pll"

OBJECT_DECLARE_SIMPLE_TYPE(RccClockMuxState, RCC_CLOCK_MUX)
OBJECT_DECLARE_SIMPLE_TYPE(RccPllState, RCC_PLL)

/* Register map */
REG32(CR, 0x00)
FIELD(CR, PLLSAIRDY, 29, 1)
FIELD(CR, PLLSAION, 28, 1)
FIELD(CR, PLLI2SRDY, 27, 1)
FIELD(CR, PLLI2SON, 26, 1)
FIELD(CR, PLLRDY, 25, 1)
FIELD(CR, PLLON, 24, 1)
FIELD(CR, CSSON, 19, 1)
FIELD(CR, HSEBYP, 18, 1)
FIELD(CR, HSERDY, 17, 1)
FIELD(CR, HSEON, 16, 1)
FIELD(CR, HSICAL, 8, 8)
FIELD(CR, HSITRIM, 3, 4)
FIELD(CR, HSIRDY, 1, 1)
FIELD(CR, HSION, 0, 1)
REG32(PLLCFGR, 0x04)
FIELD(PLLCFGR, PLLQ, 24, 4)
FIELD(PLLCFGR, PLLSRC, 22, 1)
FIELD(PLLCFGR, PLLP, 16, 2)
FIELD(PLLCFGR, PLLN, 6, 9)
FIELD(PLLCFGR, PLLM, 0, 6)
REG32(CFGR, 0x08)
FIELD(CFGR, MCO2, 30, 2)
FIELD(CFGR, MCO2PRE, 27, 3)
FIELD(CFGR, MCO1PRE, 24, 3)
FIELD(CFGR, I2SSCR, 23, 1)
FIELD(CFGR, MCO1, 21, 2)
FIELD(CFGR, RTCPRE, 16, 5)
FIELD(CFGR, PPRE2, 13, 3)
FIELD(CFGR, PPRE1, 10, 3)
FIELD(CFGR, HPRE, 4, 4)
FIELD(CFGR, SWS, 2, 2)
FIELD(CFGR, SW, 0, 2)
REG32(CIR, 0x0C)
FIELD(CIR, CSSC, 23, 1)
FIELD(CIR, PLLSAIRDYC, 22, 1)
FIELD(CIR, PLLI2SRDYC, 21, 1)
FIELD(CIR, PLLRDYC, 20, 1)
FIELD(CIR, HSERDYC, 19, 1)
FIELD(CIR, HSIRDYC, 18, 1)
FIELD(CIR, LSERDYC, 17, 1)
FIELD(CIR, LSIRDYC, 16, 1)
FIELD(CIR, PLLSAIRDYIE, 14, 1)
FIELD(CIR, PLLI2SRDYIE, 13, 1)
FIELD(CIR, PLLRDYIE, 12, 1)
FIELD(CIR, HSERDYIE, 11, 1)
FIELD(CIR, HSIRDYIE, 10, 1)
FIELD(CIR, LSERDYIE, 9, 1)
FIELD(CIR, LSIRDYIE, 8, 1)
FIELD(CIR, CSSF, 7, 1)
FIELD(CIR, PLLSAIRDYF, 6, 1)
FIELD(CIR, PLLI2SRDYF, 5, 1)
FIELD(CIR, PLLRDYF, 4, 1)
FIELD(CIR, HSERDYF, 3, 1)
FIELD(CIR, HSIRDYF, 2, 1)
FIELD(CIR, LSERDYF, 1, 1)
FIELD(CIR, LSIRDYF, 0, 1)
REG32(PLLI2SCFGR, 0x84)
FIELD(PLLI2SCFGR, PLLI2SR, 28, 3)
FIELD(PLLI2SCFGR, PLLI2SQ, 24, 4)
FIELD(PLLI2SCFGR, PLLI2SN, 6, 9)
REG32(PLLSAICFGR, 0x88)
FIELD(PLLSAICFGR, PLLSAIR, 28, 3)
FIELD(PLLSAICFGR, PLLSAIQ, 24, 4)
FIELD(PLLSAICFGR, PLLSAIN, 6, 9)
// REG32(CIER, 0x18) // TODO: below are not yet updated
// /* HSI48RDYIE: only on STM32L496xx/4A6xx devices */
// FIELD(CIER, LSECSSIE, 9, 1)
// FIELD(CIER, PLLSAI2RDYIE, 7, 1)
// FIELD(CIER, PLLSAIRDYIE, 6, 1)
// FIELD(CIER, PLLRDYIE, 5, 1)
// FIELD(CIER, HSERDYIE, 4, 1)
// FIELD(CIER, HSIRDYIE, 3, 1)
// FIELD(CIER, MSIRDYIE, 2, 1)
// FIELD(CIER, LSERDYIE, 1, 1)
// FIELD(CIER, LSIRDYIE, 0, 1)
// REG32(CIFR, 0x1C)
// /* HSI48RDYF: only on STM32L496xx/4A6xx devices */
// FIELD(CIFR, LSECSSF, 9, 1)
// FIELD(CIFR, CSSF, 8, 1)
// FIELD(CIFR, PLLSAI2RDYF, 7, 1)
// FIELD(CIFR, PLLSAIRDYF, 6, 1)
// FIELD(CIFR, PLLRDYF, 5, 1)
// FIELD(CIFR, HSERDYF, 4, 1)
// FIELD(CIFR, HSIRDYF, 3, 1)
// FIELD(CIFR, MSIRDYF, 2, 1)
// FIELD(CIFR, LSERDYF, 1, 1)
// FIELD(CIFR, LSIRDYF, 0, 1)
// REG32(CICR, 0x20)
// /* HSI48RDYC: only on STM32L496xx/4A6xx devices */
// FIELD(CICR, LSECSSC, 9, 1)
// FIELD(CICR, CSSC, 8, 1)
// FIELD(CICR, PLLSAI2RDYC, 7, 1)
// FIELD(CICR, PLLSAIRDYC, 6, 1)
// FIELD(CICR, PLLRDYC, 5, 1)
// FIELD(CICR, HSERDYC, 4, 1)
// FIELD(CICR, HSIRDYC, 3, 1)
// FIELD(CICR, MSIRDYC, 2, 1)
// FIELD(CICR, LSERDYC, 1, 1)
// FIELD(CICR, LSIRDYC, 0, 1)
// REG32(AHB1RSTR, 0x28)
// REG32(AHB2RSTR, 0x2C)
// REG32(AHB3RSTR, 0x30)
// REG32(APB1RSTR1, 0x38)
// REG32(APB1RSTR2, 0x3C)
// REG32(APB2RSTR, 0x40)
// REG32(AHB1ENR, 0x48)
// /* DMA2DEN: reserved for STM32L475xx */
// FIELD(AHB1ENR, TSCEN, 16, 1)
// FIELD(AHB1ENR, CRCEN, 12, 1)
// FIELD(AHB1ENR, FLASHEN, 8, 1)
// FIELD(AHB1ENR, DMA2EN, 1, 1)
// FIELD(AHB1ENR, DMA1EN, 0, 1)
// REG32(AHB2ENR, 0x4C)
// FIELD(AHB2ENR, RNGEN, 18, 1)
// /* HASHEN: reserved for STM32L475xx */
// FIELD(AHB2ENR, AESEN, 16, 1)
// /* DCMIEN: reserved for STM32L475xx */
// FIELD(AHB2ENR, ADCEN, 13, 1)
// FIELD(AHB2ENR, OTGFSEN, 12, 1)
// /* GPIOIEN: reserved for STM32L475xx */
// FIELD(AHB2ENR, GPIOHEN, 7, 1)
// FIELD(AHB2ENR, GPIOGEN, 6, 1)
// FIELD(AHB2ENR, GPIOFEN, 5, 1)
// FIELD(AHB2ENR, GPIOEEN, 4, 1)
// FIELD(AHB2ENR, GPIODEN, 3, 1)
// FIELD(AHB2ENR, GPIOCEN, 2, 1)
// FIELD(AHB2ENR, GPIOBEN, 1, 1)
// FIELD(AHB2ENR, GPIOAEN, 0, 1)
// REG32(AHB3ENR, 0x50)
// FIELD(AHB3ENR, QSPIEN, 8, 1)
// FIELD(AHB3ENR, FMCEN, 0, 1)
// REG32(APB1ENR1, 0x58)
// FIELD(APB1ENR1, LPTIM1EN, 31, 1)
// FIELD(APB1ENR1, OPAMPEN, 30, 1)
// FIELD(APB1ENR1, DAC1EN, 29, 1)
// FIELD(APB1ENR1, PWREN, 28, 1)
// FIELD(APB1ENR1, CAN2EN, 26, 1)
// FIELD(APB1ENR1, CAN1EN, 25, 1)
// /* CRSEN: reserved for STM32L475xx */
// FIELD(APB1ENR1, I2C3EN, 23, 1)
// FIELD(APB1ENR1, I2C2EN, 22, 1)
// FIELD(APB1ENR1, I2C1EN, 21, 1)
// FIELD(APB1ENR1, UART5EN, 20, 1)
// FIELD(APB1ENR1, UART4EN, 19, 1)
// FIELD(APB1ENR1, USART3EN, 18, 1)
// FIELD(APB1ENR1, USART2EN, 17, 1)
// FIELD(APB1ENR1, SPI3EN, 15, 1)
// FIELD(APB1ENR1, SPI2EN, 14, 1)
// FIELD(APB1ENR1, WWDGEN, 11, 1)
// /* RTCAPBEN: reserved for STM32L475xx */
// FIELD(APB1ENR1, LCDEN, 9, 1)
// FIELD(APB1ENR1, TIM7EN, 5, 1)
// FIELD(APB1ENR1, TIM6EN, 4, 1)
// FIELD(APB1ENR1, TIM5EN, 3, 1)
// FIELD(APB1ENR1, TIM4EN, 2, 1)
// FIELD(APB1ENR1, TIM3EN, 1, 1)
// FIELD(APB1ENR1, TIM2EN, 0, 1)
// REG32(APB1ENR2, 0x5C)
// FIELD(APB1ENR2, LPTIM2EN, 5, 1)
// FIELD(APB1ENR2, SWPMI1EN, 2, 1)
// /* I2C4EN: reserved for STM32L475xx */
// FIELD(APB1ENR2, LPUART1EN, 0, 1)
// REG32(APB2ENR, 0x60)
// FIELD(APB2ENR, DFSDM1EN, 24, 1)
// FIELD(APB2ENR, SAI2EN, 22, 1)
// FIELD(APB2ENR, SAI1EN, 21, 1)
// FIELD(APB2ENR, TIM17EN, 18, 1)
// FIELD(APB2ENR, TIM16EN, 17, 1)
// FIELD(APB2ENR, TIM15EN, 16, 1)
// FIELD(APB2ENR, USART1EN, 14, 1)
// FIELD(APB2ENR, TIM8EN, 13, 1)
// FIELD(APB2ENR, SPI1EN, 12, 1)
// FIELD(APB2ENR, TIM1EN, 11, 1)
// FIELD(APB2ENR, SDMMC1EN, 10, 1)
// FIELD(APB2ENR, FWEN, 7, 1)
// FIELD(APB2ENR, SYSCFGEN, 0, 1)
// REG32(AHB1SMENR, 0x68)
// REG32(AHB2SMENR, 0x6C)
// REG32(AHB3SMENR, 0x70)
// REG32(APB1SMENR1, 0x78)
// REG32(APB1SMENR2, 0x7C)
// REG32(APB2SMENR, 0x80)
// REG32(CCIPR, 0x88)
// FIELD(CCIPR, DFSDM1SEL, 31, 1)
// FIELD(CCIPR, SWPMI1SEL, 30, 1)
// FIELD(CCIPR, ADCSEL, 28, 2)
// FIELD(CCIPR, CLK48SEL, 26, 2)
// FIELD(CCIPR, SAI2SEL, 24, 2)
// FIELD(CCIPR, SAI1SEL, 22, 2)
// FIELD(CCIPR, LPTIM2SEL, 20, 2)
// FIELD(CCIPR, LPTIM1SEL, 18, 2)
// FIELD(CCIPR, I2C3SEL, 16, 2)
// FIELD(CCIPR, I2C2SEL, 14, 2)
// FIELD(CCIPR, I2C1SEL, 12, 2)
// FIELD(CCIPR, LPUART1SEL, 10, 2)
// FIELD(CCIPR, UART5SEL, 8, 2)
// FIELD(CCIPR, UART4SEL, 6, 2)
// FIELD(CCIPR, USART3SEL, 4, 2)
// FIELD(CCIPR, USART2SEL, 2, 2)
// FIELD(CCIPR, USART1SEL, 0, 2)
// REG32(BDCR, 0x90)
// FIELD(BDCR, LSCOSEL, 25, 1)
// FIELD(BDCR, LSCOEN, 24, 1)
// FIELD(BDCR, BDRST, 16, 1)
// FIELD(BDCR, RTCEN, 15, 1)
// FIELD(BDCR, RTCSEL, 8, 2)
// FIELD(BDCR, LSECSSD, 6, 1)
// FIELD(BDCR, LSECSSON, 5, 1)
// FIELD(BDCR, LSEDRV, 3, 2)
// FIELD(BDCR, LSEBYP, 2, 1)
// FIELD(BDCR, LSERDY, 1, 1)
// FIELD(BDCR, LSEON, 0, 1)
// REG32(CSR, 0x94)
// FIELD(CSR, LPWRRSTF, 31, 1)
// FIELD(CSR, WWDGRSTF, 30, 1)
// FIELD(CSR, IWDGRSTF, 29, 1)
// FIELD(CSR, SFTRSTF, 28, 1)
// FIELD(CSR, PORRSTF, 27, 1)
// FIELD(CSR, PINRSTF, 26, 1)
// FIELD(CSR, BORRSTF, 25, 1)
// FIELD(CSR, RMVF, 24, 1)
// FIELD(CSR, LSIRDY, 1, 1)
// FIELD(CSR, LSION, 0, 1)
/* CRRCR and CCIPR2 registers are present on L496/L4A6 devices only. */

/* Read Only masks to prevent writes in unauthorized bits */
#define CR_READ_ONLY_MASK (R_CR_PLLSAIRDY_MASK | \
                           R_CR_PLLI2SRDY_MASK | \
                           R_CR_PLLRDY_MASK |    \
                           R_CR_HSERDY_MASK |    \
                           R_CR_HSICAL_MASK |    \
                           R_CR_HSIRDY_MASK)
#define CR_READ_SET_MASK (R_CR_PLLSAION_MASK | \
                          R_CR_PLLI2SON_MASK | \
                          R_CR_PLLON_MASK |    \
                          R_CR_CSSON_MASK |    \
                          R_CR_HSEBYP_MASK |   \
                          R_CR_HSEON_MASK |    \
                          R_CR_HSITRIM_MASK |  \
                          R_CR_HSION_MASK)
#define CFGR_READ_ONLY_MASK   (R_CFGR_SWS_MASK)
#define APB2ENR_READ_SET_MASK (R_APB2ENR_FWEN_MASK)
#define BDCR_READ_ONLY_MASK   (R_BDCR_LSECSSD_MASK | R_BDCR_LSERDY_MASK)
#define CSR_READ_ONLY_MASK    (R_CSR_LPWRRSTF_MASK | \
                            R_CSR_WWDGRSTF_MASK |    \
                            R_CSR_SFTRSTF_MASK |     \
                            R_CSR_BORRSTF_MASK |     \
                            R_CSR_PINRSTF_MASK |     \
                            R_CSR_LSIRDY_MASK)

/* Pll Channels */
enum PllChannels {
    RCC_PLL_CHANNEL_PLLCLK  = 0,
    RCC_PLL_CHANNEL_PLL48CK = 1,
    /* No R channel */
};

enum PLLI2SChannels {
    /* No P channel */
    RCC_PLLI2S_CHANNEL_PLLSAICLK = 1,
    RCC_PLLI2S_CHANNEL_PLLI2SCLK = 2,
};

enum PLLSAIChannels {
    /* No P channel */
    RCC_PLLSAI_CHANNEL_PLLSAICLK = 1,
    RCC_PLLSAI_CHANNEL_PLLCDCLK  = 2,
};

typedef enum RccClockMuxSource {
    RCC_CLOCK_MUX_SRC_GND = 0,
    RCC_CLOCK_MUX_SRC_HSI,
    RCC_CLOCK_MUX_SRC_LSE,
    RCC_CLOCK_MUX_SRC_HSE,
    RCC_CLOCK_MUX_SRC_PLLCLK,
    RCC_CLOCK_MUX_SRC_LSI,
    RCC_CLOCK_MUX_SRC_PLL48CK,
    RCC_CLOCK_MUX_SRC_PLLSAICLK_I2S,
    RCC_CLOCK_MUX_SRC_PLLI2SCLK,
    RCC_CLOCK_MUX_SRC_PLLSAICLK_SAI,
    RCC_CLOCK_MUX_SRC_PLLCDCLK,
    RCC_CLOCK_MUX_SRC_I2S_CKIN,
    RCC_CLOCK_MUX_SRC_SYSCLK,
    RCC_CLOCK_MUX_SRC_HCLK,
    RCC_CLOCK_MUX_SRC_PCLK1,
    RCC_CLOCK_MUX_SRC_PCLK2,
    RCC_CLOCK_MUX_SRC_HSE_RTC,
    RCC_CLOCK_MUX_SRC_RTCCLK,

    RCC_CLOCK_MUX_SRC_NUMBER,
} RccClockMuxSource;

/* PLL init info */
typedef struct PllInitInfo {
    const char *name;

    const char *channel_name[RCC_NUM_CHANNEL_PLL_OUT];
    bool channel_exists[RCC_NUM_CHANNEL_PLL_OUT];
    uint32_t default_channel_divider[RCC_NUM_CHANNEL_PLL_OUT];

    RccClockMuxSource src_mapping[RCC_NUM_CLOCK_MUX_SRC];
} PllInitInfo;

static const PllInitInfo PLL_INIT_INFO[] = {
    [RCC_PLL_PLL] = {
        .name         = "pll",
        .channel_name = {
            "pllclk",
            "pll48ck",
            NULL},
        .channel_exists = {true, true, false},
        /* From PLLCFGR register documentation */
        .default_channel_divider = {2, 4, 0}},
    [RCC_PLL_PLLI2S] = {
        .name         = "plli2s",
        .channel_name = {
            NULL,
            "pllsaiclk",
            "plli2sclk",
        },
        .channel_exists = {false, true, true},
        /* From PLLCFGR register documentation */
        .default_channel_divider = {0, 4, 2},
    },
    [RCC_PLL_PLLSAI] = {
        .name         = "pllsai",
        .channel_name = {
            NULL,
            "pllsaiclk",
            "plllcdclk",
        },
        .channel_exists = {false, true, true},
        /* From PLLSAICFGR register documentation */
        .default_channel_divider = {0, 4, 2},
    },
};

static inline void set_pll_init_info(RccPllState *pll,
                                     RccPll id)
{
    int i;

    pll->id             = id;
    pll->vco_multiplier = 1;
    for (i = 0; i < RCC_NUM_CHANNEL_PLL_OUT; i++) {
        pll->channel_enabled[i] = true; // enable all channel under PLL since stm32f429 didn't have dedicated register to turn on/off P,Q,R channel
        pll->channel_exists[i]  = PLL_INIT_INFO[id].channel_exists[i];
        pll->channel_divider[i] = PLL_INIT_INFO[id].default_channel_divider[i];
    }
}

/* Clock mux init info */
typedef struct ClockMuxInitInfo {
    const char *name;

    uint32_t multiplier;
    uint32_t divider;
    bool enabled;
    /* If this is true, the clock will not be exposed outside of the device */
    bool hidden;

    RccClockMuxSource src_mapping[RCC_NUM_CLOCK_MUX_SRC];
} ClockMuxInitInfo;

#define FILL_DEFAULT_FACTOR \
    .multiplier = 1,        \
    .divider    = 1

#define FILL_DEFAULT_INIT_ENABLED \
    FILL_DEFAULT_FACTOR,          \
        .enabled = true

#define FILL_DEFAULT_INIT_DISABLED \
    FILL_DEFAULT_FACTOR,           \
        .enabled = false

static const ClockMuxInitInfo CLOCK_MUX_INIT_INFO[] = {
    [RCC_CLOCK_MUX_SYSCLK] = {
        .name = "sysclk",
        /* Same mapping as: CFGR_SW */
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HSI,
            RCC_CLOCK_MUX_SRC_HSE,
            RCC_CLOCK_MUX_SRC_PLLCLK,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    [RCC_CLOCK_MUX_PLL_INPUT] = {
        .name = "pll-input",
        /* Same mapping as: PLLCFGR_PLLSRC */
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HSI,
            RCC_CLOCK_MUX_SRC_HSE,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    [RCC_CLOCK_MUX_HCLK] = {
        .name        = "hclk",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    [RCC_CLOCK_MUX_PCLK1] = {
        .name        = "pclk1",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HCLK,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    [RCC_CLOCK_MUX_PCLK2] = {
        .name        = "pclk2",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HCLK,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    [RCC_CLOCK_MUX_HSE_RTC] = {
        .name        = "hse_rtc",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HSE,
        },
        .hidden = true,
    },
    [RCC_CLOCK_MUX_RTCCLK] = {
        .name        = "rtcclk",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_LSI,
            RCC_CLOCK_MUX_SRC_LSE,
            RCC_CLOCK_MUX_HSE_RTC,
        },
        .hidden = true,
        FILL_DEFAULT_INIT_ENABLED,
    },
    /* From now on, muxes with a publicly available output */
    [RCC_CLOCK_MUX_CORTEX_REFCLK] = {
        .name       = "cortex-refclk",
        .multiplier = 1,
        /* REFCLK is always HCLK/8 */
        .divider     = 8,
        .enabled     = true,
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HCLK,
        },
    },
    // [RCC_CLOCK_MUX_USART1]        = {
    //            .name = "usart1",
    //     /* Same mapping as: CCIPR_USART1SEL */
    //            .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_USART2] = {
    //     .name = "usart2",
    //     /* Same mapping as: CCIPR_USART2SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_USART3] = {
    //     .name = "usart3",
    //     /* Same mapping as: CCIPR_USART3SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_UART4] = {
    //     .name = "uart4",
    //     /* Same mapping as: CCIPR_UART4SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_UART5] = {
    //     .name = "uart5",
    //     /* Same mapping as: CCIPR_UART5SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_LPUART1] = {
    //     .name = "lpuart1",
    //     /* Same mapping as: CCIPR_LPUART1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_I2C1] = {
    //     .name = "i2c1",
    //     /* Same mapping as: CCIPR_I2C1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_I2C2] = {
    //     .name = "i2c2",
    //     /* Same mapping as: CCIPR_I2C2SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_I2C3] = {
    //     .name = "i2c3",
    //     /* Same mapping as: CCIPR_I2C3SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_LPTIM1] = {
    //     .name = "lptim1",
    //     /* Same mapping as: CCIPR_LPTIM1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_LSI,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_LPTIM2] = {
    //     .name = "lptim2",
    //     /* Same mapping as: CCIPR_LPTIM2SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_LSI,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SWPMI1] = {
    //     .name = "swpmi1",
    //     /* Same mapping as: CCIPR_SWPMI1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //         RCC_CLOCK_MUX_SRC_HSI,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    [RCC_CLOCK_MUX_MCO1] = {
        .name = "mco1",
        /* Same mapping as: CFGR_MCO1 */
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HSI,
            RCC_CLOCK_MUX_SRC_LSE,
            RCC_CLOCK_MUX_SRC_HSE,
            RCC_CLOCK_MUX_SRC_PLLCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_MCO2] = {
        .name = "mco2",
        /* Same mapping as: CFGR_MCO2 */
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
            RCC_CLOCK_MUX_SRC_PLLI2SCLK,
            RCC_CLOCK_MUX_SRC_HSE,
            RCC_CLOCK_MUX_SRC_PLLCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    // [RCC_CLOCK_MUX_LSCO] = {
    //     .name = "lsco",
    //     /* Same mapping as: BDCR_LSCOSEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_LSI,
    //         RCC_CLOCK_MUX_SRC_LSE,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_DFSDM1] = {
    //     .name = "dfsdm1",
    //     /* Same mapping as: CCIPR_DFSDM1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_ADC] = {
    //     .name = "adc",
    //     /* Same mapping as: CCIPR_ADCSEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_GND,
    //         RCC_CLOCK_MUX_SRC_PLLADC1,
    //         RCC_CLOCK_MUX_SRC_PLLADC2,
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_CLK48] = {
    //     .name = "clk48",
    //     /* Same mapping as: CCIPR_CLK48SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_GND,
    //         RCC_CLOCK_MUX_SRC_PLL48M2,
    //         RCC_CLOCK_MUX_SRC_PLL48M1,
    //         RCC_CLOCK_MUX_SRC_MSI,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SAI2] = {
    //     .name = "sai2",
    //     /* Same mapping as: CCIPR_SAI2SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PLLSAI,
    //         RCC_CLOCK_MUX_SRC_PLLSAI2,
    //         RCC_CLOCK_MUX_SRC_PLLSAI3,
    //         RCC_CLOCK_MUX_SRC_SAI2_EXTCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SAI1] = {
    //     .name = "sai1",
    //     /* Same mapping as: CCIPR_SAI1SEL */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PLLSAI,
    //         RCC_CLOCK_MUX_SRC_PLLSAI2,
    //         RCC_CLOCK_MUX_SRC_PLLSAI3,
    //         RCC_CLOCK_MUX_SRC_SAI1_EXTCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    /* From now on, these muxes only have one valid source */
    // [RCC_CLOCK_MUX_TSC] = {
    //     .name        = "tsc",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_CRC] = {
    //     .name        = "crc",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_FLASH] = {
    //     .name        = "flash",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_DMA2] = {
    //     .name        = "dma2",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_DMA1] = {
    //     .name        = "dma1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_RNG] = {
    //     .name        = "rng",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_AES] = {
    //     .name        = "aes",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_OTGFS] = {
    //     .name        = "otgfs",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    [RCC_CLOCK_MUX_GPIOA] = {
        .name        = "gpioa",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOB] = {
        .name        = "gpiob",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOC] = {
        .name        = "gpioc",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOD] = {
        .name        = "gpiod",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOE] = {
        .name        = "gpioe",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOF] = {
        .name        = "gpiof",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOG] = {
        .name        = "gpiog",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOH] = {
        .name        = "gpioh",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOI] = {
        .name        = "gpioi",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOJ] = {
        .name        = "gpioj",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    [RCC_CLOCK_MUX_GPIOK] = {
        .name        = "gpiok",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_SYSCLK,
        },
        FILL_DEFAULT_INIT_DISABLED,
    },
    // [RCC_CLOCK_MUX_QSPI] = {
    //     .name        = "qspi",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_FMC] = {
    //     .name        = "fmc",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_OPAMP] = {
    //     .name        = "opamp",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_DAC1] = {
    //     .name        = "dac1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_PWR] = {
    //     .name = "pwr",
    //     /*
    //      * PWREN is in the APB1ENR1 register,
    //      * but PWR uses SYSCLK according to the clock tree.
    //      */
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_SYSCLK,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_CAN1] = {
    //     .name        = "can1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SPI3] = {
    //     .name        = "spi3",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SPI2] = {
    //     .name        = "spi2",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_WWDG] = {
    //     .name        = "wwdg",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_LCD] = {
    //     .name        = "lcd",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_LCD_AND_RTC_COMMON,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM7] = {
    //     .name        = "tim7",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM6] = {
    //     .name        = "tim6",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM5] = {
    //     .name        = "tim5",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM4] = {
    //     .name        = "tim4",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM3] = {
    //     .name        = "tim3",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM2] = {
    //     .name        = "tim2",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK1,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM17] = {
    //     .name        = "tim17",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM16] = {
    //     .name        = "tim16",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM15] = {
    //     .name        = "tim15",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM8] = {
    //     .name        = "tim8",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SPI1] = {
    //     .name        = "spi1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_TIM1] = {
    //     .name        = "tim1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SDMMC1] = {
    //     .name        = "sdmmc1",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_FW] = {
    //     .name        = "fw",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_SYSCFG] = {
    //     .name        = "syscfg",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_PCLK2,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    // [RCC_CLOCK_MUX_RTC] = {
    //     .name        = "rtc",
    //     .src_mapping = {
    //         RCC_CLOCK_MUX_SRC_LCD_AND_RTC_COMMON,
    //     },
    //     FILL_DEFAULT_INIT_DISABLED,
    // },
    [RCC_CLOCK_MUX_CORTEX_FCLK] = {
        .name        = "cortex-fclk",
        .src_mapping = {
            RCC_CLOCK_MUX_SRC_HCLK,
        },
        FILL_DEFAULT_INIT_ENABLED,
    },
};

static inline void set_clock_mux_init_info(RccClockMuxState *mux,
                                           RccClockMux id)
{
    mux->id         = id;
    mux->multiplier = CLOCK_MUX_INIT_INFO[id].multiplier;
    mux->divider    = CLOCK_MUX_INIT_INFO[id].divider;
    mux->enabled    = CLOCK_MUX_INIT_INFO[id].enabled;
    /*
     * Every peripheral has the first source of their source list as
     * as their default source.
     */
    mux->src = 0;
}

#endif /* HW_STM32F429_RCC_INTERNALS_H */
