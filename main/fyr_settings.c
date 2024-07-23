#include "fyr_settings.h"

#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "Settings";

#define STORE_UINT32_FLOAT_SCALING_FACTOR 1000
int32_t StoreFloatToInt32(float value) {
	return (uint32_t)(value * STORE_UINT32_FLOAT_SCALING_FACTOR);
}

float StoreUint32ToFloat(uint32_t value) {
	return ((float)value) / STORE_UINT32_FLOAT_SCALING_FACTOR;
}

bool ValidateFloatInrangeZeroToOne(float value) {
	if (value >= 0.0 && value <= 1.0) {
		return true;
	} else {
		return false;
	}
}

void ValidateAndSantizeSettings() {
	if (ValidateFloatInrangeZeroToOne(fyrsettings.light) == false) {
		ESP_LOGE(TAG, "Setting Validate light %f should be between 0 and 1.", fyrsettings.light);
		fyrsettings.light = 0.0;
		ESP_LOGI(TAG, "Reset parameter to %f", fyrsettings.light);
	}
	if (ValidateFloatInrangeZeroToOne(fyrsettings.speed) == false) {
		ESP_LOGE(TAG, "Setting Validate speed %f should be between 0 and 1.", fyrsettings.speed);
		fyrsettings.speed = 0.0;
		ESP_LOGI(TAG, "Reset parameter to %f", fyrsettings.speed);
	}
	if (ValidateFloatInrangeZeroToOne(fyrsettings.offset) == false) {
		ESP_LOGE(TAG, "Setting Validate offset %f should be between 0 and 1.", fyrsettings.offset);
		fyrsettings.offset = 0.0;
		ESP_LOGI(TAG, "Reset parameter to %f", fyrsettings.offset);
	}
	if (ValidateFloatInrangeZeroToOne(fyrsettings.sector_width) == false) {
		ESP_LOGE(TAG, "Setting Validate sector_width %f should be between 0 and 1.", fyrsettings.sector_width);
		fyrsettings.sector_width = 0.0;
		ESP_LOGI(TAG, "Reset parameter to %f", fyrsettings.sector_width);
	}
}

void LoadSettings() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	nvs_handle load_handle;
	err = nvs_open("storage", NVS_READWRITE, &load_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error %s opening NVS handle", esp_err_to_name(err));
	} else {
		int32_t temp = 0;
		err = nvs_get_i32(load_handle, "light", &temp);
		switch (err) {
			case ESP_OK:
				fyrsettings.light = StoreUint32ToFloat(temp);
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				ESP_LOGI(TAG, "The light is not initialized yet %d", err);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading light", esp_err_to_name(err));
		}
		err = nvs_get_i32(load_handle, "speed", &temp);
		switch (err) {
			case ESP_OK:
				fyrsettings.speed = StoreUint32ToFloat(temp);
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				ESP_LOGI(TAG, "The speed is not initialized yet %d", err);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading speed", esp_err_to_name(err));
		}
		err = nvs_get_i32(load_handle, "offset", &temp);
		switch (err) {
			case ESP_OK:
				fyrsettings.offset = StoreUint32ToFloat(temp);
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				ESP_LOGI(TAG, "The offset is not initialized yet %d", err);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading offset", esp_err_to_name(err));
		}
		err = nvs_get_i32(load_handle, "sector_width", &temp);
		switch (err) {
			case ESP_OK:
				fyrsettings.sector_width = StoreUint32ToFloat(temp);
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				ESP_LOGI(TAG, "The sector_width is not initialized yet %d", err);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading sector_width", esp_err_to_name(err));
		}
	}

	nvs_close(load_handle);
	ValidateAndSantizeSettings();
}

void SaveSettings() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	nvs_handle save_handle;
	err = nvs_open("storage", NVS_READWRITE, &save_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error %s opening NVS handle", esp_err_to_name(err));
	} else {
		int32_t temp = 0;
		temp = StoreFloatToInt32(fyrsettings.light);
		err = nvs_set_i32(save_handle, "light", temp);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving light", esp_err_to_name(err));
		}
		temp = StoreFloatToInt32(fyrsettings.speed);
		err = nvs_set_i32(save_handle, "speed", temp);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving speed", esp_err_to_name(err));
		}
		temp = StoreFloatToInt32(fyrsettings.offset);
		err = nvs_set_i32(save_handle, "offset", temp);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving offset", esp_err_to_name(err));
		}
		temp = StoreFloatToInt32(fyrsettings.sector_width);
		err = nvs_set_i32(save_handle, "sector_width", temp);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving sector_width", esp_err_to_name(err));
		}

		err = nvs_commit(save_handle);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s committing", esp_err_to_name(err));
		}
	}

	nvs_close(save_handle);
}

