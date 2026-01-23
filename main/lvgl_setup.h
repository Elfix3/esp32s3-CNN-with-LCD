#ifndef LVGL_FUNCTIONS_H
#define LVGL_FUNCTIONS_H

#include "hardware.h"
#include "lvgl.h"

#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include <sys/param.h>

#include "events.h"


//data for the touch_queue
typedef struct {
    int32_t x;
    int32_t y;
    bool pressed; // 0=pressed, 1=released, 2=moved ??
} touch_data_t;


void init_lvgl();



#ifdef __cplusplus
extern "C" {
#endif

bool get_resized_image_canvas(uint8_t* dst_28_28_buff);

#ifdef __cplusplus
}
#endif

#endif