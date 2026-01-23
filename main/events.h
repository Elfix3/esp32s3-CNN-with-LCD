#ifndef EVENTS_H
#define EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t eventQueue;

typedef enum {
    RESET_EVENT,            //Clears screen
    CANCEL_EVENT,           //Removes the last line
    INFERENCE_EVENT         //Makes some inference
} EVENT_TYPES;

typedef struct{
    EVENT_TYPES type;
    union {
        struct {
            char className[25];
            float probability;
        } inference;
    };
} event_t ;

#endif