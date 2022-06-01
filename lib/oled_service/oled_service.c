#include "oled_service.h"

static const char *TAG = "OLED";

static uint8_t drop[] = {
		0b00000001, 0b00000000, 0b00000000,
		0b00000011, 0b10000000, 0b00000000,
		0b00000111, 0b11000010, 0b00000000,
		0b00001111, 0b11100111, 0b00000000,
		0b00011111, 0b11001111, 0b10000000,
		0b00111111, 0b10011111, 0b11000000,
		0b00111111, 0b00111111, 0b11100000,
		0b01000111, 0b00100111, 0b11100000,
		0b01000111, 0b00100111, 0b11100000,
		0b01000111, 0b00100001, 0b11100000,
		0b01000001, 0b00100001, 0b11100000,
		0b00110001, 0b10011111, 0b11000000,
		0b00001111, 0b11000000, 0b00000000,
	};


void oled_service_init(void){
    ESP_LOGI(TAG, "Init I2C interface as Master...");
	i2c_master_init(&dev, OLED_SDA, OLED_SCL, -1);
    ESP_LOGI(TAG, "SSD1306 128x32 init...");
	ssd1306_init(&dev, 128, 32);
	top = 1;
	center = 1;
	bottom = 4;
	ESP_LOGI(TAG, "SSD1306 128x32 config finished");
}

void oled_service_clean(void){
	ssd1306_clear_screen(&dev, false);
}

void oled_service_write(char text[], bool invert){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, text, strlen(text), invert);
}

void oled_service_scroll_up(char title[]){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, title, strlen(title), true);
	//ssd1306_software_scroll(&dev, 7, 1);
	ssd1306_software_scroll(&dev, (dev._pages - 1), 1);
	for (int line = 0;line < bottom+10;line++) {
		lineChar[0] = 0x01;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void oled_service_scroll_down(void){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "--Scroll  DOWN--", 16, true);
	//ssd1306_software_scroll(&dev, 1, 7);
	ssd1306_software_scroll(&dev, 1, (dev._pages - 1));
	for (int line = 0;line < bottom+10;line++) {
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void oled_service_page_down(void){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "---Page	DOWN---", 16, true);
	ssd1306_software_scroll(&dev, 1, (dev._pages-1));
	for (int line = 0;line < bottom+10; line++) {
		//if ( (line % 7) == 0) ssd1306_scroll_clear(&dev);
		if ((line % (dev._pages-1)) == 0) ssd1306_scroll_clear(&dev);
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void oled_service_horizontal_scroll(char text[]){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, text, strlen(text), false);
	ssd1306_hardware_scroll(&dev, SCROLL_RIGHT);
    vTaskDelay(pdMS_TO_TICKS(5000));
	ssd1306_hardware_scroll(&dev, SCROLL_LEFT);
    vTaskDelay(pdMS_TO_TICKS(5000));
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
}

void oled_service_vertical_scroll(char text[]){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, text, strlen(text), false);
	ssd1306_hardware_scroll(&dev, SCROLL_DOWN);
    vTaskDelay(pdMS_TO_TICKS(5000));
	ssd1306_hardware_scroll(&dev, SCROLL_UP);
    vTaskDelay(pdMS_TO_TICKS(5000));
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
}

void oled_service_invert(void){
	ssd1306_clear_screen(&dev, true);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "  Good Bye!!", 12, true);
	vTaskDelay(pdMS_TO_TICKS(5000));
}

void oled_service_measure(float glucose){
	ssd1306_clear_screen(&dev, false);

	ssd1306_display_text(&dev, 3, "           mg/dL", 16, false);
	vTaskDelay(pdMS_TO_TICKS(100));

	char glu_val[50];
	snprintf(glu_val, sizeof(glu_val),"%d",(int)(glucose + .5));
	ssd1306_display_text_x3(&dev, top, glu_val, strlen(glu_val), false);
	vTaskDelay(pdMS_TO_TICKS(100));

	int bitmapWidth = 3*8;
	int width = ssd1306_get_width(&dev);
	int xpos = width - 18; // center of width
	xpos = xpos - bitmapWidth/2; 
	ESP_LOGD(TAG, "width=%d xpos=%d", width, xpos);

	ssd1306_bitmaps(&dev, xpos, 0, drop, 24, 13, false);
	vTaskDelay(pdMS_TO_TICKS(100));
}