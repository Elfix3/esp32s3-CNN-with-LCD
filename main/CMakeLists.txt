idf_component_register(SRCS "main.c" "hardware.c" "lvgl_setup.c" "cnn_model.cc" "functions.cc"
    INCLUDE_DIRS "."
    REQUIRES 
        "esp_lcd"
        "esp_timer"
        "espressif__esp-tflite-micro"
)
