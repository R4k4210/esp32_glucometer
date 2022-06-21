#include <mqtt_service.h>

static const char *TAG = "MQTT";

extern const uint8_t aws_root_c1_pem_start[] asm("_binary_aws_root_c1_pem_start");
extern const uint8_t aws_root_c1_pem_end[] asm("_binary_aws_root_c1_pem_end");

extern const uint8_t device_crt_start[] asm("_binary_device_pem_crt_start");
extern const uint8_t device_crt_end[] asm("_binary_device_pem_crt_end");

extern const uint8_t private_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_key_end[] asm("_binary_private_pem_key_end");

const esp_mqtt_client_config_t mqtt_config = {
    .transport = MQTT_TRANSPORT_OVER_SSL,
    .host = MQTT_HOST,
    .port = MQTT_PORT,
    .client_id = MQTT_THING_NAME,
    .cert_pem = (const char *)aws_root_c1_pem_start,
    .client_cert_pem = (const char*)device_crt_start,
    .client_key_pem = (const char*)private_key_start
};

mqtt_service_t service_data = {
    .mqtt_subscribed = false,
};

static void log_error_if_nonzero(const char *message, int error_code){
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, AWS_PUBLISH_TOPIC, 0);
            ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            //msg_id = esp_mqtt_client_unsubscribe(client, AWS_PUBLISH_TOPIC);
            //ESP_LOGD(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            service_data.mqtt_subscribed = true;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            service_data.mqtt_subscribed = false;
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGD(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGD(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_service_start(void){
    service_data.mqtt_client = esp_mqtt_client_init(&mqtt_config);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(service_data.mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(service_data.mqtt_client);
}

void mqtt_service_stop(void){
    esp_mqtt_client_stop(service_data.mqtt_client);
}

void mqtt_service_pub(void){
    if(service_data.mqtt_subscribed){
        char *stringify_message = cJSON_Print(service_data.mqtt_message);
        int sended = esp_mqtt_client_publish(service_data.mqtt_client, AWS_PUBLISH_TOPIC, stringify_message, 0, 0, 0);
        ESP_LOGD(TAG, "Sent publish successful, msg_id=%d", sended);
        cJSON_Delete(service_data.mqtt_message);
    }else{
        ESP_LOGD(TAG, "MQTT is not suscribed");
    }
}