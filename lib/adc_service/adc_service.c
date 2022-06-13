#include "adc_service.h"

static const char *TAG = "ADC";

static void check_efuse(void){
	//Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported\n");
    } else {
        ESP_LOGI(TAG, "eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported\n");
    } else {
        ESP_LOGI(TAG, "eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type){
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Characterized using eFuse Vref\n");
    } else {
        ESP_LOGI(TAG, "Characterized using Default Vref\n");
    }
}

void adc_service_adc1_config(void){
	//Check if Two Point or Vref are burned into eFuse
    //check_efuse();

	//Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(width_10_bits);
        adc1_config_channel_atten(ADC1_6_CHANNEL, ADC1_ATTEN);
        adc1_config_channel_atten(ADC1_5_CHANNEL, ADC1_ATTEN); // Se requiere configurar la atenuacion para un pin adc nuevo?
    } else {
        adc2_config_channel_atten((adc2_channel_t)ADC1_6_CHANNEL, ADC1_ATTEN);
        adc2_config_channel_atten((adc2_channel_t)ADC1_5_CHANNEL, ADC1_ATTEN);
    }

	//Characterize ADC
    //adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    //esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, ADC1_ATTEN, width_10_bits, DEFAULT_VREF, adc_chars);
    //print_char_val_type(val_type);
}

int adc_service_adc1_read(void){
	uint32_t adc_reading = 0;
	//Multisampling
	for (int i = 0; i < NO_OF_SAMPLES; i++) {
		if (unit == ADC_UNIT_1) {
			adc_reading += adc1_get_raw((adc1_channel_t)ADC1_6_CHANNEL);
		} else {
			int raw;
			adc2_get_raw((adc2_channel_t)ADC1_6_CHANNEL, width_10_bits, &raw);
			adc_reading += raw;
		}
	}
	adc_reading /= NO_OF_SAMPLES;

	//Convert adc_reading to voltage in mV using eFuse
	//uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
	//ESP_LOGI(TAG, "Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
    return adc_reading;
}

int adc_service_adc1_5_read(void){
    uint32_t adc_reading = 0;

    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++){
        if (unit == ADC_UNIT_1){
            adc_reading += adc1_get_raw((adc1_channel_t)ADC1_5_CHANNEL);
        }
        else{
            int raw;
            adc2_get_raw((adc2_channel_t)ADC1_5_CHANNEL, width_10_bits, &raw);
            adc_reading += raw;
        }
    }

    adc_reading /= NO_OF_SAMPLES;

    return adc_reading;
}