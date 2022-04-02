#include <wifi_service.h>

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static const char *TAG = "WIFI-STATION";

//Custom struct to handle wifi credentials
ws_config_t config;

void wifi_service_init(){
    //initialize NVS flash. All wifi configuration is stored here.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //Create an event group for FreeRTOS
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    //Set default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){ 
    //Check base event and event id
    if(event_base == WIFI_EVENT){
        //Wifi events
        if(event_id == WIFI_EVENT_STA_START){
            ESP_LOGI(TAG, "Connecting to Station...");
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else if(event_id == WIFI_EVENT_STA_DISCONNECTED) {
            //If connection fails, try to retry
            if (s_retry_num < ESP_MAXIMUM_RETRY) {
                ESP_ERROR_CHECK(esp_wifi_connect());
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else{
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                ESP_LOGI(TAG, "Connect to the AP fail");
            }
        }
    //IP events
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_service_sta_connect(){
    wifi_service_init();

    //Create the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //Create a default WIFI STA. In case of error API aborts.
    //Pointer to esp-netif instance
    esp_netif_create_default_wifi_sta();
    //Context identifying an instance of a registered event handler
    esp_event_handler_instance_t instance_any_id; 
    esp_event_handler_instance_t instance_got_ip;
    //Register WIFI events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,             //Base event
                                                        ESP_EVENT_ANY_ID,       //Event id
                                                        &event_handler,         //Event handler function
                                                        NULL,                   //Event handler args
                                                        &instance_any_id));     //Instance
    //Register IP events                                                        
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,               
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t sta_config = {
        .sta = {
            //.ssid = "test",
            //.password = "test",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	        //.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
        },
    };
    //Setting mode as Station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //For strings we need strcpy function
    strcpy((char*)sta_config.sta.ssid, config.ssid);
    strcpy((char*)sta_config.sta.password, config.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to ap SSID:%s password:%s", config.ssid, config.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", config.ssid, config.password);
    } else {
        ESP_LOGI(TAG, "UNEXPECTED EVENT");
    }
}

void wifi_service_ap_connect(){
    wifi_service_init();

    //WIFI_AUTH_WPA_WPA2_PSK needs 8 characters password or will loop on reboot
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "test",
            .ssid_len = strlen("test"),
            .channel = 1,
            .password = "test1234", 
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK, 
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

}

void wifi_service_set_sta_config(char *_ssid, char *_password){
    strcpy(config.ssid, _ssid);
    strcpy(config.password, _password);
}

ws_config_t wifi_service_get_sta_config(){
    return config;
}