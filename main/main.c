#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "driver/gpio.h"

#include "lvgl_setup.h"
#include "functions.h"
#include "events.h"


QueueHandle_t eventQueue;



void IRAM_ATTR on_back_button_press(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    event_t backEvt = {.type = CANCEL_EVENT};
    xQueueSendFromISR(eventQueue, &backEvt,&xHigherPriorityTaskWoken);
}

void IRAM_ATTR on_reset_button_press(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    event_t resetEvt = {.type = RESET_EVENT};
    xQueueSendFromISR(eventQueue, &resetEvt,&xHigherPriorityTaskWoken);
}

void app_main(void){
    //<-----INIT SECTION---->
    eventQueue = xQueueCreate(5,sizeof(event_t));
    init_hardware();
    init_lvgl();
    INIT_MODEL();

    //ISR :
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_RESET_BUTTON,on_reset_button_press, NULL);
    gpio_isr_handler_add(PIN_BACK_BUTTON,on_back_button_press, NULL);


    //Event queue :

    xTaskCreatePinnedToCore(inferenceTask, "Inference task", 4096, NULL, 10, NULL, 1);
    //const int8_t *converted;
    //static uint8_t resized_image[784];
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
        //get_resized_image_canvas(resized_image);
        //converted = convertImageForCnn(resized_image,784);
        //r = infere(converted);
       
    }
}
