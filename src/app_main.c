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

#define BTN_SENSE 				GPIO_NUM_19
#define LED_EMITER 				GPIO_NUM_23

#define LOW  0
#define HIGH 1

//mqtt_service_start();
//wifi_manager_disconnect_async();

static const char TAG[] = "MAIN";
int last_state = HIGH;


void init_gpio_config(void){
	//Outputs
	gpio_set_direction(LED_EMITER, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(LED_EMITER, GPIO_PULLUP_ONLY);

	//Inputs
	gpio_set_direction(BTN_SENSE, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_SENSE, GPIO_PULLUP_ONLY); //If btn is connected to 3.3v, should be pull-down
   	ESP_LOGI(TAG, "GPIO config finished.");
}

void cpu_main(void *pvParameter){
	ESP_LOGI(TAG, "Running main code.");

	for(;;){
		//ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		int btn_state = gpio_get_level(BTN_SENSE);
		vTaskDelay(pdMS_TO_TICKS(100));
		if(btn_state == LOW && last_state == HIGH){ //This is inverted because is pulled-up
			//last_state = LOW;
			ESP_LOGI(TAG, "Button pressed: %d", btn_state);
		}

		adc_service_adc1_read();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
	//If we want to delete the stask
	vTaskDelete(NULL);
}

void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
}

void app_main(void){
	//xTaskCreate(&read_btn, "read_btn", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	init_gpio_config();
	wifi_manager_start();
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

	xTaskCreatePinnedToCore(&cpu_main, "cpu_main", 2048, NULL, 1, NULL, 1);
}

