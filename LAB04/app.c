/**
 * Name         : app.c
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : ASF driver/services initialization used in Lab 4
 */
#include <asf.h>
#include "app.h"
#include "conf_sd_mmc_spi.h"
#include "conf_app.h"


/*****  PRIVATE PROTOTYPES  *******************************************/

// Private functions
void app_init_sdmmc_spi(void);
void app_init_adc(void);
void app_init_tc(void);


/*****  FUNCTIONS  ****************************************************/

/*
 * Main initializing
 *
 *  This function initializes the required drivers
 */
void app_init(void)
{
	// Init SD/MMC SPI driver
	app_init_sdmmc_spi();
	
	// Init ADC driver
	app_init_adc();

	// Init Timer/Counter driver
	app_init_tc();
}


/*****  PRIVATE FUNCTIONS  ********************************************/

/*
 * SD/MMC via SPI initializing
 *
 *  This initializes the SD/MMC driver using SPI
 */
void app_init_sdmmc_spi(void)
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
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
	sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(spiOptions, sysclk_get_pba_hz());
}


/*
 * ADC initializing
 *
 *  This initializes the ADC driver with configuration from conf_app.h
 */
void app_init_adc(void)
{
	// ADC GPIO pin config
	const gpio_map_t ADC_GPIO_MAP = {
		{ APP_ADC_POT_PIN, APP_ADC_POT_FUNCTION }
	};
	
	// Assign and enable GPIO pins to the ADC function
	gpio_enable_module(ADC_GPIO_MAP, sizeof(ADC_GPIO_MAP) / sizeof(ADC_GPIO_MAP[0]));
	
	// Configure the ADC peripheral module
	AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
	adc_configure(&AVR32_ADC);
	
	// Enable the ADC channel
	adc_enable(&AVR32_ADC, APP_ADC_POT_CHANNEL);
}


/*
 * Timer/Counter initializer
 *
 *  Initializes the Timer/Counter driver for reading ADC data.
 *  It uses the TC waveform type with clock source 5 (128 divider).
 *  Configured with an update interval of 50 Hz to correspond with the ADC driver
 */
void app_init_tc(void)
{
	// Options for waveform generation.
	static const tc_waveform_opt_t tc_waveform_config = {
		.channel  = APP_TC_CHANNEL,			// Channel selection.

		.bswtrg   = TC_EVT_EFFECT_NOOP,		// Software trigger effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,		// External event effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,		// RC compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,		// RB compare effect on TIOB.

		.aswtrg   = TC_EVT_EFFECT_NOOP,		// Software trigger effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,		// External event effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,		// RC compare effect on TIOA.
		.acpa     = TC_EVT_EFFECT_NOOP,		// RA compare effect on TIOA. (none, set and clear)

		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER, // Waveform selection: Up mode with automatic trigger(reset) on RC compare.
		.enetrg   = false,					// External event trigger enable.
		.eevt     = 0,						// External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,			// External event edge selection.
		.cpcdis   = false,					// Counter disable when RC compare.
		.cpcstop  = false,					// Counter clock stopped with RC compare.
		.burst    = false,					// Burst signal selection.
		.clki     = false,					// Clock inversion.
		.tcclks   = TC_CLOCK_SOURCE_TC5		// Internal source clock 5, connected to fPBA / 128.
	};
	// Initialize the timer/counter.
	tc_init_waveform(APP_TC, &tc_waveform_config);
	
	
	// Options for enabling TC interrupts
	static const tc_interrupt_t tc_interrupt_config = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};

	// We want 50 Hz: (1 / (fPBA / 128)) * RC = 0.02 s, hence RC = (fPBA / 128) / 50
	tc_write_rc(APP_TC, APP_TC_CHANNEL, ((sysclk_get_pba_hz() / 128) / APP_READ_ADC_FREQ));

	// configure the timer interrupt
	tc_configure_interrupts(APP_TC, APP_TC_CHANNEL, &tc_interrupt_config);

	// Start the timer/counter.
	tc_start(APP_TC, APP_TC_CHANNEL);
}



