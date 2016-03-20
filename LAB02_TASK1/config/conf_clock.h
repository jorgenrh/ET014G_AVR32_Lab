/**
 * Name			: conf_clock.h
 * Author		: Jørgen Ryther Hoem
 * Lab			: Lab 2, Task 1. (ET014G)
 * Description  : Clock configuration
 */

#ifndef CONF_CLOCK_H_INCLUDED
#define CONF_CLOCK_H_INCLUDED

// ===== System Clock Source Options
#define CONFIG_SYSCLK_SOURCE          SYSCLK_SRC_PLL0 // PPL for OSC0

// ===== PLL0 Options
#define CONFIG_PLL0_SOURCE            PLL_SRC_OSC0 // 12 MHz external OSC 0
// Fpll = 48 MHz
#define CONFIG_PLL0_MUL               8 /* Fpll = (Fclk * PLL_mul) / PLL_div */
#define CONFIG_PLL0_DIV               2 /* Fpll = (Fclk * PLL_mul) / PLL_div */

// ===== USB Clock Source Options
#define CONFIG_USBCLK_SOURCE          USBCLK_SRC_PLL0
// Fusb = 48 MHz (High-Speed USB)
#define CONFIG_USBCLK_DIV             0 /* Fusb = Fsys/(2 ^ USB_div) */

#endif /* CONF_CLOCK_H_INCLUDED */
