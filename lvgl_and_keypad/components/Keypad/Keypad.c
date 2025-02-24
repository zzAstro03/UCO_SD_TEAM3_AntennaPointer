#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "Keypad.h"

#define NUM_ROWS 4
#define NUM_COLS 5

static const char *KEYPADTAG = "KEYPAD";

static uint32_t row_gpio[NUM_ROWS] = {18, 8, 9, 17};
static uint32_t col_gpio[NUM_COLS] = {14, 13, 12, 11, 10};

/*  
    Below uses ASCII to print on LVGL and maps to

    7 8 9    NA    Up
    4 5 6    NA    Down
    1 2 3    NA    Left
    . 0 Back Enter Right

*/
static const uint32_t keys[NUM_ROWS][NUM_COLS] = {
    {55, 56, 57, 0, 17},
    {52, 53, 54, 0, 18},
    {49, 50, 51, 0, 20},
    {8, 48, 46, 10, 19}
};

gpio_config_t config_pin = {
    .pin_bit_mask = 0,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

void keypad_init(void){
    ESP_LOGI(KEYPADTAG, "Initializing Keypad GPIO");

    for(int i = 0; i < NUM_COLS; i++){
        config_pin.pin_bit_mask = (1ULL << col_gpio[i]);
        config_pin.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&config_pin);
    }

    for(int i = 0; i < NUM_ROWS; i++){
        config_pin.pin_bit_mask = (1ULL << row_gpio[i]);
        config_pin.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&config_pin);
    }
}

uint32_t keypad_get_key(void){
    esp_log_level_set("gpio", ESP_LOG_ERROR);

    for(int i = 0; i < NUM_ROWS; i++){
        config_pin.pin_bit_mask = (1ULL << row_gpio[i]);
        config_pin.mode = GPIO_MODE_OUTPUT;
        gpio_config(&config_pin);
        gpio_set_level(row_gpio[i], 0);

        for(int j = 0; j < NUM_COLS; j++){
            int8_t level = gpio_get_level(col_gpio[j]);

            if(level == 0){
                config_pin.mode = GPIO_MODE_INPUT;
                gpio_config(&config_pin);
                return keys[i][j];
            }
        }
        config_pin.mode = GPIO_MODE_INPUT;
        gpio_config(&config_pin);
    }

    return 0;
}