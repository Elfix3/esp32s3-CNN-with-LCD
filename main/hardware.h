#ifndef HW_H
#define HW_H

//<----- CONFIG FOR BOTH MY LCD AND TOUCH ---->

//###########################//
//######## INCLUDES #########//
//###########################//

#include <stdio.h>
#include "driver/gpio.h"                //for buttons (interrupt), LED

#include "esp_lcd_ili9341.h"            //screen driver
#include "esp_lcd_touch_xpt2046.h"      //touch driver

#include "esp_lcd_panel_io.h"           //esp_lcd_touch includes
#include "esp_lcd_panel_vendor.h"       //used to register devices 
#include "esp_lcd_panel_ops.h"

#include "esp_err.h"                    //loging
#include "esp_log.h"        


//###########################//
//########## PINS ###########//
//###########################//

// SPI pins
#define PIN_NUM_SCLK        15
#define PIN_NUM_MOSI        7
#define PIN_NUM_MISO        17
#define PIN_NUM_DC          6
#define PIN_NUM_RST         5
#define PIN_NUM_CS          4
#define PIN_NUM_BK          16
#define PIN_NUM_CS_TOUCH    18
#define PIN_NUM_IRQ         9
#define LCD_HOST            SPI2_HOST

// LCD config
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_H_RES 240
#define LCD_V_RES 320

// Backlight levels
#define BK_LIGHT_ON  1
#define BK_LIGHT_OFF 0


//LED and buttons
#define PIN_LED             2
#define PIN_RESET_BUTTON    35
#define PIN_BACK_BUTTON     36

//external handles used by LVGL
extern esp_lcd_panel_handle_t panel_handle;
extern esp_lcd_touch_handle_t touch_handle;


//to call once to initiliaze both LCD and touch
void init_hardware();

#endif