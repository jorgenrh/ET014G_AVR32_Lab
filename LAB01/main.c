/****************************************************************
 Name          : main.c
 Author        : Jørgen Ryther Hoem
 Copyright     : No
 Description   : ET014G Lab 1 using AVR32 Studio 2.6 and EVK1100
 ****************************************************************/

// Include Files 
#include "board.h"
#include "gpio.h"
#include "pm.h"
#include "compiler.h"
#include "dip204.h"
#include "delay.h"
#include "spi.h"
#include <avr32/io.h>


void init_LCD_SPI(void)
{
	static const gpio_map_t DIP204_SPI_GPIO_MAP = {
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// add the spi options driver structure for the LCD DIP204
	spi_options_t spiOptions = {
		.reg          = DIP204_SPI_NPCS,
		.baudrate     = 1000000,
		.bits         = 8,
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Initialize SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));
	spi_initMaster(DIP204_SPI, &spiOptions);
	spi_selectionMode(DIP204_SPI, 0, 0, 0);
	spi_enable(DIP204_SPI);
	spi_setupChipReg(DIP204_SPI, &spiOptions, FOSC0);

	// Initialize delay driver
	delay_init(FOSC0);
}


int main(void) {

	// Set clock to external oscillator (12 MHz)
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);


	// Initialize LCD
	init_LCD_SPI();
	dip204_init(backlight_PWM, TRUE);
	dip204_hide_cursor();
	dip204_set_cursor_position(1, 1);
	dip204_write_string("Math Operation Test:");


	// Calculate clock cycle difference
	// for int vs float multiplication
	int start_cy, diff_cy_int, diff_cy_float, diff_cy_tot, diff_us;

	U32 x = 12345678;
	U32 y = 87654321;
	U64 z;

	float a = 1234.5678;
	float b = 8765.4321;
	float c;

	// int cycle calculation
	start_cy = Get_sys_count();
	z = x*y;
	diff_cy_int = Get_sys_count()-start_cy;

	// float cycle calculation
	start_cy = Get_sys_count();
	c = a*b;
	diff_cy_float = Get_sys_count()-start_cy;

	diff_us = cpu_cy_2_us(diff_cy_int, FOSC0);
	dip204_set_cursor_position(1, 2);
	dip204_printf_string("Int:   %lu us (%d cy)", diff_us, diff_cy_int);

	diff_us = cpu_cy_2_us(diff_cy_float, FOSC0);
	dip204_set_cursor_position(1, 3);
	dip204_printf_string("Float: %lu us (%d cy)", diff_us, diff_cy_float);

	// total cycles
	diff_cy_tot = diff_cy_int+diff_cy_float;
	diff_us = cpu_cy_2_us(diff_cy_tot, FOSC0);
	dip204_set_cursor_position(1, 4);
	dip204_printf_string("Total: %lu us (%d cy)", diff_us, diff_cy_tot);


	while (true)
	{
		// Set LED6 on with PB0
		if (!gpio_get_pin_value(GPIO_PUSH_BUTTON_0))
			LED_On(LED6);
		else
			LED_Off(LED6);
	}

	return 0;
}
