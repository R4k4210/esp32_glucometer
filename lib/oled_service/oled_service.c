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

static uint8_t ise[] = {
	0b00000001, 0b11111111, 0b10000000,
	0b00000000, 0b01111110, 0b00000000,
	0b00000000, 0b00111100, 0b00000000,
	0b00000000, 0b01111110, 0b00000000,
	0b00000000, 0b11111111, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000110, 0b11111111, 0b01100000,
	0b00000110, 0b11111111, 0b01100000,
	0b00000110, 0b11111111, 0b01100000,
	0b00000110, 0b11111111, 0b01100000,
	0b00000000, 0b11111111, 0b00000000,
	0b00000000, 0b01100110, 0b00000000,
	0b00000000, 0b01100110, 0b00000000,
};

static uint8_t bat_full[] = {
	0b01111111, 0b11111111, 0b11111000,
	0b11000000, 0b00000000, 0b00001100,
	0b11001111, 0b00111100, 0b11001100,
	0b11001111, 0b00111100, 0b11001111,
	0b11001110, 0b01111001, 0b11001111,
	0b11001110, 0b01111001, 0b11001111,
	0b11001100, 0b11110011, 0b11001111,
	0b11001100, 0b11110011, 0b11001100,
	0b11000000, 0b00000000, 0b00001100,
	0b01111111, 0b11111111, 0b11111000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
};

static uint8_t bat_mid[] = {
	0b01111111, 0b11111111, 0b11111000,
	0b11000000, 0b00000000, 0b00001100,
	0b11001111, 0b00111100, 0b00001100,
	0b11001111, 0b00111100, 0b00001111,
	0b11001110, 0b01111000, 0b00001111,
	0b11001110, 0b01111000, 0b00001111,
	0b11001100, 0b11110000, 0b00001111,
	0b11001100, 0b11110000, 0b00001100,
	0b11000000, 0b00000000, 0b00001100,
	0b01111111, 0b11111111, 0b11111000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
};

static uint8_t bat_low[] = {
	0b01111111, 0b11111111, 0b11111000,
	0b11000000, 0b00000000, 0b00001100,
	0b11001111, 0b00000000, 0b00001100,
	0b11001111, 0b00000000, 0b00001111,
	0b11001110, 0b00000000, 0b00001111,
	0b11001110, 0b00000000, 0b00001111,
	0b11001100, 0b00000000, 0b00001111,
	0b11001100, 0b00000000, 0b00001100,
	0b11000000, 0b00000000, 0b00001100,
	0b01111111, 0b11111111, 0b11111000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000,
};

void oled_service_init(void){
    ESP_LOGD(TAG, "Init I2C interface as Master...");
	i2c_master_init(&dev, OLED_SDA, OLED_SCL, -1);
    ESP_LOGD(TAG, "SSD1306 128x32 init...");
	ssd1306_init(&dev, 128, 32);
	ESP_LOGD(TAG, "SSD1306 128x32 config finished");
}

void oled_service_clean(void){
	ssd1306_clear_screen(&dev, false);
}

void oled_service_write(char text[], int position, bool invert){
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, position, text, strlen(text), invert);
	vTaskDelay(pdMS_TO_TICKS(100));
}

void oled_service_battery(int bat_lvl){
	char bat_msg[16];
	sprintf(bat_msg, "         %d    ", bat_lvl);
	ssd1306_display_text(&dev, O_PAGE_0, bat_msg, 16, false);

	int bitmapWidth = 3*8;
	int width = ssd1306_get_width(&dev);
	int xpos = width - 18;
	xpos = xpos - bitmapWidth/2;

	if(bat_lvl >= 66){
		ssd1306_bitmaps(&dev, xpos, 0, bat_full, 24, 13, false);
	}else if(bat_lvl > 33 && bat_lvl < 66){
		ssd1306_bitmaps(&dev, xpos, 0, bat_mid, 24, 13, false);
	}else{
		ssd1306_bitmaps(&dev, xpos, 0, bat_low, 24, 13, false);
	}
}

void oled_service_measure(float glucose){
	ssd1306_clear_screen(&dev, false);
	ssd1306_display_text(&dev, O_PAGE_3, "           mg/dL", 16, false);
	vTaskDelay(pdMS_TO_TICKS(100));

	char glu_val[50];
	snprintf(glu_val, sizeof(glu_val),"%d",(int)(glucose + .5));
	ssd1306_display_text_x3(&dev, O_PAGE_1, glu_val, strlen(glu_val), false);
	vTaskDelay(pdMS_TO_TICKS(100));

	int bitmapWidth = 3*8;
	int width = ssd1306_get_width(&dev);
	int xpos = width - 18; // center of width
	xpos = xpos - bitmapWidth/2; 
	ESP_LOGD(TAG, "width=%d xpos=%d", width, xpos);

	ssd1306_bitmaps(&dev, xpos, 0, drop, 24, 13, false);
	vTaskDelay(pdMS_TO_TICKS(100));
}

void oled_service_welcome(void){
	ssd1306_clear_screen(&dev, false);
	ssd1306_display_text(&dev, O_PAGE_1, "    ISE        ", 16, false);
	ssd1306_display_text(&dev, O_PAGE_3, "    ELECTRONICA", 16, false);
	ssd1306_bitmaps(&dev, 0, 12, ise, 24, 13, false);
	vTaskDelay(pdMS_TO_TICKS(3000));
	ssd1306_display_text(&dev, O_PAGE_1, "    KremoWy &  ", 16, false);
	ssd1306_display_text(&dev, O_PAGE_3, "    R4k4210    ", 16, false);
	ssd1306_bitmaps(&dev, 0, 12, ise, 24, 13, false);
	vTaskDelay(pdMS_TO_TICKS(3000));
	ssd1306_clear_screen(&dev, false);
	ssd1306_display_text(&dev, O_PAGE_1, "    GLUCOMETRO ", 16, false);
	ssd1306_display_text(&dev, O_PAGE_3, "    NO INVASIVO", 16, false);
	ssd1306_bitmaps(&dev, 0, 12, ise, 24, 13, false);
	vTaskDelay(pdMS_TO_TICKS(5000));
	ssd1306_clear_screen(&dev, false);
}