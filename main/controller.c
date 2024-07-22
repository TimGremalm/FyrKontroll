#include "controller.h"

#include <stdint.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "dmx.h"
#include "ui.h"

#define Fixture_ChannelStart 1

/*
Fyr
01 Dimmer
02 Red
03 Green
04 Blue
05 White
06 Pan
07 Pan Fine
08 Speed Min (Ex. 204)
09 Speed Max (Ex. 127)
*/

/*
Fixture Eurolite LED PARty RGBW Spot
01 Red
02 Green
03 Blue
04 White
05 Dimmer
06 Control
*/

/*
NoName Moving Head 11ch
01 Pan
02 Pan Fine
03 Tilt
04 Tilt Fine
05 Color
06 Gobo
07 Strobe
08 Dimming
09 Rotation Speed
10 Auto-Play
11 Reposition
*/

#define Channel_Red 2
#define Channel_Green 3
#define Channel_Blue 4
#define Channel_White 5
#define Channel_Dimmer 1
#define Channel_Pan 6
#define Channel_Pan_Fine 7
#define Channel_Speed_Min 8
#define Channel_Speed_Max 9

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static const char *TAG = "Controller";

void setChannels(uint8_t channel, uint8_t value) {
	uint8_t ch = 0;
	ch = Fixture_ChannelStart + (channel - 1);
	// ESP_LOGI(TAG, "Channel: %d Value: %d", ch, value);
	dmx_data[ch] = value;
}

typedef struct {
	uint32_t seconds;
	float max;
} wave_t;

void controllertask(void *pvParameters) {
	ESP_LOGI(TAG, "Start");

	// Dimmers
	float level_dim = 0.1;
	float level_color = 0.5;

	// Colors
	float red = 0.50;
	float green = 0.12;
	float blue = 0.00;
	float white = 0.00;

	// Rotation
	float pan = 0.00;
	float speed_min = 0.80;
	float speed_max = 0.50;

	// Draw parameters
	float center = 0.0;
	float offset = 0.95;
	float width = 0.2;
	float rotate_time = 5.7;
	float rotate_time_min = 1.0;
	float rotate_time_max = 20.0;
	float step_time = 0.020;
	float now = 0.0;

	while(1) {
		// Update variables
		level_dim = ui_light;
		level_color = ui_light;
		offset = ui_offset;
		width = mapf(ui_sector_width, 0.0, 1.0, 0.05, 0.75);
		float speed = mapf(ui_speed, 0.0, 1.0, -1.0, 1.0);
		float speed_abs = fabs(speed);
		float speed_abs_inv = 1.0 - speed_abs;
		float rotate_time_delta = rotate_time_max - rotate_time_min;
		rotate_time = (rotate_time_delta * speed_abs_inv) + rotate_time_min;

		// Draw
		float steps = rotate_time / step_time;
		float step_increment = 1.0 / steps;
		if (speed < 0) {
			step_increment = step_increment * -1.0;
		}
		if (speed_abs < 0.05) {
			step_increment = 0;
		}
		// ESP_LOGI(TAG, "rotate time %f inc %f", rotate_time, step_increment);
		now += step_increment;
		// ESP_LOGI(TAG, "%f fmod = %f", now, newnow);
		now = fmod(now + 1.0, 1.0);
		if (now < 0.0) {
			now = 0;
		}
		// ESP_LOGI(TAG, "now %f", now);
		// Add position + center of sector + cllibration offset. Modula by 1.
		pan = fmod((now + center + offset), 1.0);
		float outer_width = (1.0 - width) / 2.0;
		if (now >= 0 && now < outer_width) {
			// Before
			red = 0.0;
			green = 1.0;
			blue = 0.0;
			white = 0.0;
		} else if (now >= outer_width && now < (outer_width+width)) {
			// In Sector
			red = 0.5;
			green = 0.5;
			blue = 0.2;
			white = 1.0;
		} else {
			// After
			red = 1.0;
			green = 0.0;
			blue = 0.0;
			white = 0.0;
		}

		// Convert float 0-1 values to 8-bit levels, also adjust for individual colors.
		setChannels(Channel_Dimmer, (uint8_t)(level_dim * 255));
		setChannels(Channel_Red, (uint8_t)(level_color * 255 * red));
		setChannels(Channel_Green, (uint8_t)(level_color * 255 * green));
		setChannels(Channel_Blue, (uint8_t)(level_color * 255 * blue));
		setChannels(Channel_White, (uint8_t)(level_color * 255 * white));
		setChannels(Channel_Pan, (uint8_t)(pan * 255));
		setChannels(Channel_Speed_Min, (uint8_t)(speed_min * 255));
		setChannels(Channel_Speed_Max, (uint8_t)(speed_max * 255));

		uint32_t loop_sleep_ms = (uint32_t)(step_time * 1000);
		vTaskDelay(loop_sleep_ms / portTICK_PERIOD_MS);
	}
}

