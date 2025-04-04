#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"



static uint8_t oled_buffer[EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8];
static _lock_t lvgl_api_lock;

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t io_panel, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle =(esp_lcd_panel_handle_t) lv_display_get_user_data(disp);

    // This is necessary because LVGL reserves 2 x 4 bytes in the buffer, as these are assumed to be used as a palette. Skip the palette here
    // More information about the monochrome, please refer to https://docs.lvgl.io/9.2/porting/display.html#monochrome-displays
    px_map += EXAMPLE_LVGL_PALETTE_SIZE;

    uint16_t hor_res = lv_display_get_physical_horizontal_resolution(disp);
    int x1 = area->x1;
    int x2 = area->x2;
    int y1 = area->y1;
    int y2 = area->y2;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            /* The order of bits is MSB first
                        MSB           LSB
               bits      7 6 5 4 3 2 1 0
               pixels    0 1 2 3 4 5 6 7
                        Left         Right
            */
            bool chroma_color = (px_map[(hor_res >> 3) * y  + (x >> 3)] & 1 << (7 - x % 8));

            /* Write to the buffer as required for the display.
            * It writes only 1-bit for monochrome displays mapped vertically.*/
            uint8_t *buf = oled_buffer + hor_res * (y >> 3) + (x);
            if (chroma_color) {
                (*buf) &= ~(1 << (y % 8));
            } else {
                (*buf) |= (1 << (y % 8));
            }
        }
    }
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, oled_buffer);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
		if(time_till_next_ms==0)
			time_till_next_ms=10;
        usleep(1000 * time_till_next_ms);
    }
}

void showData(void * pArg)
{
	static _lock_t lvgl_api_lock;
	static lv_style_t titleStyle;

	lv_obj_t *scr = lv_display_get_screen_active(disp);
    // lv_style_init(&titleStyle);
	// lv_style_set_text_font(&titleStyle,  &lv_font_montserrat_18);
	lv_obj_t *label = lv_label_create(scr);
	// lv_obj_add_style(label, &titleStyle, 0);
	lv_obj_t *label2 = lv_label_create(scr);
	// lv_obj_add_style(label2, &titleStyle, 0);


	while(1)
	{
		_lock_acquire(&lvgl_api_lock);
		lv_label_cut_text(label,0,20);
		lv_label_cut_text(label2,0,20);
		lv_label_set_text_fmt(label,"Beats %d",theMeter.getBeats());
		lv_label_set_text_fmt(label2,"kWh %d",theMeter.getLkwh());
		// lv_label_set_text(label,losb);
		// lv_label_set_text(label2,losk);
		// lv_obj_set_pos(label, 40, 20); 
		// lv_obj_set_pos(label2, 40, 80); 
		lv_obj_align(label,LV_ALIGN_TOP_MID, 0, 7);
		lv_obj_align(label2,LV_ALIGN_BOTTOM_MID ,0,-10);
		_lock_release(&lvgl_api_lock);
		delay(1000);
	}
}

esp_err_t init_lvgl()
{
    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .bits_per_pixel = 1,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));


    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    esp_err_t ret=esp_lcd_panel_init(panel_handle);
    if(ret!=0)
        {
            printf("No OLED\n");
            return ESP_FAIL;
        }
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
    esp_lcd_panel_mirror(panel_handle, true, true);

    ESP_LOGI(TAG, "Initialize LVGL");
    lv_init();
    // create a lvgl display
    display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    // associate the i2c panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // create draw buffer
    dispbuf = NULL;
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers");
    // LVGL reserves 2 x 4 bytes in the buffer, as these are assumed to be used as a palette.
    size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8 + EXAMPLE_LVGL_PALETTE_SIZE;
    dispbuf = heap_caps_calloc(1, draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(dispbuf);

    // LVGL9 suooprt new monochromatic format.
    lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, dispbuf, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_FULL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    /* Register done callback */
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display);

    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
	if(!lvHandle)
    	xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, &lvHandle);

	return ESP_OK;
}

void deinit_lvgl()
{
	if(dispTimer)
	{
		if(xTimerStop(dispTimer,0)!=pdPASS)
			printf("Could not stop dispTimer\n");
		if(xTimerDelete(dispTimer,0)!=pdPASS)
			printf("Could not delete disptimer\n");
		esp_rom_printf("Timers\n");
	}
	if(showHandle)
		vTaskDelete(showHandle);
		printf("Shwoh\n");
	if(lvHandle)
		vTaskDelete(lvHandle);
	showHandle=NULL;
	lvHandle=NULL;
	esp_rom_printf("Tasks\n");
	
	if (lvgl_tick_timer)
		if(esp_timer_delete(lvgl_tick_timer)!=ESP_OK)
			printf("Failed timer lvgl\n");
		
	// const esp_lcd_panel_io_callbacks_t cbs = {
	// 	.on_color_trans_done = NULL,
	// };
	/* Register done callback */
	// esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display);
	// lv_display_set_flush_cb(display, NULL);
	esp_rom_printf("cbs\n");
	if(dispbuf)
	{
		free(dispbuf);
		dispbuf=NULL;
		esp_rom_printf("buffer\n");
	}
	if(display)
	{
		lv_display_delete(display);
		display=NULL;
		esp_rom_printf("disp delete\n");
	}
	if(panel_handle)
	{
		esp_lcd_panel_del(panel_handle);
		panel_handle=NULL;
		esp_rom_printf("Panel\n");

	}

	esp_rom_printf("deinit done\n");
}

void timeout_disp(TimerHandle_t xTimer )
{
	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
	deinit_lvgl();
	esp_rom_printf("Tcb done\n");
}
void displayManager(void *arg) 
{
  	gpio_config_t 	        io_conf;
    
    bzero(&io_conf,sizeof(io_conf));

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en =GPIO_PULLUP_ENABLE;
	io_conf.pin_bit_mask =  (1ULL<<0); //input pins Flash button
	gpio_config(&io_conf);

	
	while(true)
	{
		if(!gpio_get_level((gpio_num_t)0))
		{
			if(showHandle)
			{
				esp_rom_printf("Disp deinit\n");
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
				deinit_lvgl();
			}
			else
			{
				init_lvgl();
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
				xTaskCreate(&showData,"sdata",1024*3,NULL, 5, &showHandle); 
				// dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(30000),pdFALSE,NULL, []( TimerHandle_t xTimer)
				// { 	
				// 	// if(showHandle)
				// 	// 	vTaskDelete(showHandle);
				// 	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
				// 	deinit_lvgl();
					
				// });   
				if(!dispTimer)
					dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(5000),pdFALSE,0,timeout_disp);
				if(dispTimer)
					xTimerStart(dispTimer,0);
	        
			}       
		}
		delay(1000);
	}
}


