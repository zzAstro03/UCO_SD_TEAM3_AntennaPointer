#include <stdio.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_err.h>
#include <esp_freertos_hooks.h>
#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <lv_conf.h>

#include "sdkconfig.h"
#include "Keypad.h"
#include "esp_lcd_ili9488.h"

static const char *TAG = "main";
static const char *LVGLTAG = "LVGL";
static const char *SCREENTAG = "SCREEN";
static const char *KEYPADTAG = "KEYPAD";

static const int DISPLAY_HORIZONTAL_PIXELS = 480;
static const int DISPLAY_VERTICAL_PIXELS = 320;
static const int DISPLAY_COMMAND_BITS = 8;
static const int DISPLAY_PARAMETER_BITS = 8;
static const unsigned int DISPLAY_REFRESH_HZ = 40000000;
static const int DISPLAY_SPI_QUEUE_LEN = 10;
static const int SPI_MAX_TRANSFER_SIZE = 32768;

// Default to 25 lines of color data
static const size_t LV_BUFFER_SIZE = DISPLAY_HORIZONTAL_PIXELS * DISPLAY_VERTICAL_PIXELS / 10;
//static const size_t LV_BUFFER_SIZE = DISPLAY_HORIZONTAL_PIXELS * 25;
static const int LVGL_UPDATE_PERIOD_MS = 5;

static const ledc_mode_t BACKLIGHT_LEDC_MODE = LEDC_LOW_SPEED_MODE;
static const ledc_channel_t BACKLIGHT_LEDC_CHANNEL = LEDC_CHANNEL_0;
static const ledc_timer_t BACKLIGHT_LEDC_TIMER = LEDC_TIMER_1;
static const ledc_timer_bit_t BACKLIGHT_LEDC_TIMER_RESOLUTION = LEDC_TIMER_10_BIT;
static const uint32_t BACKLIGHT_LEDC_FRQUENCY = 5000;

static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;
static lv_display_t * lv_display = NULL;
static lv_color_t * lv_buf_1 = NULL;
static lv_color_t * lv_buf_2 = NULL;

static lv_obj_t * input_screen = NULL;
static lv_obj_t * output_screen = NULL;
static lv_indev_t * indev_keypad;
static lv_obj_t * Back_button;
lv_obj_t * Enter_button;

bool screen_state = true;

static void lv_indev_keypad_read_cb(lv_indev_t * indev, lv_indev_data_t * data){
    uint32_t act_key = keypad_get_key();
    data->key = act_key;

    if(act_key != 0){
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lv_indev_keypad_init(void){
    ESP_LOGI(KEYPADTAG, "Creating LVGL INDEV Keypad");

    keypad_init();

    indev_keypad = lv_indev_create();
    lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_keypad, lv_indev_keypad_read_cb);

}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx){
    lv_display_t *disp_driver = (lv_display_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

void lvgl_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * color_map){
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(display);

    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

    lv_display_flush_ready(display);
}

static void IRAM_ATTR lvgl_tick_cb(void *param)
{
    lv_tick_inc(LVGL_UPDATE_PERIOD_MS);
}

static void display_brightness_init(void){
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = (gpio_num_t)CONFIG_TFT_BACKLIGHT_PIN,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = 
        {
            .output_invert = 0
        }
    };

    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .duty_resolution = BACKLIGHT_LEDC_TIMER_RESOLUTION,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = BACKLIGHT_LEDC_FRQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_LOGI(TAG, "Initializing LEDC for backlight pin: %d", CONFIG_TFT_BACKLIGHT_PIN);
    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
}

void display_brightness_set(int brightness_percentage){
    if (brightness_percentage > 100){
        brightness_percentage = 100;
    } else if (brightness_percentage < 0){
        brightness_percentage = 0;
    }

    ESP_LOGI(TAG, "Setting backlight to %d%%", brightness_percentage);
    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * brightness_percentage) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL));
}

void initialize_spi(){
    ESP_LOGI(TAG, "Initializing SPI bus (MOSI:%d, MISO:%d, CLK:%d)", CONFIG_SPI_MOSI, CONFIG_SPI_MISO, CONFIG_SPI_CLOCK);
    spi_bus_config_t bus = {
        .mosi_io_num = CONFIG_SPI_MOSI,
        .miso_io_num = CONFIG_SPI_MISO,
        .sclk_io_num = CONFIG_SPI_CLOCK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO |
                 SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
}

void initialize_display(){
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = CONFIG_TFT_CS_PIN,
        .dc_gpio_num = CONFIG_TFT_DC_PIN,
        .pclk_hz = DISPLAY_REFRESH_HZ,
        .spi_mode = 0,
        .lcd_cmd_bits = DISPLAY_COMMAND_BITS,
        .lcd_param_bits = DISPLAY_PARAMETER_BITS,
        .trans_queue_depth = DISPLAY_SPI_QUEUE_LEN,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &lv_display,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode = 0,
            .sio_mode = 0,
            .lsb_first = 0,
            .cs_high_active = 0
        }
    };

    const esp_lcd_panel_dev_config_t lcd_config = {
        .reset_gpio_num = CONFIG_TFT_RESET_PIN,
        .color_space = CONFIG_DISPLAY_COLOR_MODE,
        .bits_per_pixel = 18,
        .flags = {
            .reset_active_high = 0
        },
        .vendor_config = NULL
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &lcd_io_handle)); 

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &lcd_config, LV_BUFFER_SIZE, &lcd_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, false));

    /*
        These two lines will adjust orientation: 
        true, false, true works for current device
        If other orientations are desired, must change
        DISPLAY_HORIZONTAL_PIXELS AND DISPLAY_VERTICAL_PIXELS
        if necessary at top of file    
    */

    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, false, true));

    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));
}

void initialize_lvgl(){
    ESP_LOGI(LVGLTAG, "Initializing LVGL");
    lv_init();

    ESP_LOGI(LVGLTAG, "Initializing %dx%d display", DISPLAY_HORIZONTAL_PIXELS, DISPLAY_VERTICAL_PIXELS);
    lv_display = lv_display_create(DISPLAY_HORIZONTAL_PIXELS, DISPLAY_VERTICAL_PIXELS);

    ESP_LOGI(LVGLTAG, "Allocating %zu bytes for first LVGL buffer", LV_BUFFER_SIZE * sizeof(lv_color_t));
    lv_buf_1 = (lv_color_t *)heap_caps_malloc(LV_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    ESP_LOGI(TAG, "Allocating %zu bytes for second LVGL buffer", LV_BUFFER_SIZE * sizeof(lv_color_t));
    lv_buf_2 = (lv_color_t *)heap_caps_malloc(LV_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    ESP_LOGI(LVGLTAG, "Creating LVGL display buffer");
    lv_display_set_buffers(lv_display, lv_buf_1, lv_buf_2, LV_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(LVGLTAG, "Creating LVGL flush callback");
    lv_display_set_user_data(lv_display, lcd_handle);
    lv_display_set_flush_cb(lv_display, lvgl_flush_cb);

    ESP_LOGI(LVGLTAG, "Creating LVGL tick timer");

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_UPDATE_PERIOD_MS * 1000));

    ESP_LOGI(LVGLTAG, "LVGL initialization complete!");
}


/*
    Event callbacks for LVGL
*/

/*  I want to be able to format date and also make sure it is a valid date in the future
     Currently does not work, issue with backspace...
static void format_date(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_target(e);
    const char * txt = lv_textarea_get_text(ta);
    if(txt[0] >= '0' && txt[0] <= '9' &&
       txt[1] >= '0' && txt[1] <= '9' &&
       txt[2] != '/') {
        lv_textarea_set_cursor_pos(ta, 2);
        lv_textarea_add_char(ta, '/');

    } else if(txt[0] >= '0' && txt[0] <= '9' &&
        txt[1] >= '0' && txt[1] <= '9' &&
        //txt[2] == '/' &&
        txt[3] >= '0' && txt[3] <= '9' &&
        txt[4] >= '0' && txt[4] <= '9' &&
        txt[5] != '/'){
         lv_textarea_set_cursor_pos(ta, 5);
         lv_textarea_add_char(ta, '/');
    }
}
*/
static void switch_screen(lv_event_t * e){
    if(screen_state){
        ESP_LOGI(SCREENTAG, "Switching to output screen.");
        lv_screen_load(output_screen);
        lv_group_focus_obj(Back_button);
        screen_state = false;
    } else {
        ESP_LOGI(SCREENTAG, "Switching to input screen.");
        lv_screen_load(input_screen);
        lv_group_focus_obj(input_screen);
        screen_state = true;
    }
}

void initialize_screens(void){
    ESP_LOGI(SCREENTAG, "Initializing screens and creating widgets...");

    /* Group for navigation and inputs
    */

    lv_group_t * input_group = lv_group_create();
    lv_group_set_default(input_group);
    lv_indev_set_group(indev_keypad, input_group);

    /*
    
        INPUT SCREEN, LABELS, AND WIDGETS
    
    */

    input_screen = lv_obj_create(NULL);
    lv_screen_load(input_screen);
    lv_gridnav_add(input_screen, LV_GRIDNAV_CTRL_ROLLOVER);
    lv_group_add_obj(input_group, input_screen);

    /* Inputs label
    */
    lv_obj_t * input_label = lv_label_create(input_screen);
    lv_label_set_text(input_label, "Inputs:");
    lv_obj_align(input_label, LV_ALIGN_TOP_MID, 0, 10);

    /* Latitude input box 
    */
    lv_obj_t * Lat_ta = lv_textarea_create(input_screen);
    lv_obj_set_width(Lat_ta, lv_pct(40));
    lv_obj_align(Lat_ta, LV_ALIGN_LEFT_MID, 10, -40);
    lv_textarea_set_one_line(Lat_ta, true);
    lv_textarea_set_max_length(Lat_ta, 10);
    lv_textarea_set_placeholder_text(Lat_ta, "Decimal Deg. (N)");
    lv_group_remove_obj(Lat_ta);
    
    /* Create Latitude label
    */
    lv_obj_t * Lat_label = lv_label_create(input_screen);
    lv_label_set_text(Lat_label, "Latitude (N)");
    lv_obj_align_to(Lat_label, Lat_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);

    /* Longitude input box 
    */
    lv_obj_t * Long_ta = lv_textarea_create(input_screen);
    lv_obj_set_width(Long_ta, lv_pct(40));
    lv_obj_align(Long_ta, LV_ALIGN_RIGHT_MID, -10, -40);
    lv_textarea_set_one_line(Long_ta, true);
    lv_textarea_set_max_length(Long_ta, 10);
    lv_textarea_set_placeholder_text(Long_ta, "Decimal Deg. (W)");
    lv_group_remove_obj(Long_ta);

    /* Longitude label
    */
    lv_obj_t * long_label = lv_label_create(input_screen);
    lv_label_set_text(long_label, "Longitude (W)");
    lv_obj_align_to(long_label, Long_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);


    /* AntennaOffset input box
    */
    lv_obj_t * AntennaOfs_ta = lv_textarea_create(input_screen);
    lv_obj_set_width(AntennaOfs_ta, lv_pct(40));
    lv_obj_align(AntennaOfs_ta, LV_ALIGN_LEFT_MID, 10, 40);
    lv_textarea_set_one_line(AntennaOfs_ta, true);
    lv_textarea_set_max_length(AntennaOfs_ta, 6);
    lv_textarea_set_placeholder_text(AntennaOfs_ta, "Degrees");
    lv_group_remove_obj(AntennaOfs_ta);

    /* AntennaOffset label 
    */
    lv_obj_t * AntennaOfs_label = lv_label_create(input_screen);
    lv_label_set_text(AntennaOfs_label, "Antenna Offset:");
    lv_obj_align_to(AntennaOfs_label, AntennaOfs_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);

    /* Date input box 
    */
    lv_obj_t * Date_ta = lv_textarea_create(input_screen);
    lv_obj_set_width(Date_ta, lv_pct(40));
    lv_obj_align(Date_ta, LV_ALIGN_RIGHT_MID, -10, 40);
    lv_textarea_set_one_line(Date_ta, true);
    lv_textarea_set_max_length(Date_ta, 8);
    lv_textarea_set_accepted_chars(Date_ta, "0123456789");
    lv_textarea_set_placeholder_text(Date_ta, "MMDDYYYY");
    lv_group_remove_obj(Date_ta);
    //lv_obj_add_event_cb(Date_ta, format_date, LV_EVENT_VALUE_CHANGED, NULL);

    /* Date label 
    */
    lv_obj_t * Date_label = lv_label_create(input_screen);
    lv_label_set_text(Date_label, "Date:");
    lv_obj_align_to(Date_label, Date_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);


    /* Enter button
    */
    Enter_button = lv_button_create(input_screen);
    lv_obj_align(Enter_button, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_group_remove_obj(Enter_button);
    lv_obj_add_event_cb(Enter_button, switch_screen, LV_EVENT_PRESSED, NULL);

    /* Enter label
    */
    lv_obj_t * Enter_label = lv_label_create(Enter_button);
    lv_label_set_text(Enter_label, "Enter");
    lv_obj_center(Enter_label);

    /*

        OUTPUT SCREEN, LABELS, AND WIDGETS

    */

    output_screen = lv_obj_create(NULL);
    //lv_group_add_obj(input_group, output_screen);

    /* Outputs label 
    */
    lv_obj_t * output_label = lv_label_create(output_screen);
    lv_label_set_text(output_label, "Outputs:");
    lv_obj_align(output_label, LV_ALIGN_TOP_MID, 0, 10);

    /* Azimuth output box 
    */
    lv_obj_t * Azimuth_ta = lv_textarea_create(output_screen);
    lv_obj_set_width(Azimuth_ta, lv_pct(40));
    lv_obj_align(Azimuth_ta, LV_ALIGN_LEFT_MID, 10, 0);
    lv_textarea_set_one_line(Azimuth_ta, true);
    lv_obj_remove_flag(Azimuth_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Azimuth label
    */
    lv_obj_t * Azimuth_label = lv_label_create(output_screen);
    lv_label_set_text(Azimuth_label, "Azimuth:");
    lv_obj_align_to(Azimuth_label, Azimuth_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);

    /* Elevation output box
    */
    lv_obj_t * Elevation_ta = lv_textarea_create(output_screen);
    lv_obj_set_width(Elevation_ta, lv_pct(40));
    lv_obj_align(Elevation_ta, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_textarea_set_one_line(Elevation_ta, true);
    lv_obj_remove_flag(Azimuth_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Elevation label
    */
    lv_obj_t * Elevation_label = lv_label_create(output_screen);
    lv_label_set_text(Elevation_label, "Elevation:");
    lv_obj_align_to(Elevation_label, Elevation_ta, LV_ALIGN_OUT_TOP_MID, 0, 0);

    /* Back button
    */
    Back_button = lv_button_create(output_screen);
    lv_obj_align(Back_button, LV_ALIGN_BOTTOM_MID, 0, -20);
    //lv_group_remove_obj(Back_button);
    lv_obj_add_event_cb(Back_button, switch_screen, LV_EVENT_PRESSED, NULL);

    /* Back label
    */
    lv_obj_t * Back_label = lv_label_create(Back_button);
    lv_label_set_text(Back_label, "Back");
    lv_obj_center(Back_label);

    ESP_LOGI(SCREENTAG, "Screen initialization complete!");

}

void app_main(){

    ESP_LOGI(TAG, "\n\nDevice booting up...\n");
    display_brightness_init();
    display_brightness_set(0);
    initialize_spi();
    initialize_display();
    initialize_lvgl();
    lv_indev_keypad_init();
    initialize_screens();
    display_brightness_set(100);
    while (1){
        vTaskDelay(10/portTICK_PERIOD_MS);
        lv_timer_handler();
    }

}