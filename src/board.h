// GPIO mapping for 2x4 relay mux board

#pragma once

#include <stm32f042x6.h>
#include <stm32f0xx_hal.h>
#include <stdbool.h>

#define CYCLE_BTN_PORT  GPIOF
#define CYCLE_BTN_PIN   GPIO_PIN_0

#define CLEAR_BTN_PORT  GPIOB
#define CLEAR_BTN_PIN   GPIO_PIN_8

#define BTN_DEBOUNCE_TIME_MS 100

#define STATUS_LED_PORT GPIOF
#define STATUS_LED_PIN  GPIO_PIN_1
#define STATUS_LED_ON() STATUS_LED_PORT->BRR = STATUS_LED_PIN
#define STATUS_LED_OFF() STATUS_LED_PORT->BSRR = STATUS_LED_PIN

#define PWR_EN_PORT     GPIOB
#define PWR_EN_PIN      GPIO_PIN_1

#define RELAY_PORT      GPIOA
#define RELAY_PIN_SHIFT 0
#define RELAY_COUNT     8
#define RELAY_MASK      0xFF
#define RELAY_OPERATE_DELAY_MS  10
#define RELAY_RELEASE_DELAY_MS  10

void board_init();

void set_relay_pwr(bool enable);
void close_relays(uint32_t channels);
void open_relays(uint32_t channels);
uint32_t relays_get_state();

#define BTN_CYCLE   0x01
#define BTN_CLEAR   0x02

// Returns button presses exactly once per press-and-release
uint32_t read_buttons();

// Simple logging to memory ringbuffer
void board_log(const char *data);

// Serial number for the device (from STM32 unique ID)
const char *board_serialnumber();

