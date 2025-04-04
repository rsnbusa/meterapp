#include "defines.h"
#define TAG "MI"
#define GLOBAL
#include "globals.h"
//lcd 
// #define PIN_NUM_SDA                     (1) 
// #define PIN_NUM_SCL                     (42) 



esp_err_t init_lcd()
{
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config;
    bzero(&bus_config,sizeof(bus_config));
        bus_config.i2c_port = I2C_BUS_PORT;
        bus_config.sda_io_num = (gpio_num_t)2;
        bus_config.scl_io_num = (gpio_num_t)42;
        bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_config.glitch_ignore_cnt = 7;

            bus_config.flags.enable_internal_pullup = true;
            bus_config.flags.allow_pd = false;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));


    ESP_LOGI(TAG, "Install panel IO");
    io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
        .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    return ESP_OK;

    // ESP_LOGI(TAG, "Install SSD1306 panel driver");
    // panel_handle = NULL;
    // esp_lcd_panel_dev_config_t panel_config = {
    //     .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    //     .bits_per_pixel = 1,
    // };
    // esp_lcd_panel_ssd1306_config_t ssd1306_config = {
    //     .height = EXAMPLE_LCD_V_RES,
    // };
    // panel_config.vendor_config = &ssd1306_config;
    // ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));


    // ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    // esp_err_t ret=esp_lcd_panel_init(panel_handle);
    // if(ret!=0)
    //     {
    //         printf("No OLED\n");
    //         return ESP_FAIL;
    //     }
    // ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
    // esp_lcd_panel_mirror(panel_handle, true, true);

    // ESP_LOGI(TAG, "Initialize LVGL");
    // lv_init();
    // // create a lvgl display
    // lv_display_t *display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    // // associate the i2c panel handle to the display
    // lv_display_set_user_data(display, panel_handle);
    // // create draw buffer
    // void *buf = NULL;
    // ESP_LOGI(TAG, "Allocate separate LVGL draw buffers");
    // // LVGL reserves 2 x 4 bytes in the buffer, as these are assumed to be used as a palette.
    // size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8 + EXAMPLE_LVGL_PALETTE_SIZE;
    // buf = heap_caps_calloc(1, draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // assert(buf);

    // // LVGL9 suooprt new monochromatic format.
    // lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    // // initialize LVGL draw buffers
    // lv_display_set_buffers(display, buf, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_FULL);
    // // set the callback which can copy the rendered image to an area of the display
    // lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    // ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    // const esp_lcd_panel_io_callbacks_t cbs = {
    //     .on_color_trans_done = example_notify_lvgl_flush_ready,
    // };
    // /* Register done callback */
    // esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display);

    // ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    // const esp_timer_create_args_t lvgl_tick_timer_args = {
    //     .callback = &example_increase_lvgl_tick,
    //     .name = "lvgl_tick"
    // };
    // esp_timer_handle_t lvgl_tick_timer = NULL;
    // ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    // ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    // ESP_LOGI(TAG, "Create LVGL task");
    // xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    // return ESP_OK;
}



