# Image classification with ESP32s3 and tflite

### Description  :
CNNs often require high computation costs and some heavy image processing algorithms. Thanks to tensorflow lite, implementing some microcontroller friendly CNNs is feasable.This personal project demonstrates a CNN that recognizes simple user drawings on a TFT LCD screen among 20 different categories

### Categories :
bee, blueberry, bread, butterfly, cake, candle, carrot, cat, hourglass, ice cream, leaf, lobster, lollipop, monkey, mushroom, pineapple, potato, spider, umbrella, wine glass

### Technologies used :
-Quickdraw data set from google (28*28 rescaled MNIST image style)
-Tensorflow and python for training
-TFlite for microcontroler for a rescaled and quantized version of the CNN
-Esp-IDF and FreeRTOS for the real time inference system and screen management
-Lvgl as a graphical library
-Hardware : esp32s3, ILI9341 screen, xpt2046 touch module

### A few results :
-Test accuracy on the quantized model : 91 %
-Inference time 39 ms

 ### Final system ![butterfly](https://github.com/user-attachments/assets/54aade2e-9779-45d3-824a-948c062d8218)