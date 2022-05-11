#include "oled_service.h"

static const char *TAG = "OLED";

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

void oled_service_fade_out(void){
	ssd1306_fadeout(&dev);
}


/*
// Display Count Down
	uint8_t image[24];
	memset(image, 0, sizeof(image));
	ssd1306_display_image(&dev, top, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&dev, top+1, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&dev, top+2, (6*8-1), image, sizeof(image));
	for(int font=0x39;font>0x30;font--) {
		memset(image, 0, sizeof(image));
		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
		memcpy(image, font8x8_basic_tr[font], 8);
		if (dev._flip) ssd1306_flip(image, 8);
		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}


	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
  	ssd1306_display_text_x3(&dev, 0, "Hello World", 11, false);
	vTaskDelay(pdMS_TO_TICKS(3000));

	ssd1306_display_text(&dev, 0, "SSD1306 128x32", 14, false);
	ssd1306_display_text(&dev, 1, "Hello World!!", 13, false);
	ssd1306_display_text(&dev, 2, "SSD1306 128x32", 14, true);
	ssd1306_display_text(&dev, 3, "Hello World!!", 13, true);
	vTaskDelay(pdMS_TO_TICKS(3000));
*/