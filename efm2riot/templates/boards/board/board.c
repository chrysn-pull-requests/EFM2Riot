/*
 * Copyright (C) 2015-2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_{{ board }}
 * @{
 *
 * @file
 * @brief       Board specific implementations {{ board_display_name }} board
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Bas Stottelaar <basstottelaar@gmail.com>
 *
 * @}
 */

#include "board.h"
#include "cpu.h"

#include "periph/gpio.h"

{% strip 2, true %}
    {% if architecture not in ["m0", "m0plus"] %}
        #include "em_dbg.h"
        #include "em_gpio.h"
    {% endif %}
{% endstrip %}

void board_init(void)
{
    /* initialize the CPU */
    cpu_init();

    /* enable access to the evaluation board controller chip. Without this, the
     * board controller does not forward the UART output to the USB port */
#if BC_ENABLED
    gpio_init(BC_PIN, GPIO_OUT);
    gpio_set(BC_PIN);
#endif

    {% strip 2, true %}
        {% if architecture not in ["m0", "m0plus"] %}
            /* enable core debug output AEM */
        #if AEM_ENABLED
            if (DBG_Connected()) {
                /* enable GPIO clock for configuring SWO pins */
                CMU_ClockEnable(cmuClock_GPIO, true);

                /* enable debug peripheral via SWO */
                {% strip 2 %}
                    {% if cpu_platform == 1 %}
                        DBG_SWOEnable(GPIO_ROUTE_SWLOCATION_LOC0);
                    {% else %}
                        DBG_SWOEnable(GPIO_ROUTELOC0_SWVLOC_LOC0);
                    {% endif %}
                {% endstrip %}

                /* enable trace in core debug */
                CoreDebug->DHCSR |= CoreDebug_DHCSR_C_DEBUGEN_Msk;
                CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

                /* enable PC and IRQ sampling output */
                DWT->CTRL = 0x400113FF;

                /* set TPIU prescaler to 16. */
                TPI->ACPR = 15;

                /* set protocol to NRZ */
                TPI->SPPR = 2;

                /* disable continuous formatting */
                TPI->FFCR = 0x100;

                /* unlock ITM and output data */
                ITM->LAR = 0xC5ACCE55;
                ITM->TCR = 0x10009;
            }
        #endif
        {% endif %}
    {% endstrip %}

    /* initialize the boards LEDs */
    gpio_init(LED0_PIN, GPIO_OUT);
    gpio_init(LED1_PIN, GPIO_OUT);
}
