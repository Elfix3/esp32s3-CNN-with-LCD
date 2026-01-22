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
    .pin_bit_mask = (1ULL << PIN_NUM_IRQ)|(1ULL << PIN_RESET_BUTTON),
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



static const char *TAG = "LCD touch mmodule";           //ILI9341 and XPT2046

void init_screen()
{
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

    //Attach LCD to the SPI bus
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

    //ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    //ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t touch_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_CS_TOUCH);
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
    
    
    ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(touch_io_handle, &touch_config, &touch_handle));

    vTaskDelay(pdMS_TO_TICKS(50));
    esp_lcd_touch_read_data(touch_handle);
}


void test_draw_frame(){
    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "LCD not initialized");
        return;
    }
    size_t pixels = LCD_H_RES * LCD_V_RES;
    uint16_t *frame = heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!frame) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        return;
    }
    for (size_t i = 0; i < pixels; i++) {
        frame[i] = 0x001F; // RGB565 blue
    }
    esp_err_t r = esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, frame);

    if( r!= ESP_OK){
        ESP_LOGE(TAG, "Failed to draw bitmap: %s", esp_err_to_name(r));
    } else {
        ESP_LOGI(TAG, "Sucess");
    }
    heap_caps_free(frame);
}

/* void test_touch_detect(){
    if (touch_handle == NULL) {
        ESP_LOGE(TAG, "Touch not initialized");
        return;
    } 

    // IMPORTANT : D'abord lire depuis le hardware
    esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
    
    if (ret != ESP_OK) {
        printf("Failed to read touch data: %s\n", esp_err_to_name(ret));
        return;
    }

    // Ensuite récupérer les coordonnées
    uint16_t x[1], y[1];
    uint16_t strength[1];
    uint8_t point_count = 0;
    bool touched = false;
    
    esp_lcd_touch_get_coordinates(touch_handle, x, y, strength, &point_count, 1);
    if (point_count > 0) {
        printf("Touch detected: x=%d, y=%d, strength=%d\n", x[0], y[0], strength[0]);
    } else {
        printf("No touch detected\n");
    }
}
 */