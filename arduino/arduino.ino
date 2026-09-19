#include <Servo.h>

Servo handServo;

#define SERVO_PIN 9
#define IDLE_ANGLE 90
#define ROCK_ANGLE 90
#define PAPER_ANGLE 0
#define SCISSORS_ANGLE 180

void setup() {
  Serial.begin(115200);
  handServo.attach(SERVO_PIN);
  handServo.write(IDLE_ANGLE);
  Serial.println("Arduino ready. Waiting for ESP32-CAM prediction...");
}

void loop() {
  if (Serial.available()) {
    String received = Serial.readStringUntil('\n');
    received.trim();  // remove whitespace/newline characters

    if (received.length() == 0) {
      return;  // ignore empty lines
    }

    Serial.print("Received: ");
    Serial.println(received);

    int targetAngle = IDLE_ANGLE;

    if (received == "ROCK") {
      targetAngle = ROCK_ANGLE;
      Serial.println("Player: ROCK -> Machine shows: PAPER");
      targetAngle = PAPER_ANGLE;  // machine's counter-move
    } else if (received == "PAPER") {
      Serial.println("Player: PAPER -> Machine shows: SCISSORS");
      targetAngle = SCISSORS_ANGLE;
    } else if (received == "SCISSORS") {
      Serial.println("Player: SCISSORS -> Machine shows: ROCK");
      targetAngle = ROCK_ANGLE;
    } else {
      Serial.println("Unknown input, ignoring.");
      return;
    }

    handServo.write(targetAngle);
    delay(2000);  // hold the reveal position briefly

    handServo.write(IDLE_ANGLE);  // return to idle (Rock position)
  }
}
