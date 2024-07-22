#include "ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/adc.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

static const char *TAG = "UI";

// Pins for potentiometers
#define POT1 ADC1_CHANNEL_6  // IO34
#define POT2 ADC1_CHANNEL_7  // IO35
#define POT_MAX 4095  // 12bit ADC

// Pins for Encoder switch
#define GPIO_SW1 14
#define GPIO_SW2 12
#define GPIO_SW4 2
#define GPIO_SW8 4
#define GPIO_PIN_SEL  ((1ULL<<GPIO_SW1) | (1ULL<<GPIO_SW2) | (1ULL<<GPIO_SW4) | (1ULL<<GPIO_SW8))

// Pins for leds
#define LED_LEFT_PIN 23
#define LED_RIGHT_PIN 19

enum SWITCHMODE {
	MODE_NONE,
	MODE_LIGHT,
	MODE_SPEED,
	MODE_OFFSET,
	MODE_SECTOR_WIDTH
};

bool potAndValueMatches(float pot, float value) {
	float diff = pot - value;
	// If set value is within a percentage validate as there
	if (fabs(diff) < 0.01) {
		// Lit both leds to show it matches
		gpio_set_level(LED_LEFT_PIN, 1);
		gpio_set_level(LED_RIGHT_PIN, 1);
		return true;
	} else {
		// Get which direction value is showing and lit left or right led
		if (diff > 0) {
			gpio_set_level(LED_LEFT_PIN, 1);
			gpio_set_level(LED_RIGHT_PIN, 0);
		} else {
			gpio_set_level(LED_LEFT_PIN, 0);
			gpio_set_level(LED_RIGHT_PIN, 1);
		}
		return false;
	}
}

void uitask(void *pvParameters) {
	ESP_LOGI(TAG, "Start");

	ESP_LOGI(TAG, "Configure ADC1");
	adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
	adc1_config_channel_atten(POT1, ADC_ATTEN_DB_11);
	adc1_config_channel_atten(POT2, ADC_ATTEN_DB_11);

	ESP_LOGI(TAG, "Configure GPIO for encoder switch");
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	ESP_LOGI(TAG, "Configure GPIO for led output");
	gpio_reset_pin(LED_LEFT_PIN);  // Set pad driver capability to GPIO
	gpio_set_direction(LED_LEFT_PIN, GPIO_MODE_OUTPUT);  // Set the GPIO as a push/pull output
	gpio_set_level(LED_LEFT_PIN, 0);
	gpio_reset_pin(LED_RIGHT_PIN);  // Set pad driver capability to GPIO
	gpio_set_direction(LED_RIGHT_PIN, GPIO_MODE_OUTPUT);  // Set the GPIO as a push/pull output
	gpio_set_level(LED_RIGHT_PIN, 0);

	int pot1_raw = 0;
	float pot1_value = 0;
	unsigned int sw_mode_light = 0;
	unsigned int sw_mode_speed = 0;
	unsigned int sw_mode_offset = 0;
	unsigned int sw_mode_sector_width = 0;
	enum SWITCHMODE mode = MODE_NONE;
	enum SWITCHMODE mode_previous = MODE_NONE;
	bool mode_aligned = false;
	while(1) {
		// Get input
		pot1_raw = adc1_get_raw(POT1);
		pot1_value = ((float)pot1_raw) / POT_MAX;
		// ESP_LOGI(TAG, "Pot 1: %d %f", pot1_raw, pot1_value);
		sw_mode_light = gpio_get_level(GPIO_SW1);
		sw_mode_speed = gpio_get_level(GPIO_SW2);
		sw_mode_offset = gpio_get_level(GPIO_SW4);
		sw_mode_sector_width = gpio_get_level(GPIO_SW8);
		// ESP_LOGI(TAG, "Light: %d Speed: %d Offset: %d Width: %d", sw_mode_light, sw_mode_speed, sw_mode_offset, sw_mode_sector_width);

		// Check mode and level
		if (sw_mode_light) {
			mode = MODE_LIGHT;
		} else if (sw_mode_speed) {
			mode = MODE_SPEED;
		} else if (sw_mode_offset) {
			mode = MODE_OFFSET;
		} else if (sw_mode_sector_width) {
			mode = MODE_SECTOR_WIDTH;
		} else {
			mode = MODE_NONE;
		}
		if (mode != mode_previous) {
			ESP_LOGI(TAG, "Mode changed to %d", mode);
			mode_previous = mode;
			mode_aligned = false;
		}

		// Check level
		switch (mode) {
			case MODE_LIGHT:
				// ESP_LOGI(TAG, "Pot 1: %f Light: %f", pot1_value, ui_light);
				if (mode_aligned) {
					ui_light = pot1_value;
				} else {
					if (potAndValueMatches(pot1_value, ui_light)) {
						mode_aligned = true;
					}
				}
				break;
			case MODE_SPEED:
				// ESP_LOGI(TAG, "Pot 1: %f Speed: %f", pot1_value, ui_speed);
				if (mode_aligned) {
					ui_speed = pot1_value;
				} else {
					if (potAndValueMatches(pot1_value, ui_speed)) {
						mode_aligned = true;
					}
				}
				break;
			case MODE_OFFSET:
				// ESP_LOGI(TAG, "Pot 1: %f Offset: %f", pot1_value, ui_offset);
				if (mode_aligned) {
					ui_offset = pot1_value;
				} else {
					if (potAndValueMatches(pot1_value, ui_offset)) {
						mode_aligned = true;
					}
				}
				break;
			case MODE_SECTOR_WIDTH:
				// ESP_LOGI(TAG, "Pot 1: %f Sector Width: %f", pot1_value, ui_sector_width);
				if (mode_aligned) {
					ui_sector_width = pot1_value;
				} else {
					if (potAndValueMatches(pot1_value, ui_sector_width)) {
						mode_aligned = true;
					}
				}
				break;
			case MODE_NONE:
				ESP_LOGI(TAG, "Mode None should not be able to exist.");
				gpio_set_level(LED_LEFT_PIN, 0);
				gpio_set_level(LED_RIGHT_PIN, 0);
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}

