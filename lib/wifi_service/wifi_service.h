#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#ifndef _WIFI_SERVICE_H_
#define _WIFI_SERVICE_H_
    //Tells platformio this should be compiled as C file
    #ifdef __cplusplus
        extern "C" {
    #endif

        void wifi_service_init();
        void wifi_service_scan();
    
    #ifdef __cplusplus
        }
    #endif

#endif