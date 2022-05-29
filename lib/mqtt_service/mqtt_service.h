#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"


#define MQTT_THING_NAME       CONFIG_AWS_THING_NAME
#define MQTT_HOST             CONFIG_MQTT_BROKER_HOST
#define MQTT_PORT             CONFIG_MQTT_BROKER_PORT

#define AWS_PUBLISH_TOPIC     "bldgluco/v1/pub"
#define AWS_SUBSCRIBE_TOPIC   "bldgluco/v1/sub"

esp_mqtt_client_handle_t mqtt_client;
bool mqtt_subscribed = false;
cJSON *mqtt_message;
cJSON *mqtt_message_device;
cJSON *mqtt_message_data;
cJSON *mqtt_message_status;

void mqtt_service_start(void);
void mqtt_service_pub(void);