#include "functions.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"


/* const char* categories[NUM_CLASSES] = {
    "bread",
    "candle",
    "cat",
    "hourglass",
    "mushroom",
    "pineapple",
    "potato",
    "spider",
    "umbrella",
    "wine glass"
}; */


/* const char* categories_30[30] = {
    "bee",
    "blueberry",
    "brain",
    "bread",
    "butterfly",
    "cactus",
    "cake",
    "candle",
    "carrot",
    "cat",
    "frying_pan",
    "hourglass",
    "ice_cream",
    "leaf",
    "lobster",
    "lollipop",
    "monkey",
    "mushroom",
    "octopus",
    "pineapple",
    "pizza",
    "potato",
    "rabbit",
    "shovel",
    "snowman",
    "spider",
    "sword",
    "umbrella",
    "violin",
    "wine_glass"
};
 */
const char* categories_20[NUM_CLASSES] = {
    "bee",
    "blueberry",
    "bread",
    "butterfly",
    "cake",
    "candle",
    "carrot",
    "cat",
    "hourglass",
    "ice cream",
    "leaf",
    "lobster",
    "lollipop",
    "monkey",
    "mushroom",
    "pineapple",
    "potato",
    "spider",
    "umbrella",
    "wine glass"
};






// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace




void INIT_MODEL(){

    model = tflite::GetModel(model_parameters);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model provided is schema version %d not equal to supported "
                    "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }


    //resolver
    static tflite::MicroMutableOpResolver<8> resolver;                                          //keep softmax in resolver ?
    if (resolver.AddMaxPool2D() != kTfLiteOk ||
    resolver.AddConv2D() != kTfLiteOk ||
    resolver.AddShape() != kTfLiteOk ||
    resolver.AddReshape() != kTfLiteOk ||
    resolver.AddFullyConnected() != kTfLiteOk ||
    resolver.AddStridedSlice() != kTfLiteOk ||
    resolver.AddSoftmax() != kTfLiteOk ||
    resolver.AddPack() != kTfLiteOk) { 
    MicroPrintf("Error: resolver error");
} else {
    MicroPrintf("Resolver correct");
}
    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed");
        return;
    } else {
        MicroPrintf("AllocateTensors() Successful");
    }

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);
    output = interpreter->output(0);

    MicroPrintf("Input size  :  %i, (%i x %i x %i x %i)",input->dims->size, input->dims->data[0],input->dims->data[1],input->dims->data[2],input->dims->data[3]);
    MicroPrintf("Output size  :  %i, (%i x %i)",output->dims->size, output->dims->data[0],output->dims->data[1]);
    // Keep track of how many inferences we have performed.

    switch (input->type) {
        case kTfLiteInt8:
            MicroPrintf("Model expects int8 input");
            break;
        default:
            MicroPrintf("Unknown input type: %d", input->type);
    }

    MicroPrintf("Input scale: %f, Input zero_point: %d", input->params.scale, input->params.zero_point);
    MicroPrintf("Output scale: %f, Output zero_point: %d", output->params.scale, output->params.zero_point);

    inference_count = 0;
}

void convertImageForCnn(uint8_t *resized_28_28, uint32_t n_pixels, int8_t *destination){
    for(uint32_t i = 0; i<n_pixels; i++){
        destination[i]= 255-resized_28_28[i]; //inverts color
        destination[i]-=128;                  //input->param.zero_points
        //we dont divide by the scale, because the image is already 0-255 :)
    }
}


volatile static int64_t start_time;

void infere(const int8_t*image){  //pass parameter here

    start_time = esp_timer_get_time();
    //input->data.int8;
    memcpy(input->data.int8, image, 28*28*sizeof(int8_t));
    if (kTfLiteOk != interpreter->Invoke()) {
      MicroPrintf("Invoke failed.");
    }

    output = interpreter->output(0);

    int8_t* out = output->data.int8;  // cast properly
    
    //Qz parameters
    float scale = output->params.scale;
    int zero_point = output->params.zero_point;
    
    // Compute softmax probabilities
    float dequantized[NUM_CLASSES];
    float exp_sum = 0.0f;
    
    int predicted_class = -1;
    float max_score = -std::numeric_limits<float>::max();
    
    for (int i = 0; i < NUM_CLASSES; i++) {
        dequantized[i] = scale * (out[i] - zero_point);  // dequantize
        dequantized[i] = expf(dequantized[i]);           // exponentiate
        exp_sum += dequantized[i];
        
        
        if (dequantized[i] > max_score) {
            max_score = dequantized[i];
            predicted_class = i;
        }
    }
    volatile int64_t finish_time = esp_timer_get_time();
    const char* guess = categories_20[predicted_class];
    
    MicroPrintf("Micro printf : Inference time : %lli ms, guessed : %s", (finish_time - start_time + 500)/1000 ,guess);
    //printf("Printf : Inference time : %lli ms, guessed : %s\n", (finish_time - start_time) ,guess);
    event_t inf_event;
    inf_event.type = INFERENCE_EVENT;
    
    // Accédez directement au champ de l'union sans nom
    strncpy(inf_event.inference.className, guess, sizeof(inf_event.inference.className) - 1);
    inf_event.inference.className[sizeof(inf_event.inference.className) - 1] = '\0';
    inf_event.inference.probability = (float)(max_score / exp_sum);
    
    
    //event_t t = {.type = CANCEL_EVENT};
    if(xQueueSend(eventQueue, &inf_event,20) != pdPASS){
        printf("Error of queue send\n");
    }
    inference_count++;
}

void inferenceTask(void* TaskParameters_t){
    static uint8_t buffer[784];
    static int8_t converted[784];
    while(1){
        if(!get_resized_image_canvas(buffer)){
            printf("Error of resize\n");
        }else {
            convertImageForCnn(buffer,784, converted);
            infere(converted);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}