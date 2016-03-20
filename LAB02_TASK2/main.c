/**
 * Name			: main.c
 * Author		: Jørgen Ryther Hoem
 * Lab			: Lab 2, Task 2. (ET014G)
 * Description  : Main file, writing from SDRAM to SD Card using DMA
 */
#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "gpio.h"
#include "spi.h"
#include "pm.h"
#include "conf_sd_mmc_spi.h"
#include "sd_mmc_spi.h"
#include "usart.h"
#include "print_funcs.h"
#include "pdca.h"
#include "pdca_write_sector.h"
#include "intc.h"
#include "sdramc.h"
#include "delay.h"
#include "cycle_counter.h"



/*****  DEFINITIONS  **************************************************/

// Clock frequencies (Hz)
#define CPU_HZ						60000000
#define PBA_HZ						30000000

// PDCA channels for receiving and transmitting (ch. 0 and 1)
#define AVR32_PDCA_CHANNEL_SPI_RX	0
#define AVR32_PDCA_CHANNEL_SPI_TX	1



/*****  VARIABLES  ****************************************************/

// Dummy data for PDCA transmission
static const char dummy_data[MMC_SECTOR_SIZE] = {
	#define DUMMY_8_BYTES(line, unsed)	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	MREPEAT(64, DUMMY_8_BYTES, ~)
	#undef DUMMY_8_BYTES
};

// Receive buffer for PDCA transmission
volatile char ram_buffer[1024];

// SDRAM pointer
volatile uint32_t *sdram = SDRAM;



/*****  METHODS  ******************************************************/

/* 
 * Initializes SD/MMC resources: GPIO, SPI and SD/MMC.
 *
 */
static void sd_mmc_resources_init(void)
{
	// GPIO pins used for SD/MMC interface
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
	{
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = SD_MMC_SPI_NPCS,
		.baudrate     = SD_MMC_SPI_MASTER_SPEED,  // Defined in conf_sd_mmc_spi.h.
		.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP, sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(spiOptions, PBA_HZ);
}


/* 
 * Initializes PDCA.
 *
 */
static void local_pdca_init(void)
{
	// PDCA channel option for RX
	pdca_channel_options_t pdca_options_SPI_RX = {
		.addr = ram_buffer,
		.size = 512,                              // transfer counter
		.r_addr = NULL,                           // next memory address
		.r_size = 0,                              // next transfer counter
		.pid = AVR32_PDCA_PID_SPI1_RX,			  // select peripheral ID
		.transfer_size = PDCA_TRANSFER_SIZE_BYTE  // size of the transfer
	};

	// PDCA channel option for TX
	pdca_channel_options_t pdca_options_SPI_TX = { 
		.addr = (void *)&dummy_data,
		.size = 512,
		.r_addr = NULL,
		.r_size = 0,
		.pid = AVR32_PDCA_PID_SPI1_TX,
		.transfer_size = PDCA_TRANSFER_SIZE_WORD
	};

	// Init PDCA transmission channels
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_TX, &pdca_options_SPI_TX);
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_RX, &pdca_options_SPI_RX);
}



/* 
 * Main application function.
 *
 */
int main(void)
{
	// Variables used in this application
	volatile uint32_t i, j, k, p;
	volatile uint32_t sdram_size, num_sd_sectors, progress_inc, num_errors;
	volatile uint32_t cy_count, cy_count_ms;
	


	/*** Application initialization *******/


	// Configure clocks
	pm_freq_param_t clock_conf = {
		.cpu_f = CPU_HZ,
		.pba_f = PBA_HZ,
		.osc0_f = FOSC0,
		.osc0_startup = OSC0_STARTUP
	};
	pm_configure_clocks(&clock_conf);
	
	// Initialize debug RS232 with PBA clock
	init_dbg_rs232(PBA_HZ);
	print_dbg("\r\n- LAB02 Task 2 - SDRAM to SD/MMC Card using DMA");
	print_dbg("\r\n--------------------------- J.R.Hoem (ET014G) -\r\n");

	// Display clock configuration
	print_dbg("\r\nCPU/PBA: ");
	print_dbg_ulong(CPU_HZ/1e6);
	print_dbg_char('/');
	print_dbg_ulong(PBA_HZ/1e6);
	print_dbg(" MHz\r\n");

	// Initialize SDRAM
	sdramc_init(CPU_HZ);
	print_dbg("\r\nSDRAM initialized (");
	print_dbg_ulong(SDRAM_SIZE >> 20);
	print_dbg(" MB)\r\n");

	// Configure and initialize SD/MMC driver
	sd_mmc_resources_init();

	// Check if card is found
	if (!sd_mmc_spi_mem_check())
	{
		print_dbg("\r\nInsert SD/MMC Card\r\n");
		while (!sd_mmc_spi_mem_check());
	}
	print_dbg("SDCard detected");

	// Display card capacity
	sd_mmc_spi_get_capacity();
	print_dbg(" (");
	print_dbg_ulong(capacity >> 20);
	print_dbg(" MB)\r\n");
	
	// Configure and initialize PDCA driver
	local_pdca_init();
	
	// Setting EBI slave to have a fixed default master
	AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EBI].defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_FIXED_DEFAULT;

	// Set EBI slave to have PDCA as master
	AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EBI].fixed_defmstr = AVR32_HMATRIX_MASTER_PDCA;
	
	
	
	/*** Application routines *************/
	

	/** SDRAM **/

	// Fill SDRAM with bytes ranging 0x00 to 0xFF
	sdram_size = SDRAM_SIZE >> 2;
	progress_inc = (uint32_t)((sdram_size + 50) / 100);
	
	print_dbg("\r\nFilling SDRAM with test data:\r\n  ");
	cy_count = Get_sys_count();
	for (i=0,j=0,p=0; i < sdram_size; i++)
	{
		// Write data to sdram
		sdram[i] = j++;
		if (j > 255) j = 0;
		
		// Display progress
		if (i == p * progress_inc)
		{
			print_dbg_ulong(p);
			print_dbg("%, ");
			p += 10;			
		}
	}
	cy_count_ms = cpu_cy_2_ms((Get_sys_count() - cy_count), CPU_HZ);
	print_dbg("Done!\r\n");	
	
	// Display time elapsed
	print_dbg("  Time elapsed: ");
	print_dbg_ulong(cy_count_ms);
	print_dbg(" ms\r\n");

	// Display first 20 bytes of SDRAM	
	print_dbg("\r\nControl reading SDRAM and showing first 20 bytes:\r\n ");
	for (i=0; i < 20; i++)
	{
		print_dbg(" ");
		print_dbg_char_hex((uint8_t)(*(sdram + i)));
	}
	print_dbg("\r\n  ");
	
	// Check SDRAM for errors
	num_errors = 0;
	for (i=0,j=0,p=0; i < sdram_size; i++)
	{
		if ((uint8_t)sdram[i] != (uint8_t)j++) num_errors++;
		if (j > 255) j = 0;
		
		// Display progress
		if (i == p * progress_inc)
		{
			print_dbg_ulong(p);
			print_dbg("%, ");
			p += 10;
		}
	}
	print_dbg("Done!\r\n  Errors found: ");
	print_dbg_ulong(num_errors);
	print_dbg("\r\n");


	/** PDCA **/
	
	// Write from SDRAM to SD Card
	num_sd_sectors = (uint32_t)(sdram_size / MMC_SECTOR_SIZE);
	progress_inc = (uint32_t)(num_sd_sectors / 100);
	p = 0;
	
	print_dbg("\r\nWriting SDRAM to SD Card (");
	print_dbg_ulong(num_sd_sectors);
	print_dbg(" sectors):\r\n  ");
	cy_count = Get_sys_count();
	for (i=1; i <= num_sd_sectors; i++)
	{
		// Open sector number i
		if (sd_mmc_spi_write_open_PDCA(i))
		{
			// Load transmission channel with sdram
			pdca_load_channel(AVR32_PDCA_CHANNEL_SPI_TX, sdram, 512);
			
			// Enable PDCA TX channel
			pdca_enable(AVR32_PDCA_CHANNEL_SPI_TX);
			
			// Wait for PDCA to finish sending 
			// by checking the channel transfer status with the correct interrupt flag
			while (!(pdca_get_transfer_status(AVR32_PDCA_CHANNEL_SPI_TX) & AVR32_PDCA_ISR_TRC_MASK));
			
			// Disable PDCA TX channel
			pdca_disable(AVR32_PDCA_CHANNEL_SPI_TX);
			
			// Close PCDA write session
			sd_mmc_spi_write_close_PDCA();
			delay_us(10); // give the PDCA time to close
			
			// Increase sdram pointer with one sector size
			sdram += MMC_SECTOR_SIZE;
		}
		else
		{
			print_dbg("\r\nERROR: Unable to open memory for writing\r\n");
		}		
	
		// Display progress
		if ((i-1) == p * progress_inc)
		{
			print_dbg_ulong(p);
			print_dbg("%, ");
			p += 10;
		}
	}	
	cy_count = Get_sys_count() - cy_count;
	print_dbg("Done!\r\n");	
	
	// Display clock cycles
	print_dbg("  Clock cycles: ");
	print_dbg_ulong(cy_count);
	print_dbg(" cy\r\n");
	

	// Display 20 bytes from the first 5 sectors
	num_errors = 0;

	print_dbg("\r\nControl reading SD Card sectors:\r\n  ");
	for (i=1,p=0; i <= num_sd_sectors; i++)
	{
		// Configure the PDCA channel for both RX and TX
		pdca_load_channel(AVR32_PDCA_CHANNEL_SPI_RX, &ram_buffer, 512);
		pdca_load_channel(AVR32_PDCA_CHANNEL_SPI_TX, (void *)&dummy_data, 512); // dummy to activate the clock

		// Open sector number i
		if (sd_mmc_spi_read_open_PDCA(i))
		{
			// Write a first dummy data to synchronize transfer
			spi_write(SD_MMC_SPI,0xFF); 
						
			// Enable PDCA RX and TX channel
			pdca_enable(AVR32_PDCA_CHANNEL_SPI_RX);
			pdca_enable(AVR32_PDCA_CHANNEL_SPI_TX);
			
			// Wait for PDCA to finish receiving
			while (!(pdca_get_transfer_status(AVR32_PDCA_CHANNEL_SPI_RX) & AVR32_PDCA_ISR_TRC_MASK));
			
			// Close PDCA read session
			sd_mmc_spi_read_close_PDCA();
			delay_us(10); // give the PDCA time to close
			
			// Disable PDCA RX and TX channel
			pdca_disable(AVR32_PDCA_CHANNEL_SPI_TX);
			pdca_disable(AVR32_PDCA_CHANNEL_SPI_RX);

			// Control read sector
			for (j=0,k=0; j < MMC_SECTOR_SIZE; j++)
			{
				if ((uint8_t)k++ != (uint8_t)(*(ram_buffer + j))) num_errors++;
				if (k > 255) k = 0;
			}
		}
		else
		{
			print_dbg("\r\nERROR: Unable to open memory for reading\r\n");
		}
		
		// Display progress
		if ((i-1) == p * progress_inc)
		{
			print_dbg_ulong(p);
			print_dbg("%, ");
			p += 10;
		}

	}
	print_dbg("Done!\r\n  Errors found: ");
	print_dbg_ulong(num_errors);
	print_dbg("\r\n");


	// End of task
	print_dbg("\r\n---------------\r\n- End of Task 2\r\n");

	// Into infinity
	while (1);
}
