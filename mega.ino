#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <NewPing.h>
#include <ArduinoJson.h>

// Hardware pins
const uint8_t TRIG_PIN = 50;
const uint8_t ECHO_PIN = 52;
const uint8_t BUZZER_PIN = 46;
const uint8_t SERVO_PIN = 48;
const uint8_t RGB_RED_PIN = 3;
const uint8_t RGB_GREEN_PIN = 4;
const uint8_t RGB_BLUE_PIN = 5;
const uint8_t START_BTN_PIN = 2;
const uint8_t JOYSTICK_X_PIN = A0;
const uint8_t JOYSTICK_Y_PIN = A1;
const uint8_t SPEED_POT_PIN = A2;

const uint8_t OLED_WIDTH = 128;
const uint8_t OLED_HEIGHT = 64;

// Libraries
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Servo droneServo;
NewPing sonar(TRIG_PIN, ECHO_PIN, 400);

// JSON buffer for UART
StaticJsonDocument<128> sensorDoc;

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;) {
      delay(10);
    }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void updateRgb(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_RED_PIN, r);
  analogWrite(RGB_GREEN_PIN, g);
  analogWrite(RGB_BLUE_PIN, b);
}

void updateOled(int joystickX, int joystickY, int speedPot, int distance) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("JOY X:");
  display.println(joystickX);
  display.print("JOY Y:");
  display.println(joystickY);
  display.print("SPEED:");
  display.println(speedPot);
  display.print("DIST:");
  display.println(distance);
  display.display();
}

void sendSensorReadings() {
  int joystickX = analogRead(JOYSTICK_X_PIN);
  int joystickY = analogRead(JOYSTICK_Y_PIN);
  int speedPot = analogRead(SPEED_POT_PIN);
  int distance = sonar.ping_cm();
  int servoAngle = map(joystickX, 0, 1023, 0, 180);

  sensorDoc["joystickX"] = joystickX;
  sensorDoc["joystickY"] = joystickY;
  sensorDoc["speed"] = speedPot;
  sensorDoc["distance"] = distance;
  sensorDoc["servo"] = servoAngle;
  sensorDoc["status"] = (digitalRead(START_BTN_PIN) == HIGH) ? "armed" : "idle";

  serializeJson(sensorDoc, Serial3);
  Serial3.println();

  droneServo.write(servoAngle);
  updateOled(joystickX, joystickY, speedPot, distance);
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(START_BTN_PIN, INPUT_PULLUP);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  Serial3.begin(9600);
  initDisplay();
  droneServo.attach(SERVO_PIN);
  updateRgb(0, 255, 0);

  display.setCursor(0, 0);
  display.println("Drone Sensor Node");
  display.println("UART: OK");
  display.display();
}

void loop() {
  sendSensorReadings();
  delay(100);
}