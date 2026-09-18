#include "esp_camera.h"
#include "board_config.h"
#include "SD_MMC.h"
#include "FS.h"

#define MOTION_TRIGGER_PERCENT 70.0   // must exceed this to START a session
#define BURST_DURATION_MS 4000        // capture for this long once triggered
#define FRAME_INTERVAL_MS 100         // one frame every 100ms (~40 frames per session)
#define COOLDOWN_MS 10000             // ignore everything for this long after a session

uint8_t *prevFrame = NULL;
size_t frameSize = 0;
int frameWidth = 0;
int frameHeight = 0;

int sessionNumber = 0;

enum State { IDLE, COOLDOWN };
State currentState = IDLE;
unsigned long cooldownStart = 0;

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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  frameWidth = 160;
  frameHeight = 120;

  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card mounted.");

  Serial.println("Ready. Make a big, deliberate motion (>70% change) to start a session.");
}

void savePGM(uint8_t *data, size_t len, int sessionNum, int frameNum) {
  char filename[64];
  snprintf(filename, sizeof(filename), "/session%02d_frame%02d.pgm", sessionNum, frameNum);

  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open %s for writing\n", filename);
    return;
  }

  file.printf("P5\n%d %d\n255\n", frameWidth, frameHeight);
  file.write(data, len);
  file.close();

  Serial.printf("Saved %s\n", filename);
}

void runCaptureBurst() {
  sessionNumber++;
  Serial.printf("=== TRIGGERED! Starting capture session %d ===\n", sessionNumber);

  unsigned long burstStart = millis();
  int frameNum = 0;

  while (millis() - burstStart < BURST_DURATION_MS) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      savePGM(fb->buf, fb->len, sessionNumber, frameNum);
      frameNum++;
      esp_camera_fb_return(fb);
    }
    delay(FRAME_INTERVAL_MS);
  }

  Serial.printf("=== Session %d complete: %d frames captured ===\n", sessionNumber, frameNum);
  Serial.println("Entering 10s cooldown...");

  currentState = COOLDOWN;
  cooldownStart = millis();
}

void loop() {
  // Handle cooldown state — ignore everything until it ends
  if (currentState == COOLDOWN) {
    if (millis() - cooldownStart >= COOLDOWN_MS) {
      currentState = IDLE;
      // Reset prevFrame so we don't compare against a stale frame from before cooldown
      free(prevFrame);
      prevFrame = NULL;
      Serial.println("Cooldown finished. Ready for next session.");
    } else {
      delay(50); // small delay while waiting out cooldown, avoids busy-spinning
      return;
    }
  }

  // IDLE state — watch for a big motion trigger
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if (prevFrame == NULL) {
    frameSize = fb->len;
    prevFrame = (uint8_t *)malloc(frameSize);
    memcpy(prevFrame, fb->buf, frameSize);
    esp_camera_fb_return(fb);
    return;
  }

  int changedPixels = 0;
  for (size_t i = 0; i < frameSize; i++) {
    int diff = abs((int)fb->buf[i] - (int)prevFrame[i]);
    if (diff > 15) {  // per-pixel brightness change threshold
      changedPixels++;
    }
  }

  float percentChanged = (changedPixels * 100.0) / frameSize;
  memcpy(prevFrame, fb->buf, frameSize);
  esp_camera_fb_return(fb);

  if (percentChanged > MOTION_TRIGGER_PERCENT) {
    runCaptureBurst();
  }
}