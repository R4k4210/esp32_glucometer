#include <Arduino.h>
#include "wifi_service.h"

void setup() {
  char *ssid = "test";
  char *password = "test";
  wifi_service_set_sta_config(ssid, password);
  wifi_service_connect();
}

void loop() {
  // put your main code here, to run repeatedly:
}