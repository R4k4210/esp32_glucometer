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
#include "mqtt_client.h"
#include "wifi_manager.h"

//Custom libraries
#include "mqtt_service.h"
#include "adc_service.h"
#include "oled_service.h"

#define BTN_SENSE 				GPIO_NUM_19
#define LED_EMITER 				GPIO_NUM_23
#define BATTERY_LEVEL           GPIO_NUM_33

#define LOW  					0
#define HIGH 					1
#define NO_OF_SECUENCES			2
#define PERIOD_IN_MILLIS		4000
#define LONG_PRESS_IN_SECONDS   10
#define RESET_TIME_IN_SECONDS	30

static const char TAG[] = "MAIN";
int last_state = HIGH;
int emiter_state = LOW;
int battery_level = 0;
bool oled_is_in_use = false;
bool is_measuring = false;
bool is_wifi_connected = false;
bool bat_r_once = true;

uint16_t ticks = 0;
extern mqtt_service_t service_data; //Extern reference the same value name and type in other .c file.

void init_gpio_config(void);
void write_json_message(int glucose);
void get_measurement(void);
void get_battery_level(void);
void cb_connection_ok(void *pvParameter);
void cb_disconnected(void *pvParameter);

void cpu_main(void *pvParameter){
	ESP_LOGI(TAG, "Running main code.");

	for(;;){
		//uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        //ESP_LOGI(TAG, "free RAM is %d.", freeRAM);
		//ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		
		/*
		if(!is_measuring){
			get_measurement();
		}
		*/
        /*
		// Debounce
		vTaskDelay(pdMS_TO_TICKS(50));
		// Re-Read Button State After Debounce
		//if (!gpio_get_level(BTN_SENSE)){
			//ESP_LOGI(TAG, "BTN Pressed Down.");
			ticks = 0;
			// Loop here while pressed until user lets go, or longer that set time
			while ((!gpio_get_level(BTN_SENSE)) && (++ticks < LONG_PRESS_IN_SECONDS * 100)){
				vTaskDelay(pdMS_TO_TICKS(10));
			} 
			// Did fall here because user held a long press or let go for a short press
			if (ticks >= LONG_PRESS_IN_SECONDS * 100){
				ESP_LOGI(TAG, "Long Press");
				if(is_wifi_connected && !is_measuring){
					wifi_manager_disconnect_async();
				}
			}else{
				ESP_LOGI(TAG, "Short Press");
				if(!is_measuring){
					get_measurement();
				}
			}
			// Wait here if they are still holding it
			//If you hold to reset, wifi will be disconnected
			while(!gpio_get_level(BTN_SENSE)){
				if(++ticks >= RESET_TIME_IN_SECONDS * 100){
					ESP_LOGI(TAG, "BTN Reset.");
					esp_restart();
				}
			}
			//ESP_LOGI(TAG, "BTN Released.");
		//}
	    */
		//vTaskDelay(pdMS_TO_TICKS(100));
		vTaskDelay(pdMS_TO_TICKS(2000));
		get_battery_level();
	}
	
	//If we want to delete the task
	vTaskDelete(NULL);
}

void app_main(void){
	init_gpio_config();
	oled_service_init();
	//Presentation
	oled_service_write("GLUCOMETRO", false);
	vTaskDelay(pdMS_TO_TICKS(3000));
	oled_service_clean();
	
	//End presentation
	wifi_manager_start();
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
	wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &cb_disconnected);
	xTaskCreatePinnedToCore(&cpu_main, "cpu_main", 2048, NULL, 1, NULL, 1);
}

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

int get_millis(void){
	return esp_timer_get_time() / 1000;
}

void get_measurement(void){
	is_measuring = true;
	float avg_value = 0;

	for(int i=0; i < NO_OF_SECUENCES; i++) {
		int max_value = 0;
		int min_value = 1023;
		int sensor_value = 0;
		int sensor_counter = 0;
		int last_time = get_millis();
		int oled_counter = 0;

		while((get_millis() - last_time) <= PERIOD_IN_MILLIS){
			//ESP_LOGI(TAG, "time %d", get_millis() - last_time);
			vTaskDelay(pdMS_TO_TICKS(10));		

			sensor_value = adc_service_adc1_read();

			if(sensor_value > max_value){
				max_value = sensor_value;
			}

			if(sensor_value < min_value){
				min_value = sensor_value;
			}
		
			//ESP_LOGI(TAG, "Sensor value -->> %d\tMinValue -> %d\tMaxValue -> %d", sensor_value, min_value, max_value);

			if((get_millis() - last_time) % 500 == 0){

				char m_msg[3][3] = {".", "..", "..."};
				char m_dest[11] = "MIDIENDO";
				if((oled_counter+1) < 4){
					strncat(m_dest, m_msg[oled_counter], 3);
				}
				oled_service_write(m_dest, false);	

				oled_counter++;

				if(emiter_state == LOW){
					gpio_set_level(LED_EMITER, HIGH);
					emiter_state = HIGH;
				}else{
					gpio_set_level(LED_EMITER, LOW);
					emiter_state = LOW;
					sensor_counter++;

					if(sensor_counter == 3){
						int difference = max_value - min_value;
						ESP_LOGI(TAG, "ADC READ FINAL DIFFERENCE %d", difference);
						avg_value += difference;
						//float voltage = difference * 3.3 / 4095.0;
						//ESP_LOGI(TAG, "Partial Glucose: %d mg/dl\tVoltage: %fV\n", difference, voltage);
					}
				}
			}
		}
	}
	
	avg_value /= NO_OF_SECUENCES;
	ESP_LOGI(TAG, "Final Glucose: %g mg/dl", avg_value);
	
	oled_service_clean();
	oled_service_measure(avg_value);

	vTaskDelay(pdMS_TO_TICKS(5000));

	if(is_wifi_connected && service_data.mqtt_subscribed){
		oled_service_write("ENVIANDO...", false);
		write_json_message(avg_value);
		//Pub to AWS 
		mqtt_service_pub();
		oled_service_write("ENVIADO", false);
		vTaskDelay(pdMS_TO_TICKS(1500));
		oled_service_clean();
	}

	is_measuring = false;
	oled_service_clean();
}

void get_battery_level(void){
	uint32_t adc_value = 0;
	float calibration = 0.36;
	float voltage_cutoff = 3.3;

	adc_value = adc_service_adc1_5_read();

	float voltage = ((adc_value * voltage_cutoff) / 1024) * 2 + calibration;
	float f_bat_lvl = (voltage - voltage_cutoff) * (100 - 0) / (4.2 - voltage_cutoff) + 0;

	if (bat_r_once){
		bat_r_once = false;
		battery_level = (int)(f_bat_lvl - .5);
	}
	
	if ((battery_level > battery_level + 5 || battery_level < battery_level - 5)){
		battery_level = (int)(f_bat_lvl - .5);
	}
 
	ESP_LOGI(TAG, "Raw: %d\tVoltage: %f\nBattery Level: %d\n", adc_value, voltage, battery_level);
}

void cb_connection_ok(void *pvParameter){
	is_wifi_connected = true;
	oled_service_write("CONECTADO", false);
	vTaskDelay(pdMS_TO_TICKS(3000));
	oled_service_clean();
	
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

	mqtt_service_start();
}

void cb_disconnected(void *pvParameter){
	is_wifi_connected = false;
	mqtt_service_stop();
}