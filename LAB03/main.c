/**
 * Name         : main.c
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 3 (ET014G)
 * Description  : Adjusting PWM with potentiometer, reading ADC using TC
 */
#include <asf.h>
#include "conf_app.h"



/*****  VARIABLES  ****************************************************/


// "Raw" PWM duty cycle and period paramters
volatile uint32_t current_duty		= 7500;
volatile uint32_t current_period	= 15000;


// PWM duty cycle and frequency
volatile uint8_t  pwm_dutycycle		= 50;		// Percentage
volatile uint32_t pwm_frequency		= 1000;		// Hertz


// PWM adjusting options enum
typedef enum {
	APP_PWM_OPT_FREQUENCY = 0,
	APP_PWM_OPT_DUTYCYCLE
} app_pwm_opt_t;

// Selected PWM option
volatile app_pwm_opt_t app_pwm_opt = APP_PWM_OPT_FREQUENCY;


// Potentiometer value saved here between readings
volatile uint16_t adc_pot_value = 0;


// ADC update flag
volatile bool update_adc_value = false;



/*****  PROTOTYPES  ***************************************************/


// Prototype for PWM update
static void app_pwm_update(app_pwm_opt_t pwm_opt, uint32_t value);



/*****  FUNCTIONS  ****************************************************/


//////// INTERRUPT /////////

/*
 * Push button interrupt
 *
 *  This interrupt handler toggles current PWM option
 *  and triggers by push button 0 (PB0) rising edge
 */
__attribute__((__interrupt__))
static void pwm_selection_irq(void)
{
	// Check if interrupt flag is set
	if (gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_0))
	{
		// Update selected PWM option on display
		switch (app_pwm_opt)
		{
			case APP_PWM_OPT_FREQUENCY:
			dip204_set_cursor_position(1, 2); dip204_write_data(' ');
			dip204_set_cursor_position(1, 3); dip204_write_data(0xDF);
			app_pwm_opt = APP_PWM_OPT_DUTYCYCLE;
			break;

			default:
			dip204_set_cursor_position(1, 2); dip204_write_data(0xDF);
			dip204_set_cursor_position(1, 3); dip204_write_data(' ');
			app_pwm_opt = APP_PWM_OPT_FREQUENCY;
			break;
		}
		
		// Toggle LED1 and LED2 to indicate selected option
		LED_Toggle(LED1);
		LED_Toggle(LED2);

		// Clear interrupt flag for PB0
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	}
}


/*
 * Potentiometer reading interrupt
 *
 *  This interrupt handler is triggered by the timer/counter,
 *  the trigger interval is selected in conf_app.h.
 *
 *  Running all the update code within the interrupt handler have been
 *  tested and did not make any performance differences due to the
 *  update frequency.
 *  But since one usually want to keep the interrupt handler cycles to a
 *  bare minimum, this function just sets a flag and the update routines
 *  are handled in the main while loop.
 */
__attribute__((__interrupt__))
static void tc_read_pot_irq(void)
{
	// Clear the interrupt flag.
	tc_read_sr(APP_TC, APP_TC_CHANNEL);

	// Set update ADC value flag to true for new reading
	update_adc_value = true;

	// Toggle LED0 as a reading interval indicator
	LED_Toggle(LED0);
}


/*
 * Interrupt initializer
 *
 *  This function initializes the internal interrupts for
 *  triggering PB0 (PWM option) and ADC reading using Timer/Counter module
 */
static void app_interrupt_init(void)
{
	// Disable the interrupts
	cpu_irq_disable();

	// Initialize interrupt vectors.
	INTC_init_interrupts();

	// Register the PB0 int handler with highest priority
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_0, GPIO_RISING_EDGE);
	INTC_register_interrupt(&pwm_selection_irq, (AVR32_GPIO_IRQ_0+88/8), AVR32_INTC_INT0);

	// Register the Timer/Counter (pot reading) int handler with lower priority
	INTC_register_interrupt(&tc_read_pot_irq, APP_TC_IRQ, AVR32_INTC_INT1);

	// Enable the interrupts
	cpu_irq_enable();
}



//////// TIMER/COUNTER /////////


/*
 * Timer/Counter initializer
 *
 *  Initializes the Timer/Counter driver to periodically trigger  
 *  the potentiometer reading. Interval is adjusted in conf_app.h
 *
 *  It uses the TC waveform type with clock source 5 (128 divider)
 */
static void app_tc_init(void)
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

	// We want: (1 / (fPBA / 128)) * RC = 0.01 s, hence RC = (fPBA / 128) / 100
	tc_write_rc(APP_TC, APP_TC_CHANNEL, (sysclk_get_pba_hz() / 128.0 / APP_READ_ADC_INTERVAL));

	// configure the timer interrupt
	tc_configure_interrupts(APP_TC, APP_TC_CHANNEL, &tc_interrupt_config);

	// Start the timer/counter.
	tc_start(APP_TC, APP_TC_CHANNEL);
}



//////// PWM /////////

/*
 * Map a integer between constrains
 *
 *  This is a helper function that is used to map the ADC value (0-1023) to 
 *  duty cycle percentage or frequency. This could be replaced by a look-up table but so far
 *  the overhead doesn't seem too bad and it provides flexibility in regards of debugging. 
 */
static uint32_t app_map_value(uint32_t value, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
	return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/*
 * PWM duty cycle (percentage) update
 *
 *  param: uint8_t percentage (0-100)
 *
 *  This updates the PWM duty cycle via the CUPD and CPD flag in the
 *  PWM driver.
 */
static void app_pwm_update_dutycycle(uint8_t percent)
{
	// Calculate duty cycle period
	current_duty = (current_period * (100 - percent)) / 100;
	
	// Display duty cycle on LCD
	dip204_set_cursor_position(11, 3);
	dip204_printf_string("%3d", percent);
	dip204_write_data('%');

	// Debug
	//if (percent != pwm_dutycycle) { dip204_set_cursor_position(12, 4); dip204_printf_string("%8lu", percent); }
	
	// Update PWM driver with new duty cycle
	avr32_pwm_channel_t pwm_channel = { .ccnt = 0 };
	pwm_channel.CMR.cpd = PWM_UPDATE_DUTY;
	pwm_channel.cupd = current_duty;
	pwm_sync_update_channel(APP_PWM_CHANNEL, &pwm_channel);
	
	// Save current duty
	pwm_dutycycle = percent;
}


/*
 * PWM frequency (Hertz) update
 *
 *  param: uint32_t frequency (100-100000)
 *
 *  This updates the PWM frequency via the CUPD and CPD flag in the
 *  PWM driver. It also "re-updates" the duty cycle with the new frequency setting.
 */
static void app_pwm_update_frequency(uint32_t frequency)
{
	// Calculate period
	current_period = sysclk_get_pba_hz() / frequency;
	
	// Display duty cycle on LCD
	dip204_set_cursor_position(8, 2);
	dip204_printf_string("%6lu", frequency);
	dip204_write_string("Hz");

	// Debug
	//dip204_set_cursor_position(10, 4); dip204_printf_string("P %lu", current_period);
	
	// Update PWM driver with new duty cycle
	avr32_pwm_channel_t pwm_channel = { .ccnt = 0 };
	pwm_channel.CMR.cpd = PWM_UPDATE_PERIOD;
	pwm_channel.cupd = current_period;
	pwm_sync_update_channel(APP_PWM_CHANNEL, &pwm_channel);
	
	// Update the duty cycle to the current frequency
	app_pwm_update_dutycycle(pwm_dutycycle);
	
	// Save current frequency
	pwm_frequency = frequency;
}


/*
 * Common PWM update function
 *
 *  param: app_pwm_opt_t  opt (i.e. APP_PWM_OPT_DUTYCYCLE)
 *  param: uint32_t adc_value (0-1023)
 *
 *  This updates the selected PWM setting with an ADC value. 
 *  It maps the ADC value to the wanted range according to the selected opt.
 */
static void app_pwm_update(app_pwm_opt_t pwm_opt, uint32_t value)
{
	uint8_t percent;
	uint32_t frequency;
	
	switch (pwm_opt)
	{
		// Update PWM frequency
		case APP_PWM_OPT_FREQUENCY:
		// Convert ADC value to wanted frequency range
		frequency = app_map_value(value, 0, 1023, 1, 1000) * 100;
		// Prevent updating if the value is the same as current
		if (frequency != pwm_frequency) {
			app_pwm_update_frequency(frequency);	
		}
		break;
		
		// Update PWM duty cycle
		case APP_PWM_OPT_DUTYCYCLE:
		// Convert ADC value to wanted duty cycle range
		percent = app_map_value(value, 0, 1023, 0, 100);
		// Prevent updating if the value is the same as current
		if (percent != pwm_dutycycle) {
			app_pwm_update_dutycycle(percent);
		}
		break;
	}
}


/*
 * PWM initializing
 *
 *  This initializes the PWM driver with configuration from conf_app.h
 */
static void app_pwm_init(void)
{
	// PWM controller configuration
	pwm_opt_t pwm_opt = {
		.diva = AVR32_PWM_DIVA_CLK_OFF,
		.divb = AVR32_PWM_DIVB_CLK_OFF,
		.prea = AVR32_PWM_PREA_MCK,
		.preb = AVR32_PWM_PREB_MCK
	};

	// PWM channel configuration structure
	avr32_pwm_channel_t pwm_channel = {
		.ccnt = 0
	};
	
	// Hz = (MCK/prescaler)/period
	pwm_channel.cdty = current_duty;				// Channel duty cycle, should be < CPRD
	pwm_channel.cprd = current_period;				// Channel period = (MCK/prescaler)/Hz
	pwm_channel.cupd =  0;							// Channel update is not used here
	pwm_channel.CMR.calg = PWM_MODE_LEFT_ALIGNED;	// Channel mode
	pwm_channel.CMR.cpol = PWM_POLARITY_LOW;		// Channel polarity
	pwm_channel.CMR.cpd  = PWM_UPDATE_DUTY;			// Not used the first time
	pwm_channel.CMR.cpre = AVR32_PWM_CPRE_MCK;		// Channel prescaler, non used here
	
	// Enable the alternative mode of the output pin to connect it to the PWM module within the device
	gpio_enable_module_pin(APP_PWM_PIN, APP_PWM_FUNCTION);

	// Initialize the PWM module
	pwm_init(&pwm_opt);

	// Set channel configuration to PWM channel
	pwm_channel_init(APP_PWM_CHANNEL, &pwm_channel);

	// Start PWM channel
	pwm_start_channels(1 << APP_PWM_CHANNEL);
}



//////// ADC /////////


/*
 * ADC initializing
 *
 *  This initializes the ADC driver with configuration from conf_app.h
 */
static void app_adc_init(void)
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



//////// LCD /////////


/*
 * LCD initializing
 *
 *  This initializes the LCD with SPI
 */
static void app_lcd_init(void)
{
	// LCD/SPI GPIO pin config
	static const gpio_map_t DIP204_SPI_GPIO_MAP = {
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS
	};

	// Add the spi options driver structure for the LCD DIP204
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

	// Assign I/Os to SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));

	// Initialize SPI with the PBA clock frequency
	spi_initMaster(DIP204_SPI, &spiOptions);
	spi_selectionMode(DIP204_SPI, 0, 0, 0);
	spi_enable(DIP204_SPI);
	spi_setupChipReg(DIP204_SPI, &spiOptions, sysclk_get_pba_hz());

	// Initialize LCD DIP204, using IO as backlight instead of PWM
	dip204_init(backlight_IO, true);
	dip204_hide_cursor();
}



//////// APP /////////


/*
 * Read ADC value 
 *
 *  This function reads the ADC value and updates the 
 *  PWM accordingly
 */
static void app_read_adc(void)
{
	// Initiate the ADC reading
	adc_start(&AVR32_ADC);
	
	// Read new value
	uint16_t adc_tmp = adc_get_value(&AVR32_ADC, APP_ADC_POT_CHANNEL);

	// "Poor mans" filtering, since the pot is a bit unstable
	if (adc_tmp > adc_pot_value+1 || adc_tmp < adc_pot_value-1)
	{
		// Save new value for next reading
		adc_pot_value = adc_tmp;
		
		// Update PWM
		app_pwm_update(app_pwm_opt, adc_pot_value);

		// Display pot value on LCD (for debugging)
		dip204_set_cursor_position(10,4); dip204_printf_string("%4d", adc_pot_value);
	}	
}


/*
 * Main app function
 *
 */
int main(void)
{
	// Init system clock
	sysclk_init();

	// Enable the clock to the Timer/Counter driver
	sysclk_enable_peripheral_clock(APP_TC);
	
	// Init default board configuration
	board_init();
	
	// Configure and enable interrupts
	app_interrupt_init();

	// Configure and enable ADC
	app_adc_init();

	// Configure and enable Timer/Counter	
	app_tc_init();

	// Configure and enable PWM
	app_pwm_init();

	// Configure and enable LCD
	app_lcd_init();


	// Set default PWM settings
	app_pwm_update_frequency(pwm_frequency);
	app_pwm_update_dutycycle(pwm_dutycycle);

	// Set default LED states
	LED_On(LED1);
	LED_Off(LED2);

	// Draw LCD layout
	dip204_set_cursor_position(1,1);
	dip204_write_string("LAB03");
	dip204_set_cursor_position(12,1);
	dip204_printf_string("%d/%d MHz", (uint8_t)(sysclk_get_cpu_hz()/1e6), (uint8_t)(sysclk_get_pba_hz()/1e6));
	dip204_set_cursor_position(1,4);
	dip204_write_string(" POT:");
	
	dip204_set_cursor_position(2,2);
	dip204_write_string("Freq:");
	dip204_set_cursor_position(2,3);
	dip204_write_string("Duty:");

	dip204_set_cursor_position(1,2);
	dip204_write_data(0xDF);
	
	
	// Main while loop
	while (true)
	{
		// Checks if the update flag is set
		if (update_adc_value)
		{
			// Indicate update
			LED_On(LED5); 
			
			// Update the ADC value
			app_read_adc();
			
			// Reset update flag
			update_adc_value = false;
	
			LED_Off(LED5);
		}
	}

}
