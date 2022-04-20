#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include "mqtt_client.h"

#include "wifi_manager.h"
#include "mqtt_service.h"

#define LED GPIO_NUM_27 

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "MAIN";


void blink_led(void *pvParameter){
	gpio_pad_select_gpio(LED);
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);

	for(;;){
		gpio_set_level(LED, 0);
		vTaskDelay(pdMS_TO_TICKS(1000));
		gpio_set_level(LED, 1);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	vTaskDelete(NULL);
}

/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code! This is an example on how you can integrate your code with wifi-manager
 */
void monitoring_task(void *pvParameter){

	for(;;){
		ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		//ESP_LOGI(TAG, "aws ----->> %s", aws_root_c1_pem_start);
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
	
	//If we want to delete the stask
	vTaskDelete(NULL);
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

	mqtt_service_start();
}

void app_main(void){
	//You can register task before wifi manager start and with different priority
	//xTaskCreate(&blink_led, "blink_led", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	/* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

	//Your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
}