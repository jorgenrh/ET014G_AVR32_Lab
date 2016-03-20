/*
 * conf_app.h
 *
 * Created: 06.03.2016 00.50.34
 *  Author: jrh
 */ 


#ifndef CONF_APP_H_
#define CONF_APP_H_


#define APP_TC				  (&AVR32_TC)
#define APP_TC_CHANNEL        0
#define APP_TC_IRQ            AVR32_TC_IRQ0
#define APP_TC_IRQ_GROUP      AVR32_TC_IRQ_GROUP
#define APP_TC_IRQ_PRIORITY   AVR32_INTC_INT0


#define APP_PWM_CHANNEL       3
#define APP_PWM_PIN           AVR32_PWM_3_PIN
#define APP_PWM_FUNCTION      AVR32_PWM_3_FUNCTION


#define APP_ADC_POT_CHANNEL   1
#define APP_ADC_POT_PIN       AVR32_ADC_AD_1_PIN
#define APP_ADC_POT_FUNCTION  AVR32_ADC_AD_1_FUNCTION

#define APP_READ_ADC_INTERVAL 50 // Hz



#endif /* CONF_APP_H_ */