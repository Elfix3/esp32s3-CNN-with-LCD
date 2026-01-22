#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "driver/gpio.h"

//dont forget esp require timer in CMakeLists

#include "hardware.h"






void app_main(void){


    //<-----INIT SECTION---->
    init_screen();


    while(1){
        gpio_set_level(PIN_LED,0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(PIN_LED,1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
