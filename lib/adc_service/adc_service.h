#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

//adc config
#if CONFIG_IDF_TARGET_ESP32

#define ADC1_6_CHANNEL 			ADC1_CHANNEL_6 //GPIO34 if ADC1
#define ADC1_5_CHANNEL          ADC1_CHANNEL_5 //GPIO33
#define ADC1_ATTEN 				ADC_ATTEN_DB_11

#endif

#define NO_OF_SAMPLES 			64
#define DEFAULT_VREF			1100

static const adc_bits_width_t width_10_bits = ADC_WIDTH_BIT_10;
static const adc_unit_t unit = ADC_UNIT_1;
static esp_adc_cal_characteristics_t *adc_chars;

void adc_service_adc1_config(void);
int adc_service_adc1_read(void);
int adc_service_adc1_5_read(void);