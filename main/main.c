#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "dmx.h"
#include "controller.h"
#include "ui.h"

// Init global variable
uint8_t dmx_data[513];
float ui_light = 0.5;
float ui_speed = 0.7;
float ui_offset = 0.0;
float ui_sector_width = 0.1;

static const char *TAG = "Main";

void app_main(void) {
	ESP_LOGI(TAG, "Start FyrKontroll");

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	xTaskCreatePinnedToCore(&controllertask, "controller_task", 4096, NULL, 5, NULL, 0);
	xTaskCreatePinnedToCore(&uitask, "ui_task", 4096, NULL, 5, NULL, 0);
	xTaskCreatePinnedToCore(&dmxtask, "dmx_task", 4096, NULL, 5, NULL, 1);
}

