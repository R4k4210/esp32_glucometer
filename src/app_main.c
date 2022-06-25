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
#include "esp_sntp.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
//Custom libraries
#include "mqtt_service.h"
#include "adc_service.h"
#include "oled_service.h"
#include "buzzer_service.h"

#define BTN_SENSE 				GPIO_NUM_19
#define LED_EMITER 				GPIO_NUM_23
#define BATTERY_LEVEL           GPIO_NUM_33
#define LOW  					0
#define HIGH 					1
#define NO_OF_SECUENCES			2
#define PERIOD_IN_MILLIS		4000
#define LONG_PRESS_IN_SECONDS   10
#define RESET_TIME_IN_SECONDS	10
#define BAT_CALIBRATION			0.36
#define BAT_VOLTAGE_CUTOFF		3.3
#define BAT_MAX_VOLTAGE			4.2
#define BAT_ADC_RESOLUTION		1023

static const char TAG[] = "MAIN";

enum SoundTimes {
	MEASURING_START = 1, 
	MEASURING_END = 2,
	WIFI_CONNECTED = 3,
	WIFI_DISCONNECT = 2,
	FORCE_RESET = 3
};

enum SoundPeriod {
	SHORT = 100,
	LONG = 300
};

int last_state = HIGH;
int emiter_state = LOW;
int battery_level = 0;
int action_counter = 0;
bool oled_is_in_use = false;
bool is_measuring = false;
bool is_wifi_connected = false;
bool is_initialized = false;
bool bat_r_once = true;
bool write_led = false;
char running_action[16];
uint16_t ticks = 0;

extern mqtt_service_t service_data; //Extern reference the same value name and type in other .c file.

void init_gpio_config(void);
void write_json_message(int glucose);
void get_measurement(void);
void check_bat_lvl(void);
void bat_level_check(void *pvParameter);
void cb_connection_ok(void *pvParameter);
void cb_disconnected(void *pvParameter);
void write_actions(void *pvParameter);

void cpu_main(void *pvParameter){
	for(;;){
		//uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        //ESP_LOGD(TAG, "free RAM is %d.", freeRAM);
		//ESP_LOGD(TAG, "free heap: %d",esp_get_free_heap_size());

		// Debounce
		vTaskDelay(pdMS_TO_TICKS(50));

		if(is_initialized){
			// Re-Read Button State After Debounce
			if (!gpio_get_level(BTN_SENSE)){
				ESP_LOGD(TAG, "BTN Pressed Down.");
				ticks = 0;
				// Loop here while pressed until user lets go, or longer that set time
				while ((!gpio_get_level(BTN_SENSE)) && (++ticks < LONG_PRESS_IN_SECONDS * 100)){
					vTaskDelay(pdMS_TO_TICKS(10));
				} 
				// Did fall here because user held a long press or let go for a short press
				if (ticks >= LONG_PRESS_IN_SECONDS * 100){
					ESP_LOGD(TAG, "Long Press");
					buzzer_service_sound(LONG, WIFI_DISCONNECT);
					wifi_manager_disconnect_async();
					ticks = 0;
				}else{
					ESP_LOGD(TAG, "Short Press");
					if(!is_measuring){
						buzzer_service_sound(SHORT, MEASURING_START);
						oled_service_battery(battery_level);
						strcpy(running_action, "MIDIENDO");
						write_led = true;
						get_measurement();
					}
				}
				// Wait here if they are still holding it
				while(!gpio_get_level(BTN_SENSE)){
					vTaskDelay(pdMS_TO_TICKS(10));
					if(++ticks >= RESET_TIME_IN_SECONDS * 100){
						ESP_LOGD(TAG, "BTN Reset.");
						buzzer_service_sound(LONG, FORCE_RESET);
						esp_restart();
					}
					
				}
				ESP_LOGD(TAG, "BTN Released.");
			}
		}
		
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	
	vTaskDelete(NULL);
}

void app_main(void){
	init_gpio_config();
	oled_service_init();
	oled_service_welcome();
	wifi_manager_start();
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
	wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &cb_disconnected);
	xTaskCreatePinnedToCore(&cpu_main, "cpu_main", 2048, NULL, 3, NULL, 1);
	xTaskCreate(&bat_level_check, "bat_level_check", 2048, NULL, 3, NULL);
	xTaskCreate(&write_actions, "write_actions", 2048, NULL, 4, NULL);
	is_initialized = true;
	check_bat_lvl();
}

void init_gpio_config(void){
	ESP_LOGD(TAG, "Configuring digital GPIO");
	//Outputs
	gpio_set_direction(LED_EMITER, GPIO_MODE_OUTPUT);	
	gpio_set_pull_mode(LED_EMITER, GPIO_PULLUP_ONLY);
	//Inputs
	gpio_set_direction(BTN_SENSE, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_SENSE, GPIO_PULLUP_ONLY);
	ESP_LOGD(TAG, "Configuring analog GPIO");
	adc_service_adc1_config();
	ESP_LOGD(TAG, "Configuring LEDC GPIO");
	buzzer_service_init();
   	ESP_LOGD(TAG, "LEDC config finished.");
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
	//ESP_LOGD(TAG, "MAC ADDRESS: %s", mac_str);
	cJSON_AddStringToObject(service_data.mqtt_message_device, "version", "1.0.0");
	cJSON_AddStringToObject(service_data.mqtt_message_device, "mac_address", mac_str);
	cJSON_AddStringToObject(service_data.mqtt_message_device, "type", "glucometer");
	//status
	cJSON_AddNumberToObject(service_data.mqtt_message_status, "battery", battery_level);
	//data
	cJSON_AddNumberToObject(service_data.mqtt_message_data, "glucose", glucose);
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
    int64_t time_ms = (int64_t)tv_now.tv_sec * 1000LL + (int64_t)tv_now.tv_usec / 1000LL;
	ESP_LOGD(TAG, "The current date/time in Argentina in MS: %lld", (long long)time_ms);
	cJSON_AddNumberToObject(service_data.mqtt_message_data, "timestamp", time_ms);
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
		int sensor_counter = 0;
		int period_last_time = get_millis();
		int sense_last_time = period_last_time;

		ESP_LOGD(TAG, "Last_Time %d", period_last_time);
		ESP_LOGD(TAG, "Timesd %d", i);

		while((get_millis() - period_last_time) <= PERIOD_IN_MILLIS){

			int sensor_value = adc_service_adc1_read(ADC1_6_CHANNEL);
			vTaskDelay(pdMS_TO_TICKS(10));

			if(sensor_value > max_value){
				max_value = sensor_value;
			}
			if(sensor_value < min_value){
				min_value = sensor_value;
			}
			
			ESP_LOGI(TAG, "Sensor value -->> %d\tMinValue -> %d\tMaxValue -> %d", sensor_value, min_value, max_value);
			
			if((get_millis() - sense_last_time) > 500){
				if(emiter_state == LOW){
					gpio_set_level(LED_EMITER, HIGH);
					emiter_state = HIGH;
				}else{
					gpio_set_level(LED_EMITER, LOW);
					emiter_state = LOW;
					sensor_counter++;

					if(sensor_counter == 3){
						int difference = max_value - min_value;
						avg_value += difference;
					}
				}

				sense_last_time = get_millis();
			}
		}
	}
	
	avg_value /= NO_OF_SECUENCES;
	ESP_LOGD(TAG, "Final Glucose: %g mg/dl", avg_value);
	write_led = false;
	
	buzzer_service_sound(SHORT, MEASURING_END);
	oled_service_clean();
	oled_service_measure(avg_value);
	vTaskDelay(pdMS_TO_TICKS(5000));
	oled_service_clean();

	if(is_wifi_connected && service_data.mqtt_subscribed){
		oled_service_battery(battery_level);
		strcpy(running_action, "ENVIANDO");
		write_led = true;
		write_json_message(avg_value);
		//Pub to AWS 
		mqtt_service_pub();
		vTaskDelay(pdMS_TO_TICKS(1000));
		write_led = false;
		oled_service_write("ENVIADO", O_PAGE_2, false);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}

	oled_service_clean();
	is_measuring = false;
	write_led = false;
}

void bat_level_check(void *pvParameter){
	for(;;){
		vTaskDelay(pdMS_TO_TICKS(300000));
		if(!is_measuring){
			check_bat_lvl();
		}
	}
	vTaskDelete(NULL);
}

void check_bat_lvl(void){
	uint32_t adc_value = adc_service_adc1_read(ADC1_5_CHANNEL);
	float voltage = ((adc_value * BAT_VOLTAGE_CUTOFF) / BAT_ADC_RESOLUTION) * 2 + BAT_CALIBRATION;
	float f_bat_lvl = (voltage - BAT_VOLTAGE_CUTOFF) * (100 - 0) / (BAT_MAX_VOLTAGE - BAT_VOLTAGE_CUTOFF) + 0;
	int bat_lvl = (int)(f_bat_lvl - .5);

	if(bat_lvl <= 0){
		battery_level = 0;
	}else if(bat_lvl >= 100){
		battery_level = 100;
	} else {
		if (bat_r_once){
			bat_r_once = false;
			battery_level = bat_lvl;
		}
		
		if ((battery_level > battery_level + 5 || battery_level < battery_level - 5)){
			battery_level = bat_lvl;
		}
	}

	ESP_LOGD(TAG, "Raw: %d\tVoltage: %f\nBattery Level: %d\n", adc_value, voltage, battery_level);
}

void write_actions(void *pvParameter){
	for(;;){
		vTaskDelay(pdMS_TO_TICKS(500));
		if(write_led){
			char m_msg[3][3] = {".  ", ".. ", "..."};
			char m_dest[16];
			strcpy(m_dest, running_action);
			if((action_counter+1) < 4){
				strncat(m_dest, m_msg[action_counter++], 3);
			}

			oled_service_write(m_dest, O_PAGE_2, false);
			
			if(action_counter == 3){
				action_counter = 0;
			}
		}
	}
	vTaskDelete(NULL);
}

void cb_connection_ok(void *pvParameter){
	is_wifi_connected = true;
	oled_service_battery(battery_level);
	oled_service_write("WIFI CONECTADO", O_PAGE_2, false);
	buzzer_service_sound(SHORT, WIFI_CONNECTED);
	oled_service_clean();

	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
	ESP_LOGD(TAG, "I have a connection and my IP is %s!", str_ip);

	mqtt_service_start();
	
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
	setenv("TZ", "UTC+3", 1);
	tzset();
}

void cb_disconnected(void *pvParameter){
	is_wifi_connected = false;
	mqtt_service_stop();
}