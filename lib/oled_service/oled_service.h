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

SSD1306_t dev;
int center, top, bottom;
char lineChar[20];

void oled_service_init(void);
void oled_service_write(char text[], bool invert);
void oled_service_scroll_up(char text[]);
void oled_service_scroll_down(void);
void oled_service_page_down(void);
void oled_service_vertical_scroll(char text[]);
void oled_service_horizontal_scroll(char text[]);
void oled_service_invert(void);
void oled_service_measure(float glucose);
void oled_service_clean(void);

#endif /* _OLED_SERVICE_H_ */