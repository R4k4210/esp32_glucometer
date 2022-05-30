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
#define NO_OF_SECUENCES			2
#define NO_OF_SENSED			4
#define LONG_PRESS_IN_SECONDS   10

static const char TAG[] = "MAIN";
int last_state = HIGH;
int emiter_state = LOW;
bool oled_is_in_use = false;
bool is_measuring = false;
uint16_t ticks = 0;
extern mqtt_service_t service_data; //Extern reference the same value name and type in other .c file.

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

	service_data.mqtt_message = cJSON_CreateObject();
	service_data.mqtt_message_data = cJSON_CreateObject();
	service_data.mqtt_message_device = cJSON_CreateObject();
	service_data.mqtt_message_status = cJSON_CreateObject();

	//device
	unsigned char mac[6] = {0};
	char mac_str[18];
	esp_read_mac(mac, ESP_MAC_WIFI_STA); 
	snprintf(mac_str,18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	ESP_LOGI(TAG, "MAC ADDRESS: %s", mac_str);
	cJSON_AddStringToObject(service_data.mqtt_message_device, "version", "1.0.0");
	cJSON_AddStringToObject(service_data.mqtt_message_device, "mac_address", mac_str);
	cJSON_AddStringToObject(service_data.mqtt_message_device, "type", "glucometer");

	//status
	cJSON_AddNumberToObject(service_data.mqtt_message_status, "battery", 100);

	//data
	cJSON_AddNumberToObject(service_data.mqtt_message_data, "glucose", glucose);
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
	cJSON_AddNumberToObject(service_data.mqtt_message_data, "timestamp", time_us);
	//root
	cJSON_AddItemToObject(service_data.mqtt_message, "device", service_data.mqtt_message_device);
	cJSON_AddItemToObject(service_data.mqtt_message, "status", service_data.mqtt_message_status);
	cJSON_AddItemToObject(service_data.mqtt_message, "data", service_data.mqtt_message_data);
}

void get_measurement(void){
	is_measuring = true;
	float avg_value = 0;

	for(int i=0; i < NO_OF_SECUENCES; i++) {
		ESP_LOGI(TAG, "NO OF SECUENCES %d", i+1);
		int max_value = 0;
		int min_value = 4095;
		int sensor_value = 0;

		for(int c=0; c < NO_OF_SENSED; c++){
			ESP_LOGI(TAG, "NO OF SENSE %d", c+1);
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

				if(c == (NO_OF_SENSED - 1)){
					int difference = max_value - min_value;
					avg_value += difference;
					float voltage = difference * 3.3 / 4095;
					ESP_LOGI(TAG, "Glucose: %d mg/dl\tVoltage: %fV\n", difference, voltage);
				}
			}
		}
	}

	avg_value /= NO_OF_SECUENCES;
	char oled_msg[50];
	snprintf(oled_msg, sizeof(oled_msg),"%g mg/dL",avg_value);
	oled_service_write(oled_msg, false);

	if(service_data.mqtt_subscribed){
		write_json_message(avg_value);
		ESP_LOGI(TAG, "Call MQTT_SERVICE Pub method");
		oled_service_write("Enviando datos...", false);
		mqtt_service_pub();
	}

	is_measuring = false;
}

void cpu_main(void *pvParameter){
	ESP_LOGI(TAG, "Running main code.");

	for(;;){
		//uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        //ESP_LOGI(TAG, "free RAM is %d.", freeRAM);
		//ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		//vTaskDelay(pdMS_TO_TICKS(10000));

		// Wait here to detect press
		while(gpio_get_level(BTN_SENSE)){
			vTaskDelay(pdMS_TO_TICKS(125));
		}
		// Debounce
		vTaskDelay(pdMS_TO_TICKS(50));
		// Re-Read Button State After Debounce
		if (!gpio_get_level(BTN_SENSE)){
			ESP_LOGI(TAG, "BTN Pressed Down.");
			ticks = 0;
			// Loop here while pressed until user lets go, or longer that set time
			while ((!gpio_get_level(BTN_SENSE)) && (++ticks < LONG_PRESS_IN_SECONDS * 100)){
				vTaskDelay(pdMS_TO_TICKS(10));
			} 
			// Did fall here because user held a long press or let go for a short press
			if (ticks >= LONG_PRESS_IN_SECONDS * 100){
				ESP_LOGI(TAG, "Long Press");
				wifi_manager_disconnect_async();
			}else{
				ESP_LOGI(TAG, "Short Press");
				if(!is_measuring){
					get_measurement();
				}
			}
			// Wait here if they are still holding it
			while(!gpio_get_level(BTN_SENSE)){
				vTaskDelay(pdMS_TO_TICKS(100));
			}
			ESP_LOGI(TAG, "BTN Released.");
		}

		vTaskDelay(pdMS_TO_TICKS(100));
/*
		int btn_state = gpio_get_level(BTN_SENSE);
		vTaskDelay(pdMS_TO_TICKS(100));
		if(btn_state == LOW && last_state == HIGH){ //This is inverted because is pulled-up
			//last_state = LOW;
			ESP_LOGI(TAG, "Button pressed: %d", btn_state);
			get_measurement();
			
		}
*/
	}
	
	//If we want to delete the task
	vTaskDelete(NULL);
}

void cb_connecting(void *pvParameter){
	oled_service_write("Conectando a la red...", false);
}

void cb_connection_ok(void *pvParameter){
	oled_service_write("Conectado a la red", false);
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

	mqtt_service_start();
}

void cb_disconnected(void *pvParameter){
	mqtt_service_stop();
}

void app_main(void){
	init_gpio_config();
	oled_service_init();
	wifi_manager_start();
	wifi_manager_set_callback(WM_ORDER_CONNECT_STA, &cb_connecting);
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
	wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &cb_disconnected);
	xTaskCreatePinnedToCore(&cpu_main, "cpu_main", 2048, NULL, 1, NULL, 1);
}