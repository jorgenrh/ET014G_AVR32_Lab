/*
 * conf_clock.h
 *
 * Created: 09.03.2016 13.20.38
 *  Author: jrh
 */ 
#ifndef CONF_CLOCK_H_INCLUDED
#define CONF_CLOCK_H_INCLUDED

// CPU (60 MHz)
#define CONFIG_SYSCLK_SOURCE          SYSCLK_SRC_PLL0

/* Fbus = Fsys / (2 ^ BUS_div) */
#define CONFIG_SYSCLK_CPU_DIV         0
#define CONFIG_SYSCLK_PBA_DIV         1
#define CONFIG_SYSCLK_PBB_DIV         0

// PPL 0 (60 MHz used by CPU)
#define CONFIG_PLL0_SOURCE            PLL_SRC_OSC0

/* Fpll0 = (Fclk * PLL_mul) / PLL_div */
#define CONFIG_PLL0_MUL               5
#define CONFIG_PLL0_DIV               1

// PPL 1 (48 MHz used by USB)
#define CONFIG_PLL1_SOURCE            PLL_SRC_OSC0

/* Fpll1 = (Fclk * PLL_mul) / PLL_div */
#define CONFIG_PLL1_MUL               4
#define CONFIG_PLL1_DIV               1

// USB (48 MHz)
#define CONFIG_USBCLK_SOURCE          USBCLK_SRC_PLL1

/* Fusb = Fsys / USB_div */
#define CONFIG_USBCLK_DIV             1


#endif /* CONF_CLOCK_H_INCLUDED */
