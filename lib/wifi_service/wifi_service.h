#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define ESP_MAXIMUM_RETRY  10

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

typedef struct {
    char ssid[32];
    char password[64];
} ws_config_t;


#ifndef _WIFI_SERVICE_H_
#define _WIFI_SERVICE_H_
    //Tells platformio this should be compiled as C file
    //#ifdef __cplusplus
        //extern "C" {
    //#endif

        void wifi_service_ap_init(void);
        void wifi_service_sta_connect(void);
        void wifi_service_ap_connect(void);
        void wifi_service_set_sta_config(char *_ssid, char *_password);
        ws_config_t wifi_service_get_sta_config();
    
    //#ifdef __cplusplus
        //}
    //#endif

#endif