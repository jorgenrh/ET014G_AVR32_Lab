/**
 * Name         : main.c
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : 
 *
 *   This application demonstrates how to write ADC values to
 *   a logfile on a SD/MMC card periodically using interrupts.
 *   It provides a simplistic CLI over USB to set the logfile, 
 *   format the disk and start/stop logging.
 *
 *   SD/MMC (SPI), ADC, TC, FAT, USB (STDIO), Delay routines
 *
 *   Atmel Studio 7.0 / ASF 3.30.1
 *
 */
#include <asf.h>
#include <inttypes.h>
#include "app.h"
#include "log.h"
#include "cli.h"
#include "conf_app.h"



/*****  VARIABLES  ****************************************************/

// App modes
typedef enum {
	APP_MODE_WAITING = 0,
	APP_MODE_LOGGING,
} app_mode_t;

// Current app mode
volatile app_mode_t app_mode = APP_MODE_WAITING;

// Count log entries
volatile uint64_t app_log_count = 0;

// Filename pointer
volatile char *app_logfile;

// Read and save ADC flag
volatile bool update_adc_to_log = false;



/*****  FUNCTIONS  ****************************************************/

/*
 * Update ADC value to logfile
 *
 *  This function update the logfile with a new ADC value if
 *  the update flag is set to true.
 */
static void app_update_adc_task(void)
{
	// Check if update flag is true
	if (update_adc_to_log)
	{
		// Begin ADC reading
		adc_start(&AVR32_ADC);

		// Read new value
		uint16_t adc_value = adc_get_value(&AVR32_ADC, APP_ADC_POT_CHANNEL);
	
		// Get the cycle count for reading
		uint32_t cycle_count = (uint32_t)Get_sys_count();

		// Log ADC value
		log_write_adc(cycle_count, adc_value);
	
		// Count entries
		app_log_count++;

		// Toggle LED6 to indicate logging
		if (!(app_log_count % 50)) LED_Toggle(LED6);
		
		// Reset ADC update flag
		update_adc_to_log = false;
	}
}


/*
 * ADC/Pot reading interrupt handler
 *
 *  This interrupt handler is triggered by the timer/counter.
 *
 *  There are usually two common ways to execute routines
 *  from an interrupt. 
 *    1) run the routines directly from the interrupt
 *    2) run the routines indirectly in the main while loop
 *
 *  The latter one is used here since one usually wants to
 *  keep the interrupt cycles to a minimum. And ADC reading 
 *  and log writing can in theory cause unexpected delays. 
 *  But both options have been tested without any
 *  noticeable performance differences.
 */
__attribute__((__interrupt__))
static void tc_read_pot_irq(void)
{
	// Clear the interrupt flag.
	tc_read_sr(APP_TC, APP_TC_CHANNEL);
	
	// Check if logging is on
	if (app_mode == APP_MODE_LOGGING)
	{
		// 2) Run ADC update from main while loop
		update_adc_to_log = true;
		
		// 1) Run the ADC update directly from interrupt
		//app_update_adc_task();	
	}
	
	// Toggle LED0 as a timer interval indicator
	LED_Toggle(LED0);
}


/*
 * Interrupt configuration
 *
 *  This function initializes the interrupts handlers and
 *  vectors for USB and Timer/Counter driver
 */
static void app_interrupt_init(void)
{
	// Disable the interrupts
	cpu_irq_disable();

	// Initialize interrupt vector table support for USB CDC
	irq_initialize_vectors();

	// Initialize interrupt vectors.
	INTC_init_interrupts();
	
	// Register the Timer/Counter (pot reading) int handler
	INTC_register_interrupt(&tc_read_pot_irq, APP_TC_IRQ, APP_TC_IRQ_PRIORITY);

	// Enable the interrupts
	cpu_irq_enable();
}


/*
 * Main app function
 *
 */
int main (void)
{
	// Init system clock
	sysclk_init();

	// Enable the clock to the Timer/Counter driver
	sysclk_enable_peripheral_clock(APP_TC);

	// Initiate default board configuration
	board_init();
	
	// Turn LEDs off
	LED_Off(LED0); // Timer/Counter indicator
	LED_Off(LED6); // Logging on/off indicator
	
	// Initiate interrupts
	app_interrupt_init();
	
	// Init USB
	stdio_usb_init();
	stdio_usb_enable();

	// Initiate SD/MMC SPI, ADC and Timer/Counter drivers
	app_init();
	
	// In some rare cases the terminal wont display the
	// first lines of text, this delay helps to prevent that
	delay_ms(4000);

	// Display welcome text
	printf(APP_WELCOME_TEXT);
	
	// Display clock settings
	printf("CPU/PBA: %d/%d MHz\r\n\r\n", (uint8_t)(sysclk_get_cpu_hz()/1e6), (uint8_t)(sysclk_get_pba_hz()/1e6));
	
	
	// Check if card is inserted
	if (!sd_mmc_spi_mem_check())
	{		
		printf("Card not detected\r\nInsert SD Card to continue\r\n");
		while (!sd_mmc_spi_mem_check()) { /* wait for card */ }
	}
	sd_mmc_spi_get_capacity(); // Read Card capacity
	printf("Card detected (%u MB)", (uint16_t)(capacity >> 20));
	
	
	// Initiate logging
	if (log_init(APP_SD_MMC_SLOT))
	{
		// Set default logfile
		app_logfile = APP_LOG_FILENAME;
		
		// Select/create logfile
		log_set_file((char*)app_logfile);
	}
	else printf("Could not initiate logging due to FAT issues\r\n");
	
	
	// Displays CLI commands
	printf("\r\n" CLI_HELP_TEXT ">");
		
	
	// Main loop waiting for commands
	while(true)
	{
		// Writes value to log if flag is set to true
		// See interrupt comment for more
		app_update_adc_task();
		
		// CLI task, reads user input
		cli_task();
		
		switch (cli_get_command())
		{
			// Command: start
			case CLI_CMD_START:
			if (app_mode == APP_MODE_LOGGING) printf("Logging is already running");
			else if (log_start())
			{
				app_mode = APP_MODE_LOGGING;
				printf("Logging started (file: %s)\r\n", app_logfile);
			}
			else printf("Error: Could not open logfile\r\n");
			printf("\r\n>");
			break;

			// Command: stop
			case CLI_CMD_STOP:
			if (app_mode == APP_MODE_WAITING) printf("Logging is not running\r\n");
			else
			{
				app_mode = APP_MODE_WAITING;
				LED_Off(LED6);
				printf("Logging stopped (entries: %" PRIu64 ")\r\n", app_log_count);
				log_stop();
			}
			printf("\r\n>");
			break;

			// Command: status
			case CLI_CMD_STATUS:
			printf("Logging:    %s\r\n", (app_mode == APP_MODE_LOGGING ? "ON" : "OFF"));
			printf("Filename:   %s\r\n", app_logfile);
			printf("Log count:  %" PRIu64 "\r\n", app_log_count);
			printf("\r\n>");
			break;
			
			// Command: format
			case CLI_CMD_FORMAT:
			if (app_mode != APP_MODE_WAITING) break;
			printf("Formatting drive ... ");
			if (format_drive())
			{
				printf("OK\r\n");
				mount_drive();
				app_log_count = 0;
			}
			else printf("ERROR\r\n");
			printf("\r\n>");
			break;
			
			// Command: file <filename>
			case CLI_CMD_FILE:
			if (app_mode != APP_MODE_WAITING) break;
			app_logfile = cli_get_argument();
			log_set_file((char*)app_logfile);
			app_log_count = 0;
			printf("Logfile set to: \"%s\"\r\n", app_logfile);
			printf("\r\n>");
			break;
			
			// Command: help
			case CLI_CMD_HELP:
			printf(CLI_HELP_TEXT ">");
			break;
			
			// Unknown command
			case CLI_CMD_UNKNOWN:
			printf("Invalid command, type help for more.\r\n");
			printf("\r\n>");
			break;
			
			// No command
			case CLI_CMD_NONE:
			default:
			// Catch all, do nothing
			break;
		}
		
	}

}
