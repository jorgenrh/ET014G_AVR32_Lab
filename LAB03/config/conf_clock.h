

#ifndef CONF_CLOCK_H_INCLUDED
#define CONF_CLOCK_H_INCLUDED

// Supported frequencies:
// Fosc0 mul div PLL div2_en cpu_f pba_f   Comment
//  12   15   1  192     1     12    12
//  12    9   3   40     1     20    20    PLL out of spec
//  12   15   1  192     1     24    12
//  12    9   1  120     1     30    15
//  12    9   3   40     0     40    20    PLL out of spec
//  12   15   1  192     1     48    12
//  12   15   1  192     1     48    24
//  12    8   1  108     1     54    27
//  12    9   1  120     1     60    15
//  12    9   1  120     1     60    30
//  12   10   1  132     1     66    16.5

#define CONFIG_SYSCLK_SOURCE			SYSCLK_SRC_PLL0

/* Fbus = Fsys / (2 ^ BUS_div) */
#define CONFIG_SYSCLK_CPU_DIV			0 /* Fcpu = Fsys/(2 ^ CPU_div) */
#define CONFIG_SYSCLK_PBA_DIV			1 /* Fpba = Fsys/(2 ^ PBA_div) = 24 MHz */
#define CONFIG_SYSCLK_PBB_DIV			0 /* Fpbb = Fsys/(2 ^ PBB_div) */

#define CONFIG_PLL0_SOURCE				PLL_SRC_OSC0

/* Fpll0 = (Fclk * PLL_mul) / PLL_div */
/* (12e6 * 4)/1 = 48e6 */ 
#define CONFIG_PLL0_MUL					5
#define CONFIG_PLL0_DIV					1

#endif /* CONF_CLOCK_H_INCLUDED */
