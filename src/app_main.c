#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "mqtt_client.h"
#include "wifi_manager.h"

//Custom libraries
#include "mqtt_service.h"
#include "adc_service.h"
#include "oled_service.h"

#define BTN_SENSE 				GPIO_NUM_19
#define LED_EMITER 				GPIO_NUM_23

#define LOW  					0
#define HIGH 					1

static const char TAG[] = "MAIN";
int last_state = HIGH;
int max_value = 0;
int min_value = 4095;
int sensor_value = 0;
int sensor_counter = 0;
int emiter_state = LOW;


void init_gpio_config(void){
	ESP_LOGI(TAG, "Configuring digital GPIO");
	//Outputs
	gpio_set_direction(LED_EMITER, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(LED_EMITER, GPIO_PULLUP_ONLY);
	//Inputs
	gpio_set_direction(BTN_SENSE, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_SENSE, GPIO_PULLUP_ONLY); //If btn is connected to 3.3v, should be pull-down
	ESP_LOGI(TAG, "Configuring analog GPIO");
	adc_service_adc1_config();
   	ESP_LOGI(TAG, "GPIO config finished.");
}

void write_json_message(int glucose){
	/* Example structure
	{
		device: {
			mac_address: "ASC-ASDASD-ADASDA",
			version: "1.0.1"
		},
		status: {
			battery: 55,
			wifi: "blablabla"
		},
		data: {
			glucometer: 160,
			timestamp: 1234567
		}
	}
	*/

	mqtt_message = cJSON_CreateObject();
	mqtt_message_data = cJSON_CreateObject();
	mqtt_message_device = cJSON_CreateObject();
	mqtt_message_status = cJSON_CreateObject();
	//MAC Address
	unsigned char mac_base[6] = {0};
	esp_efuse_mac_get_default(mac_base);
	ESP_LOGI(TAG, "MAC ADDRESS: %s", mac_base);
	//sprintf("%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0],mac_base[1],mac_base[2],mac_base[3],mac_base[4],mac_base[5]);
	//device
	cJSON_AddStringToObject(mqtt_message_device, "version", "1.0.0");
	//cJSON_AddNumberToObject(mqtt_message_device, "mac_address", mac_base); //Esto debe ser string
	cJSON_AddStringToObject(mqtt_message_device, "type", "glucometer");
	//status
	cJSON_AddNumberToObject(mqtt_message_status, "battery", 100);
	//data
	cJSON_AddNumberToObject(mqtt_message_data, "glucose", glucose);
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
	cJSON_AddNumberToObject(mqtt_message_data, "timestamp", time_us);
	//device
	cJSON_AddItemToObject(mqtt_message, "device", mqtt_message_device);
	//status
	cJSON_AddItemToObject(mqtt_message, "status", mqtt_message_status);
	//data
	cJSON_AddItemToObject(mqtt_message, "data", mqtt_message_data);
}

void cpu_main(void *pvParameter){
	ESP_LOGI(TAG, "Running main code.");

	for(;;){
		//ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		//vTaskDelay(pdMS_TO_TICKS(10000));

		/*
		int btn_state = gpio_get_level(BTN_SENSE);
		vTaskDelay(pdMS_TO_TICKS(100));
		if(btn_state == LOW && last_state == HIGH){ //This is inverted because is pulled-up
			//last_state = LOW;
			ESP_LOGI(TAG, "Button pressed: %d", btn_state);
		}
		*/

		sensor_value = adc_service_adc1_read();

		if(sensor_value > max_value){
			max_value = sensor_value;
		}

		if(sensor_value < min_value){
			min_value = sensor_value;
		}

		ESP_LOGI(TAG, "Sensor value -->> %d\tMinValue -> %d\tMaxValue -> %d", sensor_value, min_value, max_value);
		vTaskDelay(pdMS_TO_TICKS(500));
		
		if(emiter_state == LOW){
			gpio_set_level(LED_EMITER, HIGH);
			emiter_state = HIGH;
		}else{
			gpio_set_level(LED_EMITER, LOW);
			emiter_state = LOW;
			sensor_counter += 1;

			if(sensor_counter == 3){
				sensor_counter = 0;
				int difference = max_value - min_value;
				max_value = 0;
				min_value = 4095;
				float voltage = difference * 3.3 / 4095;
				ESP_LOGI(TAG, "Glucose: %d mg/dl\tVoltage: %fV\n", difference, voltage);
				write_json_message(difference);
				mqtt_service_pub();
			}
		}

	}
	
	//If we want to delete the task
	vTaskDelete(NULL);
}

/*
	void print_mac(const unsigned char *mac) {
		printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	}
*/

void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
	mqtt_service_start();
	//wifi_manager_disconnect_async();
}

void app_main(void){
	init_gpio_config();
	oled_service_init();
	wifi_manager_start();
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
	xTaskCreatePinnedToCore(&cpu_main, "cpu_main", 2048, NULL, 1, NULL, 1);
}