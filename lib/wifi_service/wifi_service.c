#include "wifi_service.h"

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

ws_config_t config;

void wifi_service_init(){
    nvs_flash_init();
    s_wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

// Here you can  find the list of errors code of type esp_err_t 
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html
static esp_err_t scan_event_handler(void *ctx, system_event_t *event){
    // Check if Scan ends
    if(event->event_id == SYSTEM_EVENT_SCAN_DONE){ // the arrow points to a struct property
        printf("Number of access points found: %d\n", event->event_info.scan_done.number);
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

static esp_err_t connect_event_handler(void *ctx, system_event_t *event){ 
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            printf("SYSTEM_EVENT_STA_START... \n");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("SYSTEM_EVENT_STA_GOT_IP... \n");
            printf("Our IP address is " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            {
                if (s_retry_num < ESP_MAXIMUM_RETRY) {
                    ESP_ERROR_CHECK(esp_wifi_connect());
                    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                    s_retry_num++;
                    printf("Retry to connect to the AP");
                }
                printf("Connect to the AP fail\n");
                break;
            }
        default:
            break;
    }
    return ESP_OK;
}

void wifi_service_sta_connect(){
    wifi_service_init();

    ESP_ERROR_CHECK(esp_event_loop_init(connect_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t sta_config;
    sta_config.sta.bssid_set = 0;
    strcpy(sta_config.sta.ssid, config.ssid);
    strcpy(sta_config.sta.password, config.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_service_scan(){   
    wifi_service_init();

    ESP_ERROR_CHECK(esp_event_loop_init(scan_event_handler, NULL));
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

void wifi_service_set_sta_config(char *_ssid, char *_password){
    strcpy(config.ssid, _ssid);
    strcpy(config.password, _password);
}

ws_config_t wifi_service_get_sta_config(){
    return config;
}