#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include <driver/gpio.h>

#include "wifi_manager.h"

#define LED GPIO_NUM_27 

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";


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
}

//Pin Input - Leer si fue presionado. Si fue presionado -> Interrupción -> Ejecuta lógica de glucómetro
//MQTT - Conectarnos como Publisher. Si queremos setear alarmas, debemos conectarnos como subscriber. 
//Pin Input - Reset de WIFI presionando 10s, resetear la memoria flash (o borrar los datos del wifi)
//Pin Output - Mostrar en pantalla (Ssd1306 OLED 128x32).
//Batería - Batería Mini De Lítio Regargable 3.7v 500mah.
//Investigar - Como dormir al micro (Ahorro de energía).


void app_main(void){
	//You can register task before wifi manager start and with different priority
	xTaskCreate(&blink_led, "blink_led", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	/* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

	//Your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
}