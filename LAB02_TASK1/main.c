/**
 * Name			: main.c
 * Author		: Jørgen Ryther Hoem
 * Lab			: Lab 2, Task 1. (ET014G)
 * Description  : Main file
 */

// Includes
#include <asf.h>


// Interrupt handler
__attribute__((__interrupt__))
static void int_handler(void) 
{
	// Check if interrupt flag is set
	if (gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_0)) 
	{
		// Toggle LED1 and LED2
		LED_Toggle( LED1 | LED2 );
		printf("---\nLED1 Off\nLED2 On\n---\n");
	
		// Clear interrupt flag for PB0
		gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
	} 
}


// Main function
int main(void)
{
	// Sysclock and board initialization
	sysclk_init();
	board_init();

	// Initialize interrupt vector table support
	irq_initialize_vectors();

	// Enable GPIO interrupt for PB0
	gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_0, GPIO_FALLING_EDGE);
	INTC_init_interrupts();
	INTC_register_interrupt(&int_handler, (AVR32_GPIO_IRQ_0+88/8), AVR32_INTC_INT0);

	// Enable interrupts
	cpu_irq_enable();

	// Initialize USB CDC over stdio (replacement for RS232)
	stdio_usb_init();
	stdio_usb_enable();

	// Set initial state of LEDs
	LED_On(LED1);
	LED_Off(LED2);
	
	// Display active LEDs state every 2. second
	while (true) {
				
		if (LED_Test(LED1))
			puts("LED1 is On");
	
		if (LED_Test(LED2))
			puts("LED2 is On");

		delay_ms(2000);
	}
}
