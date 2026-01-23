#include "hardware.h"




//panel handle
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;

//OUTPUT GPIO CONF
static const gpio_config_t bkLightConf = {
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << PIN_NUM_BK)|(1ULL << PIN_LED),
    .intr_type = GPIO_INTR_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE,
};

//INPUT GPIO CONF
static gpio_config_t IRQconf = {
    .pin_bit_mask = (1ULL << PIN_NUM_IRQ)|(1ULL << PIN_RESET_BUTTON) | (1ULL << PIN_BACK_BUTTON),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .intr_type = GPIO_INTR_NEGEDGE, // trigger on falling edge (touch)
};

//SPI CONF
static const spi_bus_config_t spiConf = {
    .sclk_io_num = PIN_NUM_SCLK,
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t) //1 chunk of 80 rows
};



static const char *TAG = "LCD touch module";           //ILI9341 and XPT2046

void init_hardware(){
    //BK GPIO initialize
    ESP_LOGI(TAG,"Initialize LCD backlight");
    ESP_ERROR_CHECK(gpio_config(&bkLightConf));
    
    ESP_LOGI(TAG,"Initialize LCD backlight");
    ESP_ERROR_CHECK(gpio_config(&IRQconf));

    //SPI initialize
    ESP_LOGI(TAG,"Initialize SPI bus");
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &spiConf, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = (20 * 1000 * 1000),              //20 MHz
        .lcd_cmd_bits = 8,                          //8 bits per command
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    //ATTACH LCD TO SPI BUS
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    gpio_set_level(PIN_NUM_BK, BK_LIGHT_ON);

    ESP_LOGI(TAG, "Install ILI9341 panel driver");


    //ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    //ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    //TOUCH INIT
    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t touch_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_CS_TOUCH);
    
    //ATTACH TOUCH TO SPI BUS
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &touch_io_config, &touch_io_handle));
    esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = PIN_NUM_IRQ,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 1, //need LCD mirror y
        },
    };
    
    
    ESP_LOGI(TAG, "Initialize XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(touch_io_handle, &touch_config, &touch_handle));
}

