#ifndef _MQTT_SERVICE_H_
#define _MQTT_SERVICE_H_

#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"


#define MQTT_THING_NAME       "ESP32_BLOOD_GLUCOMETER"
#define MQTT_HOST             "2ldx62x5vd0yr-ats.iot.us-west-2.amazonaws.com"
#define MQTT_PORT             8883

#define AWS_PUBLISH_TOPIC     "bldgluco/v1/pub"
#define AWS_SUBSCRIBE_TOPIC   "bldgluco/v1/sub"

typedef struct {
    esp_mqtt_client_handle_t mqtt_client;
    cJSON *mqtt_message;
    cJSON *mqtt_message_device;
    cJSON *mqtt_message_data;
    cJSON *mqtt_message_status;
    bool mqtt_subscribed;
} mqtt_service_t; 

void mqtt_service_start(void);
void mqtt_service_pub(void);
void mqtt_service_stop(void);

#endif /* _MQTT_SERVICE_H_ */