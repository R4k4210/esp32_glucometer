#ifndef _OLED_SERVICE_H_
#define _OLED_SERVICE_H_

#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "esp_log.h"

#define OLED_SCL				GPIO_NUM_22
#define OLED_SDA				GPIO_NUM_21
#define O_PAGE_0                0
#define O_PAGE_1                1
#define O_PAGE_2                2
#define O_PAGE_3                3

SSD1306_t dev;
char lineChar[20];

void oled_service_init(void);
void oled_service_write(char text[], int position, bool invert);
void oled_service_measure(float glucose);
void oled_service_clean(void);
void oled_service_welcome(void);

#endif /* _OLED_SERVICE_H_ */