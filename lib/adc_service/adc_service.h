#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#define ADC1_6_CHANNEL 			ADC1_CHANNEL_6 //GPIO34 if ADC1 SIGNAL
#define ADC1_5_CHANNEL          ADC1_CHANNEL_5 //GPIO33 if ADC1 BATTERY
#define ADC1_ATTEN 				ADC_ATTEN_DB_11

#define NO_OF_SAMPLES 			64

static const adc_bits_width_t width = ADC_WIDTH_BIT_10;

void adc_service_adc1_config(void);
int adc_service_adc1_read(int channel);