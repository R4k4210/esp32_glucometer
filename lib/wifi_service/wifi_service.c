#include "wifi_service.h"

void wifi_service_init(){
    nvs_flash_init();
    tcpip_adapter_init();
}

// Here you can  find the list of errors code of type esp_err_t 
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html
esp_err_t event_handler(void *ctx, system_event_t *event){
    // Check if Scan ends
    if(event->event_id == SYSTEM_EVENT_SCAN_DONE){ // the arrow points to a struct property
        printf("Number  of access points found: %d\n", event->event_info.scan_done.number);

        uint16_t apCount = event->event_info.scan_done.number;
        
        if(apCount == 0){
        return ESP_OK;
        }

        // Allocate dynamic memory space (malloc function) of the size of wifi_ap_record_t struct
        // multiply by access points found. 
        wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

        for(int i=0; i<apCount; i++){
            char *authmode;
            switch(list[i].authmode){
                case WIFI_AUTH_OPEN:
                    authmode = "WIFI_AUTH_OPEN";
                    break;
                case WIFI_AUTH_WEP:
                    authmode = "WIFI_AUTH_WEP";
                    break;
                case WIFI_AUTH_WPA_PSK:
                    authmode = "WIFI_AUTH_WPA_PSK";
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    authmode = "WIFI_AUTH_WPA2_PSK";
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    authmode = "WIFI_AUTH_WPA_WPA2_PSK";
                    break;
                default:
                    authmode = "Unknown";
                    break;
            }

            printf("ssid=%s, rssi=%d, authmode=%s\n", list[i].ssid, list[i].rssi, authmode);
        }
        // Deallocate the memory previous allocated by malloc function
        free(list);
    }

    return ESP_OK;
}

void wifi_service_scan(){   
    wifi_service_init();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    // Let us test a WiFi scan ... 
    wifi_scan_config_t scanConf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = 1
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, 0));
}