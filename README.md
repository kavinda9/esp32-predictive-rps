# ESP32-CAM Predictive Rock-Paper-Scissors

A standalone Rock-Paper-Scissors machine built with an **ESP32-CAM** and an **Arduino Uno**. The system detects hand motion, captures a frame, classifies the gesture using an on-device TensorFlow Lite Micro model, and reveals a winning counter-move on a servo-driven hand — all running directly on the ESP32-CAM, with no laptop or cloud involved during gameplay.

<p align="center">
  <!-- Add your demo GIF or video link here -->
  <!-- <img src="docs/demo.gif" width="500" /> -->
</p>

---

## ⚠️ Important note before you try this yourself

<span style="color:red"><strong>The included <code>rps_model</code> was trained entirely on my own hand, my own camera, and my own lighting setup. It is very unlikely to work well for you out of the box — environment matters a lot for this kind of model.</strong></span>

Camera angle, background, lighting, hand size, and skin tone all affect accuracy. If you want this to work reliably for you, you'll need to **collect your own dataset and retrain the model** using your own setup — see [Training your own model](#training-your-own-model) below.

---

## How it works

```
Hand motion → Timer (~3s) → Capture frame → Trained model → Predicted gesture
                                                                    ↓
                                                          Compute counter-move
                                                                    ↓
                                                    Arduino + Servo reveals it
```

1. **Motion detection** — the ESP32-CAM continuously compares consecutive grayscale frames. When enough pixels change at once, it registers the start of a hand motion.
2. **Timed capture** — after a fixed delay, it captures a single frame.
3. **Classification** — that frame is fed into a small CNN (trained on Rock/Paper/Scissors images, converted to TensorFlow Lite Micro, quantized to int8) running entirely on the ESP32-CAM.
4. **Result handoff** — the predicted gesture is sent over serial to an Arduino Uno.
5. **Physical reveal** — the Arduino computes the winning counter-move and drives a servo to display it.

---

## Hardware

- ESP32-CAM (AI-Thinker, ESP-32S, OV2640 camera)
- ESP32-CAM-MB programmer/base board (for flashing)
- Arduino Uno (or compatible clone)
- SG90-style hobby servo
- Breadboard + jumper wires

## Wiring

- **ESP32-CAM TX (GPIO1)** → **Arduino RX (Pin 0)**
- **ESP32-CAM GND** → **Arduino GND**
- **Servo signal** → **Arduino Pin 9**
- **Servo power/GND** → **Arduino 5V / GND**
- ESP32-CAM is powered independently (via its programmer board or a separate 5V source) to avoid brownouts from sharing the Arduino's regulator with the servo.

See [`docs/wiring.md`](docs/wiring.md) for full details.

## Software / libraries

- Arduino IDE with ESP32 board support
- [`Chirale_TensorFlowLite`](https://github.com/Chirale/TensorFlowLite) — TensorFlow Lite Micro for Arduino/ESP32
- Python 3, TensorFlow, OpenCV, scikit-learn (for training, on your own machine — not required on the ESP32-CAM)

## Repository structure

```
esp32-predictive-rps/
├── esp32-cam/          # Final ESP32-CAM firmware (motion detection + inference)
│   └── tests/          # Development history / individual test sketches
├── arduino-uno/         # Arduino firmware (receives prediction, drives servo)
├── ai/
│   ├── dataset/         # Training images (rock/paper/scissors)
│   ├── prepare_dataset.py
│   ├── train_model.py
│   └── convert_model.py
├── docs/
│   ├── wiring.md
│   ├── project-plan.md
│   └── testing.md
└── hardware/
    └── components.md
```

## Training your own model

1. Flash the ESP32-CAM with a capture sketch and grab clear photos of your own Rock, Paper, and Scissors gestures via the `/capture` endpoint (aim for 200+ images per gesture, varying angle/distance/lighting).
2. Sort images into `ai/dataset/rock/`, `ai/dataset/paper/`, `ai/dataset/scissors/`.
3. Run:
   ```
   python prepare_dataset.py
   python train_model.py
   python convert_model.py
   ```
4. Copy the generated `rps_model.h` into `esp32-cam/`, re-flash, and test.

## Current limitations

- Trained on a small, single-person dataset — accuracy will vary a lot by environment (see warning above).
- Currently classifies the **final** hand pose, not an early/in-progress gesture — true early prediction is a planned next step.
- No "empty frame" class — the model always outputs one of the three gestures, even if no hand is present.

## License

This project is shared for learning/demo purposes. Feel free to fork and adapt — attribution appreciated but not required.
