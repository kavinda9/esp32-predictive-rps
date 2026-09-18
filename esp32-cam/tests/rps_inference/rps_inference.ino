#include "esp_camera.h"
#include "board_config.h"
#include <ArduTFLite.h>
#include "rps_model.h"

// Model input size (must match training: 96x96 grayscale)
#define IMG_WIDTH 96
#define IMG_HEIGHT 96

// Tensor arena — working memory for the model. Start here, increase if it fails.
constexpr int kTensorArenaSize = 130 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

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
  config.frame_size = FRAMESIZE_96X96;       // capture directly at model's input size
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
  Serial.println("Initializing TFLite model...");
  if (!modelInit(rps_model, tensorArena, kTensorArenaSize)) {
    Serial.println("Model initialization failed!");
    return;
  }
  Serial.println("Model ready. Starting inference loop.");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(500);
    return;
  }

  // Feed each pixel into the model's input tensor
  // Model expects int8 quantized input: convert 0-255 grayscale -> -128 to 127
  for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
    int8_t pixelValue = (int8_t)((int)fb->buf[i] - 128);
    modelSetInput(pixelValue, i);
  }

  esp_camera_fb_return(fb);

  if (!modelRunInference()) {
    Serial.println("Inference failed!");
    delay(500);
    return;
  }

  float rockScore = modelGetOutput(0);
  float paperScore = modelGetOutput(1);

  Serial.printf("Rock: %.2f  Paper: %.2f  -> %s\n",
                rockScore, paperScore,
                rockScore > paperScore ? "ROCK" : "PAPER");

  delay(1000); // one prediction per second for now, easy to read in Serial Monitor
}