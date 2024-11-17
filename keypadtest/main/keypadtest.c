#include "esp_log.h"
#include "iot_button.h"
#include <inttypes.h>
static const char *TAG = "BUTTON TEST";

#define BUTTON_NUM 16
static button_handle_t g_btns[BUTTON_NUM] = {0};

#define NUMBER_OF_ROWS 4
#define NUMBER_OF_COLUMNS 4

#define LONG_PRESS_DURATION_MS 1500
    #define SHORT_PRESS_DURATION_MS 500


const char* keys[NUMBER_OF_ROWS][NUMBER_OF_COLUMNS] = {
    {"1", "2", "3", "A"},
    {"4", "5", "6", "B"},
    {"7", "8", "9", "C"},
    {"*", "0", "#", "D"}
};

static const char* get_btn_index(button_handle_t btn)
{
    for (size_t i = 0; i < BUTTON_NUM; i++) {
        if (btn == g_btns[i]) {
            // Calculate row and column from the linear index
            size_t row = i / NUMBER_OF_ROWS; // Integer division to get the row
            size_t col = i % NUMBER_OF_COLUMNS; // Modulo operation to get the column
            return keys[row][col];
        }
    }
    return NULL; // Return NULL if the button is not found
}

static void button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_PRESS_DOWN", get_btn_index((button_handle_t)arg));
}

static void button_press_up_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_PRESS_UP[%d]", get_btn_index((button_handle_t)arg), (int) iot_button_get_ticks_time((button_handle_t)arg));
}

static void button_press_repeat_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_PRESS_REPEAT[%d]", get_btn_index((button_handle_t)arg), (int) iot_button_get_repeat((button_handle_t)arg));
}

static void button_single_click_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
}

static void button_double_click_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_DOUBLE_CLICK", get_btn_index((button_handle_t)arg));
}

static void button_long_press_start_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_LONG_PRESS_START", get_btn_index((button_handle_t)arg));
}

static void button_long_press_hold_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_LONG_PRESS_HOLD[%d],count is [%d]", get_btn_index((button_handle_t)arg), (int) iot_button_get_ticks_time((button_handle_t)arg), (int) iot_button_get_long_press_hold_cnt((button_handle_t)arg));
}

static void button_press_repeat_done_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%s: BUTTON_PRESS_REPEAT_DONE[%d]", get_btn_index((button_handle_t)arg), (int) iot_button_get_repeat((button_handle_t)arg));
}

void initialize_keypad() {
    // Define the GPIO pins for the keypad rows and columns
    int32_t row_gpio[NUMBER_OF_ROWS] = {35, 0, 45, 48}; // Example GPIOs for rows
    int32_t col_gpio[NUMBER_OF_COLUMNS] = {47, 21, 20, 19}; // Example GPIOs for columns

    button_config_t cfg = {
        .type = BUTTON_TYPE_MATRIX,
        .long_press_time = LONG_PRESS_DURATION_MS,
        .short_press_time = SHORT_PRESS_DURATION_MS,
        .matrix_button_config = {
            .row_gpio_num = 0,
            .col_gpio_num = 0,
        }
    };

    for (int i = 0; i < NUMBER_OF_ROWS; i++) {
        cfg.matrix_button_config.row_gpio_num = row_gpio[i];
        for (int j = 0; j < NUMBER_OF_COLUMNS; j++) {
            cfg.matrix_button_config.col_gpio_num = col_gpio[j];
            g_btns[i * NUMBER_OF_ROWS + j] = iot_button_create(&cfg);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_PRESS_DOWN, button_press_down_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_PRESS_UP, button_press_up_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_PRESS_REPEAT, button_press_repeat_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_LONG_PRESS_START, button_long_press_start_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_LONG_PRESS_HOLD, button_long_press_hold_cb, NULL);
            iot_button_register_cb(g_btns[i * NUMBER_OF_ROWS + j], BUTTON_PRESS_REPEAT_DONE, button_press_repeat_done_cb, NULL);
        }
    }
}

void app_main(void) {
    initialize_keypad();
}