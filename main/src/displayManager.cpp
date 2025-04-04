#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

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
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
				vTaskDelete(showHandle);
				showHandle=NULL;
				// lvHandle=NULL;
			}
			else
			{
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
				xTaskCreate(&showData,"sdata",1024*3,NULL, 5, &showHandle); 
				dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(30000),pdFALSE,NULL, []( TimerHandle_t xTimer)
				{ 				
					vTaskDelete(showHandle);
					ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
					showHandle=NULL;

				});   
				xTimerStart(dispTimer,0);
	        
			}       
		}
		delay(1000);
	}
}


