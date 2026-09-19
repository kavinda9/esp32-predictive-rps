#include "esp_camera.h"
#include "board_config.h"
#include <Chirale_TensorFlowLite.h>
#include "rps_model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "esp_heap_caps.h"

#define IMG_WIDTH 96
#define IMG_HEIGHT 96
#define FRAME_BYTES (IMG_WIDTH * IMG_HEIGHT)

#define MOTION_TRIGGER_PERCENT 70.0
#define MOTION_PIXEL_DIFF 15
#define DELAY_BEFORE_CAPTURE_MS 3000
#define COOLDOWN_MS 10000

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

constexpr int kTensorArenaSize = 200 * 1024;
uint8_t *tensor_arena = nullptr;

uint8_t *prevFrame = NULL;

enum State { IDLE, WAITING_TO_CAPTURE, COOLDOWN };
State currentState = IDLE;
unsigned long stateStartTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  tensor_arena = (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
  if (tensor_arena == nullptr) {
    Serial.println("Failed to allocate tensor arena in PSRAM!");
    while (true);
  }

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

  model = tflite::GetModel(rps_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (true);
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    while (true);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("System ready. Waiting for motion...");
}

String runInference() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    return "ERROR";
  }

  for (int i = 0; i < FRAME_BYTES; i++) {
    float pixelNormalized = fb->buf[i] / 255.0;
    input->data.int8[i] = (int8_t)(pixelNormalized / input->params.scale + input->params.zero_point);
  }
  esp_camera_fb_return(fb);

  if (interpreter->Invoke() != kTfLiteOk) {
    return "ERROR";
  }

  float rockScore = (output->data.int8[0] - output->params.zero_point) * output->params.scale;
  float paperScore = (output->data.int8[1] - output->params.zero_point) * output->params.scale;
  float scissorsScore = (output->data.int8[2] - output->params.zero_point) * output->params.scale;

  Serial.printf("Rock: %.3f  Paper: %.3f  Scissors: %.3f\n", rockScore, paperScore, scissorsScore);

  if (rockScore >= paperScore && rockScore >= scissorsScore) return "ROCK";
  if (paperScore >= rockScore && paperScore >= scissorsScore) return "PAPER";
  return "SCISSORS";
}

void loop() {
  unsigned long now = millis();

  if (currentState == WAITING_TO_CAPTURE) {
    if (now - stateStartTime >= DELAY_BEFORE_CAPTURE_MS) {
      Serial.println("=== Capturing and predicting ===");
      String result = runInference();
      Serial.println(result);  // <-- this line is what the Arduino will read

      currentState = COOLDOWN;
      stateStartTime = millis();
      free(prevFrame);
      prevFrame = NULL;
    }
    return;
  }

  if (currentState == COOLDOWN) {
    if (now - stateStartTime >= COOLDOWN_MS) {
      currentState = IDLE;
      Serial.println("Ready for next round.");
    }
    return;
  }

  // IDLE state — watch for motion
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;

  if (prevFrame == NULL) {
    prevFrame = (uint8_t *)malloc(FRAME_BYTES);
    memcpy(prevFrame, fb->buf, FRAME_BYTES);
    esp_camera_fb_return(fb);
    return;
  }

  int changed = 0;
  for (int i = 0; i < FRAME_BYTES; i++) {
    if (abs((int)fb->buf[i] - (int)prevFrame[i]) > MOTION_PIXEL_DIFF) {
      changed++;
    }
  }
  float percentChanged = (changed * 100.0) / FRAME_BYTES;
  memcpy(prevFrame, fb->buf, FRAME_BYTES);
  esp_camera_fb_return(fb);

  if (percentChanged > MOTION_TRIGGER_PERCENT) {
    Serial.println("Motion detected! Waiting 3 seconds...");
    currentState = WAITING_TO_CAPTURE;
    stateStartTime = millis();
  }
}