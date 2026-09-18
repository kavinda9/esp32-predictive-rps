#include "esp_camera.h"
#include "board_config.h"

// ===== Motion detection settings =====
#define MOTION_THRESHOLD 15       // how much a pixel's brightness must change to count as "different"
#define MOTION_PIXEL_PERCENT 2.0  // % of pixels that must change to trigger motion

uint8_t *prevFrame = NULL;
size_t frameSize = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

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
  config.frame_size = FRAMESIZE_QQVGA;       // 160x120
  config.pixel_format = PIXFORMAT_GRAYSCALE; // raw grayscale, no JPEG
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  Serial.println("Camera ready. Hold hand still, then move it to test motion detection.");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if (prevFrame == NULL) {
    // First frame — just store it as baseline, nothing to compare yet
    frameSize = fb->len;
    prevFrame = (uint8_t *)malloc(frameSize);
    memcpy(prevFrame, fb->buf, frameSize);
    esp_camera_fb_return(fb);
    return;
  }

  // Compare current frame to previous frame, pixel by pixel
  int changedPixels = 0;
  for (size_t i = 0; i < frameSize; i++) {
    int diff = abs((int)fb->buf[i] - (int)prevFrame[i]);
    if (diff > MOTION_THRESHOLD) {
      changedPixels++;
    }
  }

  float percentChanged = (changedPixels * 100.0) / frameSize;

  if (percentChanged > MOTION_PIXEL_PERCENT) {
    Serial.printf("MOTION DETECTED at t=%lu ms (%.2f%% pixels changed)\n", millis(), percentChanged);
  }

  // Update previous frame buffer for the next comparison
  memcpy(prevFrame, fb->buf, frameSize);
  esp_camera_fb_return(fb);
}