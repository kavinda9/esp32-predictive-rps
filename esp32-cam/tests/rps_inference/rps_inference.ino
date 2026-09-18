#include "esp_camera.h"
#include "board_config.h"
#include <Chirale_TensorFlowLite.h>
#include "rps_model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#define IMG_WIDTH 96
#define IMG_HEIGHT 96

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

constexpr int kTensorArenaSize = 130 * 1024;
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

void setup() {
  Serial.begin(115200);
  Serial.println();

  // ===== Camera setup =====
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_96X96;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera ready.");

  // ===== TFLite Micro setup =====
  Serial.println("Initializing TensorFlow Lite Micro Interpreter...");

  model = tflite::GetModel(rps_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model provided and schema version are not equal!");
    while (true);
  }

  static tflite::AllOpsResolver resolver;

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    while (true);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Model ready. Starting inference loop.");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(500);
    return;
  }

  // Quantize each pixel (0-255 grayscale) into the model's input tensor
  for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
    float pixelNormalized = fb->buf[i] / 255.0;
    input->data.int8[i] = (int8_t)(pixelNormalized / input->params.scale + input->params.zero_point);
  }

  esp_camera_fb_return(fb);

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed!");
    delay(500);
    return;
  }

  // Dequantize output scores
  float rockScore = (output->data.int8[0] - output->params.zero_point) * output->params.scale;
  float paperScore = (output->data.int8[1] - output->params.zero_point) * output->params.scale;

  Serial.printf("Rock: %.3f  Paper: %.3f  -> %s\n",
                rockScore, paperScore,
                rockScore > paperScore ? "ROCK" : "PAPER");

  delay(1000);
}