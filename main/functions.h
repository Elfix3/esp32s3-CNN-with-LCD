#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include <stdint.h>
#include "events.h"
#include "lvgl_setup.h"         //only to get canvas buffer in the infere task
#include "cnn_model.h"

#ifdef __cplusplus
extern "C" {
#endif
    

#define NUM_CLASSES 20



void INIT_MODEL();
void infere(const int8_t *image);  //input data where ?
void convertImageForCnn(uint8_t *resized_28_28, uint32_t n_pixels, int8_t* destination);

void inferenceTask(void* TaskParameters_t);

#ifdef __cplusplus
}
#endif

#endif