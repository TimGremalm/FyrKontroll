#include "dmx.h"

#include <stdint.h>
#include <strings.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/uart.h"

/*
ESP-PICO-KIT ESP32 PICO D4
ESP32 UART https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html

Pin		Description
IO22	Receive
IO19	Receive Enable	Low=Enable, High=Disable
IO23	Send Enable		High=Enable, Low=Disable
IO21	Send
*/
#define PIN_RECEIVE 22
#define PIN_SEND 21

#define UART_DMX UART_NUM_2
#define DMX_SIZE 513


static const char *TAG = "DMX";
const char nullChar = 0;

void dmxtask(void *pvParameters) {
	ESP_LOGI(TAG, "Start");

	// DMX inspired by https://github.com/Hardi-St/MobaLedLib/blob/master/src/DMXInterface.cpp
	// Setup UART for DMX512
	uart_config_t uart_config = {
		.baud_rate = 250000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_2,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122
	};
	uart_param_config(UART_DMX, &uart_config);

	// Set pins for UART
	uart_set_pin(UART_DMX, PIN_SEND, PIN_RECEIVE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	// Install queue
	uart_driver_install(UART_DMX, UART_FIFO_LEN+1, UART_FIFO_LEN+DMX_SIZE, 0, NULL, 0);

	// Set start bit
	dmx_data[0] = 0;

	// ESP_LOGI(TAG, "Start receive DMX512 on UART %d IO %d", UART_DMX, PIN_RECEIVE);
	ESP_LOGI(TAG, "Start send DMX512 on UART %d IO %d", UART_DMX, PIN_SEND);
	while(1) {
		/*
		https://github.com/Hardi-St/MobaLedLib/blob/master/src/DMXInterface.cpp
		Temporarily change baudrate to lower and send a 0 byte
		DMX needs a 88us low (break) follow by 12us high
		we set the protocol to 8 bit even parity 1 stop -> together with start bit
		it gives a total of 10 low bits when sending the '0' byte
		so one bit must be (88 / 10) us long
		*/

		uart_set_baudrate(UART_DMX, ((1000000*10)/88));
		uart_set_parity(UART_DMX, UART_PARITY_EVEN);
		uart_write_bytes(UART_DMX, &nullChar, 1);

		esp_rom_delay_us(88+10);  // 88us break + some time to be sure that byte is fully sent

		// now we need 12us mark, don't delay here, because the baud rate switch takes some time anyway

		// switching the baudrate costs ~ 17us
		uart_set_baudrate(UART_DMX, 250000);
		uart_set_parity(UART_DMX, UART_PARITY_DISABLE);

		uart_write_bytes(UART_DMX, (const uint8_t *) dmx_data, DMX_SIZE);

		vTaskDelay(30 / portTICK_PERIOD_MS);
	}
}

