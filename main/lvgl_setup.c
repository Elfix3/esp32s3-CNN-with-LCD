#include "lvgl_setup.h"

static const char *LVGL_TAG = "LVGL utils";


//defines
#define CANVAS_WIDTH  224
#define CANVAS_HEIGHT 224
#define LVGL_TICK_PERIOD_MS 5


bool reset_asked = false;

//LVGL Mutex, to prevent redraw during lv_timer_handler(), Not thread safe :)
SemaphoreHandle_t lvgl_mux = NULL;

//display size and buffer
static const size_t draw_buffer_sz = LCD_H_RES * 20 * sizeof(lv_color16_t);
void *buf1 = NULL;
void *buf2 = NULL;

//display handle
static lv_display_t * lvgl_display = NULL;

//input device handle
static lv_indev_t * lvgl_indev = NULL;

//CANVAS FOR DRAWING
static lv_obj_t * draw_canvas; 
LV_DRAW_BUF_DEFINE_STATIC(draw_canvas_buffer, 224, 224,  LV_COLOR_FORMAT_L8);


//Graphical elements on my UI
typedef struct {
    lv_obj_t* draw_zone_rect;
    lv_obj_t* guess_zone_rect;
    lv_obj_t* label_guess_title;
    lv_obj_t* label_guess_result;
} ui_elements_t;


//empty ui
ui_elements_t ui = {.draw_zone_rect = NULL, .guess_zone_rect = NULL, .label_guess_title = NULL, .label_guess_result = NULL};

//Call back function of LVGL for screen
static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map){
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;
    lv_draw_sw_rgb565_swap(px_map, (x2 + 1 - x1) * (y2 + 1 - y1));
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);

    lv_display_flush_ready(disp);

}

static void lvgl_canvas_draw_dot(lv_obj_t *canvas, const int16_t x, const int16_t y, const uint8_t pxSize){
    //function used by LVGL to draw on my LCD
    if(canvas == NULL){
        printf("NULL CANVA !!\n");
        return;
    }
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0x000000); // Black color
    rect_dsc.radius = 0;

    rect_dsc.radius = pxSize / 2; // Arrondi pour effet circulaire
    
    lv_area_t area;
    area.x1 = x - pxSize / 2;
    area.y1 = y - pxSize / 2;
    area.x2 = x + pxSize / 2;
    area.y2 = y + pxSize / 2;

    lv_draw_rect(&layer, &rect_dsc, &area);
                
    lv_canvas_finish_layer(canvas, &layer);
}

static void lvgl_canvas_draw_line(lv_obj_t *canvas, const int16_t x1, const int16_t y1, const int16_t x2, const int16_t y2){
    //Used to draw lines between previous point and current point
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    /*Configurer et dessiner la ligne*/
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_hex(0x000000);
    dsc.width = 10;
    dsc.round_end = 1;
    dsc.round_start = 1;
    dsc.p1.x = x1;
    dsc.p1.y = y1;
    dsc.p2.x = x2;
    dsc.p2.y = y2;

    lv_draw_line(&layer, &dsc);
    lv_canvas_finish_layer(canvas, &layer);
}


static void lv_canvas_clear() {
    if(xSemaphoreTake(lvgl_mux,pdMS_TO_TICKS(50) == pdTRUE)){
        lv_canvas_fill_bg(draw_canvas, lv_color_hex(0xffffff), LV_OPA_COVER);
        xSemaphoreGive(lvgl_mux);
    }    
}

//Call back  function for LVGL touch
static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data){
    static int16_t last_x = -1;
    static int16_t last_y = -1;

    esp_lcd_touch_point_data_t touch_point;
    uint8_t point_cnt = 0;

    esp_lcd_touch_read_data(touch_handle);
    esp_err_t r = esp_lcd_touch_get_data(touch_handle, &touch_point, &point_cnt, 1);
    
    if (r == ESP_OK && point_cnt > 0) {
        int16_t y = (int16_t)touch_point.y;
        int16_t x = (int16_t)touch_point.x;

        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;

        if(last_x >= 0 && last_y >= 0) {
            lvgl_canvas_draw_line(draw_canvas, last_x-8, last_y-8, x-8, y-8); //offset of 8 because canva is placed at px8 px8
        }
        last_x = x;
        last_y = y;
            
    } else {
        last_x = -1;
        last_y = -1;

        data->state = LV_INDEV_STATE_REL;
        data->point.x = last_x;
        data->point.y = last_y;
    } 
}
static void lvgl_tick_cb(void *pvParameters){
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}



//LVGL MAIN TASK, not thread safe
static void lvgl_task(void *pvParameters){
    while(1) {
        if(xSemaphoreTake(lvgl_mux, pdMS_TO_TICKS(10)) == pdTRUE) {
            uint32_t delay = lv_timer_handler();
            xSemaphoreGive(lvgl_mux);
            if(delay > 100) delay = 100;            //reduce this ?
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}


    
//Create UI elements for LVGL
static void lvgl_create_UI(){

    //draw zone border
    ui.draw_zone_rect = lv_obj_create(lv_screen_active());
    lv_obj_set_pos(ui.draw_zone_rect, 0, 0);
    lv_obj_set_size(ui.draw_zone_rect, 240, 240);
    lv_obj_set_style_bg_opa(ui.draw_zone_rect, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_color(ui.draw_zone_rect, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.draw_zone_rect, 3, LV_PART_MAIN);

    //canvas draw zone
    draw_canvas = lv_canvas_create(lv_screen_active());
    LV_DRAW_BUF_INIT_STATIC(draw_canvas_buffer);

    lv_canvas_set_draw_buf(draw_canvas, &draw_canvas_buffer);
    lv_obj_set_size(draw_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_canvas_fill_bg(draw_canvas, lv_color_hex(0xffffff), LV_OPA_COVER);
    lv_obj_set_pos(draw_canvas, 8, 8);

    //Guess zone
    ui.guess_zone_rect = lv_obj_create(lv_screen_active());
    lv_obj_set_pos(ui.guess_zone_rect, 0, 240);
    lv_obj_set_size(ui.guess_zone_rect, 240, 80);
    lv_obj_set_style_bg_color(ui.guess_zone_rect, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui.guess_zone_rect, LV_OPA_100, LV_PART_MAIN);

    //guess title zone
    ui.label_guess_title = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(ui.label_guess_title, &lv_font_montserrat_12, LV_PART_MAIN); //size here
    lv_obj_set_style_text_color(ui.label_guess_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // color here
    lv_label_set_text(ui.label_guess_title, "Draw guessed :");
    lv_obj_set_pos(ui.label_guess_title,18,275);

    //guess title results : /!\ Will be modified
    ui.label_guess_result = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(ui.label_guess_result, &lv_font_montserrat_12, LV_PART_MAIN); //size here
    lv_obj_set_style_text_color(ui.label_guess_result, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // color here
    lv_label_set_text(ui.label_guess_result, "Bread 91 %");
    lv_obj_set_pos(ui.label_guess_result,130,275);
}
//


//resize the 224*224 canvas to a 28*28 uint32_t grayscale buffer
bool get_resized_image_canvas(uint8_t* dst_28_28_buff) {
    //safety improve needed
    lv_draw_buf_t buff = draw_canvas_buffer;

    if (buff.data == NULL) {
        printf("ERROR: Buffer data is NULL\n");
        return false;
    }

    float x_ratio = 224.0f / 28.0f; // = 8.0
    float y_ratio = 224.0f / 28.0f; // = 8.0

    for (uint32_t y = 0; y < 28; y++) {
        for (uint32_t x = 0; x < 28; x++) {
            // Calculer la zone source correspondant à ce pixel destination
            uint32_t x_start = (uint32_t)(x * x_ratio);
            uint32_t y_start = (uint32_t)(y * y_ratio);
            uint32_t x_end   = (uint32_t)((x + 1) * x_ratio);
            uint32_t y_end   = (uint32_t)((y + 1) * y_ratio);

            if (x_end > 224) x_end = 224;
            if (y_end > 224) y_end = 224;

            // Faire la moyenne des pixels dans la zone
            uint32_t sum = 0;
            uint32_t count = 0;
            for (uint32_t sy = y_start; sy < y_end; sy++) {
                for (uint32_t sx = x_start; sx < x_end; sx++) {
                    sum += buff.data[sy * 224 + sx];
                    count++;
                }
            }

            dst_28_28_buff[y * 28 + x] = (uint8_t)(sum / count);
        }
    }
    return true;
}



void manage_events(void* TaskParameters_t){
    static event_t event;
    while(1) {
        if(xQueueReceive(eventQueue, &event,10) == pdPASS){
            switch(event.type){
                case RESET_EVENT :
                    lv_canvas_clear();      //do work here no ?
                    printf("Clear done\n");
                break;
                case CANCEL_EVENT :
                    printf("Cancel done\n");
                //not yet implemented
                break;
                case INFERENCE_EVENT :
                    //printf("I recieve\n");
                    printf("%s %.1f%%",event.inference.className,event.inference.probability*100);

                    char buffer[100];
                    snprintf(buffer, sizeof(buffer), "%s %.1f%%", event.inference.className, event.inference.probability * 100);
                    lv_label_set_text(ui.label_guess_result, buffer);

                break;
                default :
                    //sets text here                
                break;

            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void init_lvgl(){
    //NOT   A   TASK !!!!
    // DO NOT RUN HW INIT HERE, only in main

    lvgl_mux = xSemaphoreCreateMutex();
    lv_init();
    
    //registers the display devices and its cb
    lvgl_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);
    buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    buf2 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    lv_display_set_buffers(lvgl_display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);


    //registers the inputdevice and its cb
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    
    ESP_LOGI(LVGL_TAG, "Initalisation of LVGL successfull");
    ESP_LOGI(LVGL_TAG, "Loading of the UI");
    //Create UI here
    lvgl_create_UI();

    //Start of freertos Tasks
    xTaskCreatePinnedToCore(lvgl_task, "lvgl",  8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(manage_events,"manage events", 4096, NULL, 2, NULL, 0);
    const esp_timer_create_args_t tick_timer_args = {
    .callback = &lvgl_tick_cb,
    .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_timer_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS*1000); // microsecondes
    
    //xTaskCreatePinnedToCore(printCanvasBuffer, "print buffer size", 4096, NULL, 2, NULL,0);
    //xTaskCreatePinnedToCore(lvgl_tick_task, "tick", 2048, NULL, 6, NULL, 1);
    //xTaskCreatePinnedToCore(touch_handler_task, "touch_handle", 2048, NULL, 5, NULL, 0);
    //xTaskCreatePinnedToCore(dummyBlink, "blink", 2048, NULL, 2,NULL,0);
}


//DISPLAY FUNCTIONS


