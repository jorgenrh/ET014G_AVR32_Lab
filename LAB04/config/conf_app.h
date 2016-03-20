/*
 * conf_app.h
 *
 * Created: 09.03.2016 13.20.38
 *  Author: jrh
 */ 
#ifndef CONF_APP_H_
#define CONF_APP_H_

// Default logfile
#define APP_LOG_FILENAME      "logfile.csv"

// Default memory slot
#define APP_SD_MMC_SLOT       0

// ADC reading interval in Hz
#define APP_READ_ADC_FREQ     50

// ADC configuration
#define APP_ADC_POT_CHANNEL   1
#define APP_ADC_POT_PIN       AVR32_ADC_AD_1_PIN
#define APP_ADC_POT_FUNCTION  AVR32_ADC_AD_1_FUNCTION

// Timer/Counter configuration
#define APP_TC                (&AVR32_TC)
#define APP_TC_CHANNEL        0
#define APP_TC_IRQ            AVR32_TC_IRQ0
#define APP_TC_IRQ_GROUP      AVR32_TC_IRQ_GROUP
#define APP_TC_IRQ_PRIORITY   AVR32_INTC_INT0



#endif /* CONF_APP_H_ */