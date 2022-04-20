#include "mqtt_client.h"
#include "esp_log.h"


#define MQTT_THING_NAME CONFIG_AWS_THING_NAME
#define MQTT_HOST CONFIG_MQTT_BROKER_HOST
#define MQTT_PORT CONFIG_MQTT_BROKER_PORT

#define AWS_PUBLISH_TOPIC "bldgluco/v1/pub"
#define AWS_SUBSCRIBE_TOPIC "bldgluco/v1/sub"

void mqtt_service_start(void);


