#include "adc_service.h"

static const char *TAG = "ADC";

void adc_service_adc1_config(void){
    adc1_config_width(width);
    adc1_config_channel_atten(ADC1_6_CHANNEL, ADC1_ATTEN);
    adc1_config_channel_atten(ADC1_5_CHANNEL, ADC1_ATTEN);
}

int adc_service_adc1_read(int channel){
	uint32_t adc_reading = 0;
	//Multisampling
	for (int i = 0; i < NO_OF_SAMPLES; i++) {
		adc_reading += adc1_get_raw((adc1_channel_t)channel);
	}
	adc_reading /= NO_OF_SAMPLES;
    
    return adc_reading;
}