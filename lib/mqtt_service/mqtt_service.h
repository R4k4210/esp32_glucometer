#ifndef _MQTT_SERVICE_H_
#define _MQTT_SERVICE_H_

#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"


#define MQTT_THING_NAME       CONFIG_AWS_THING_NAME
#define MQTT_HOST             CONFIG_MQTT_BROKER_HOST
#define MQTT_PORT             CONFIG_MQTT_BROKER_PORT

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