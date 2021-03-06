/*
 * Copyright (C) 2016 Bas Stottelaar <basstottelaar@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_efm32_common
 * @{
 *
 * @file
 * @brief       Low-level PWM peripheral driver implementation
 *
 * @author      Bas Stottelaar <basstottelaar@gmail.com>
 */

#include "cpu.h"

#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/pwm.h"

#include "em_cmu.h"
#include "em_timer.h"
#include "em_timer_utils.h"
#include "em_common_utils.h"

/* guard file in case no PWM device is defined */
#if PWM_NUMOF

uint32_t pwm_init(pwm_t dev, pwm_mode_t mode, uint32_t freq, uint16_t res)
{
    /* check if device is valid */
    if (dev >= PWM_NUMOF) {
        return -1;
    }

    /* enable clocks */
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(pwm_config[dev].cmu, true);

    /* calculate the prescaler by determining the best prescaler */
    uint32_t freq_timer = CMU_ClockFreqGet(pwm_config[dev].cmu);
    TIMER_Prescale_TypeDef prescaler = TIMER_PrescalerCalc(freq * res,
                                                           freq_timer);

    if (prescaler > timerPrescale1024) {
        return -2;
    }

    /* reset and initialize peripheral */
    EFM32_CREATE_INIT(init, TIMER_Init_TypeDef, TIMER_INIT_DEFAULT,
        .conf.enable = false,
        .conf.prescale = prescaler
    );

    TIMER_Reset(pwm_config[dev].dev);
    TIMER_Init(pwm_config[dev].dev, &init.conf);
    TIMER_TopSet(pwm_config[dev].dev, res);

    /* initialize channels */
    EFM32_CREATE_INIT(init_channel, TIMER_InitCC_TypeDef, TIMER_INITCC_DEFAULT,
        .conf.mode = timerCCModePWM
    );

    for (int i = 0; i < pwm_config[dev].channels; i++) {
        pwm_chan_conf_t channel = pwm_config[dev].channel[i];

        /* configure the pin */
        gpio_init(channel.pin, GPIO_OUT);

        /* configure pin function */
#ifdef _SILICON_LABS_32B_PLATFORM_1
        pwm_config[dev].dev->ROUTE |= (channel.loc |
                                       TIMER_Channel2Route(channel.index));
#else
        pwm_config[dev].dev->ROUTELOC0 |= channel.loc;
        pwm_config[dev].dev->ROUTEPEN |= TIMER_Channel2Route(channel.index);
#endif

        /* setup channel */
        TIMER_InitCC(pwm_config[dev].dev, channel.index, &init_channel.conf);
    }

    /* enable peripheral */
    TIMER_Enable(pwm_config[dev].dev, true);

    return freq_timer / TIMER_Prescaler2Div(prescaler) / res;
}

uint8_t pwm_channels(pwm_t dev)
{
    return pwm_config[dev].channels;
}

void pwm_set(pwm_t dev, uint8_t channel, uint16_t value)
{
    TIMER_CompareBufSet(pwm_config[dev].dev,
                        pwm_config[dev].channel[channel].index,
                        value);
}

void pwm_start(pwm_t dev)
{
    TIMER_Enable(pwm_config[dev].dev, true);
}

void pwm_stop(pwm_t dev)
{
    TIMER_Enable(pwm_config[dev].dev, false);
}

void pwm_poweron(pwm_t dev)
{
    CMU_ClockEnable(pwm_config[dev].cmu, true);
}

void pwm_poweroff(pwm_t dev)
{
    CMU_ClockEnable(pwm_config[dev].cmu, false);
}

#endif /* PWM_NUMOF */
