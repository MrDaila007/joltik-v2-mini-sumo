// Joltik V2 - Mini Sumo Competition Firmware
// RP2040-Zero based board with TA6586 motor driver
// 4x digital opponent sensors, 2x analog line sensors
// WS2812 RGB LED, RC5 IR remote, servo, optional OLED

#include <RC5.h>
#include "GyverButton.h"
#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <Wire.h>

// Uncomment to enable OLED display (I2C 0.91" 128x32 SSD1306, SDA=GPIO2, SCL=GPIO3)
//#define USE_DISPLAY

// Uncomment to enable serial debug output
//#define DEBUG_ENABLE

#ifdef USE_DISPLAY
  #include <GyverOLED.h>
  GyverOLED<SSD1306_128x32, OLED_BUFFER> oled;
#endif

// ==================== Pin Definitions ====================
// Motor Driver (TA6586 2-wire)
#define AIN_1 5
#define AIN_2 6
#define BIN_1 7
#define BIN_2 8

// Opponent Sensors (digital, active-low)
#define SEN_FR 13   // Front right
#define SEN_FL 12   // Front left
#define SEN_R  11   // Side right
#define SEN_L  10   // Side left

// Line / Edge Sensors (analog)
#define SEN_Line_R A2
#define SEN_Line_L A3

// Peripherals
#define SERVO_PIN 9
#define BTN_PIN   15
#define IR_PIN    14
#define RGB_LED   16  // WS2812 x1

// ==================== Speed Constants ====================
#define MAX_SPEED          200
#define STRAIGHT_SPEED     125
#define SEARCH_SPEED       100
#define ROTATE_TANK_SPEED  125
#define ROTATE_SPEED       125
#define BREAKOUT_SPEED     160
#define ATTACK_SPEED       125

// ==================== Direction ====================
#define LEFT  0
#define RIGHT 1

// ==================== Objects ====================
Adafruit_NeoPixel pixels(1, RGB_LED, NEO_GRB + NEO_KHZ800);
RC5 rc5(IR_PIN);
GButton butt1(BTN_PIN);
Servo plow;

// ==================== Colors ====================
#define COLOR_OFF     pixels.Color(0, 0, 0)
#define COLOR_WHITE   pixels.Color(255, 255, 255)
#define COLOR_RED     pixels.Color(255, 0, 0)
#define COLOR_GREEN   pixels.Color(0, 255, 0)
#define COLOR_BLUE    pixels.Color(0, 0, 255)
#define COLOR_YELLOW  pixels.Color(255, 255, 0)
#define COLOR_ORANGE  pixels.Color(255, 80, 0)

// ==================== Global State ====================
uint8_t searchDir = LEFT;
int count = 0;
bool running = false;
uint8_t currentTactic = 0;
const uint8_t TACTIC_COUNT = 4;
String lastIRCommand = "None";

// Line sensor threshold (above = on ring, below = edge/white)
#define LINE_THRESHOLD 500

// ==================== Sensor Functions ====================
// All opponent sensors are active-low: LOW = opponent detected

bool statusFR() {
  return !digitalRead(SEN_FR);
}

bool statusFL() {
  return !digitalRead(SEN_FL);
}

bool statusR() {
  return !digitalRead(SEN_R);
}

bool statusL() {
  return !digitalRead(SEN_L);
}

// "Front" = both front sensors see opponent
bool statusFront() {
  return statusFL() && statusFR();
}

// Any opponent sensor triggered
bool opponentDetected() {
  return statusFL() || statusFR() || statusL() || statusR();
}

// Line sensors: return true if on ring (dark surface, high reading)
bool lineSensL() {
  return analogRead(SEN_Line_L) > LINE_THRESHOLD;
}

bool lineSensR() {
  return analogRead(SEN_Line_R) > LINE_THRESHOLD;
}

// ==================== Motor Control ====================
void drive(int left, int right) {
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  if (left >= 0) {
    digitalWrite(AIN_2, LOW);
    analogWrite(AIN_1, abs(left));
  } else {
    digitalWrite(AIN_2, HIGH);
    analogWrite(AIN_1, abs(left));
  }

  if (right >= 0) {
    digitalWrite(BIN_1, LOW);
    analogWrite(BIN_2, abs(right));
  } else {
    digitalWrite(BIN_1, HIGH);
    analogWrite(BIN_2, abs(right));
  }
}

// ==================== LED Helpers ====================
void setLED(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
}

// ==================== Start Routine ====================
void startRoutine() {
  setLED(COLOR_GREEN);

  // Initial charge forward
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(150);

  drive(0, 0);
  delay(50);

  // Briefly search for opponent
  uint32_t startTimestamp = millis();
  while (!statusFront()) {
    if (millis() - startTimestamp > 200) {
      break;
    }
  }
}

// ==================== Back Off ====================
void backoff(uint8_t dir) {
  setLED(COLOR_RED);

  // Reverse
  drive(-BREAKOUT_SPEED, -BREAKOUT_SPEED);
#ifdef DEBUG_ENABLE
  Serial.println("Reverse");
#endif
  delay(200);

  // Stop briefly
  drive(0, 0);
  delay(50);

  // Rotate away from edge
  if (dir == LEFT) {
    drive(-ROTATE_SPEED, ROTATE_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Dir_Left");
#endif
  } else {
    drive(ROTATE_SPEED, -ROTATE_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Dir_Right");
#endif
  }
  delay(100);

  // Look for opponent during rotation
  uint32_t uTurnTimestamp = millis();
  while (millis() - uTurnTimestamp < 200) {
    if (opponentDetected()) {
      drive(0, 0);
      delay(50);
#ifdef DEBUG_ENABLE
      Serial.println("Backoff Trig");
#endif
      return;
    }
  }

  // If no opponent found, move forward
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(100);
}

// ==================== Search ====================
void search() {
  setLED(COLOR_GREEN);

  if (searchDir == LEFT) {
    drive(-SEARCH_SPEED, SEARCH_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Search_Left");
#endif
  } else {
    drive(SEARCH_SPEED, -SEARCH_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Search_Right");
#endif
  }
}

// ==================== Attack ====================
void attack() {
  setLED(COLOR_BLUE);
  uint32_t attackTimestamp = millis();

  // Check line sensors first
  bool lineL = lineSensL();
  bool lineR = lineSensR();

  if (!lineL && !lineR) {
    // Both edges detected - back off right
    backoff(RIGHT);
    return;
  }
  if (!lineL) {
    // Left edge detected - back off right (turn away from left edge)
    backoff(RIGHT);
    return;
  }
  if (!lineR) {
    // Right edge detected - back off left (turn away from right edge)
    backoff(LEFT);
    return;
  }

  // Priority 1: All four sensors - full speed ram
  if (statusFront() && statusL() && statusR()) {
    drive(MAX_SPEED + count, MAX_SPEED + count);
    count++;
#ifdef DEBUG_ENABLE
    Serial.println("Attack ALL Max Speed");
#endif
  }
  // Priority 2: Both front sensors - straight attack
  else if (statusFront()) {
    drive(ATTACK_SPEED, ATTACK_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front");
#endif
  }
  // Priority 3: Front-left only - slight left turn
  else if (statusFL()) {
    searchDir = LEFT;
    drive(ATTACK_SPEED - 30, ATTACK_SPEED + 10);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front-Left");
#endif
  }
  // Priority 4: Front-right only - slight right turn
  else if (statusFR()) {
    searchDir = RIGHT;
    drive(ATTACK_SPEED + 10, ATTACK_SPEED - 30);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front-Right");
#endif
  }
  // Priority 5: Side-left only - rotate left
  else if (statusL()) {
    searchDir = LEFT;
    drive(-ROTATE_SPEED, ROTATE_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Side-Left");
#endif
    while (!statusFront()) {
      if (millis() - attackTimestamp > 200) break;
    }
  }
  // Priority 6: Side-right only - rotate right
  else if (statusR()) {
    searchDir = RIGHT;
    drive(ROTATE_SPEED, -ROTATE_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Side-Right");
#endif
    while (!statusFront()) {
      if (millis() - attackTimestamp > 200) break;
    }
  }
}

// ==================== Tactics ====================
void tactics(uint8_t mode) {
  // Placeholder for different starting strategies
  switch (mode) {
    case 0: // Default: straight charge
      startRoutine();
      break;
    case 1: // Reverse start
      drive(-MAX_SPEED + 10, -MAX_SPEED);
      delay(400);
      drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED);
      delay(150);
      drive(0, 0);
      delay(50);
      startRoutine();
      break;
    case 2: // Left sweep
      drive(-SEARCH_SPEED, SEARCH_SPEED);
      delay(300);
      startRoutine();
      break;
    case 3: // Right sweep
      drive(SEARCH_SPEED, -SEARCH_SPEED);
      delay(300);
      startRoutine();
      break;
  }
}

// ==================== Stop (blink LED) ====================
void stopRobot() {
  running = false;
  drive(0, 0);
  count = 0;
#ifdef DEBUG_ENABLE
  Serial.println("STOPPED");
#endif

  // Blink white/off to indicate stopped state
  while (!running) {
    butt1.tick();
    if (butt1.isSingle()) {
      running = true;
      return;
    }
    if (butt1.isDouble()) {
      currentTactic = (currentTactic + 1) % TACTIC_COUNT;
#ifdef DEBUG_ENABLE
      Serial.print("Tactic: ");
      Serial.println(currentTactic);
#endif
      // Flash tactic number
      for (uint8_t i = 0; i <= currentTactic; i++) {
        setLED(COLOR_YELLOW);
        delay(150);
        setLED(COLOR_OFF);
        delay(150);
      }
    }

    setLED(COLOR_WHITE);
    delay(500);
    setLED(COLOR_OFF);
    delay(500);
  }
}

// ==================== IR Remote (Core 1) ====================
void loop1() {
  processIR();
}

void processIR() {
  unsigned char toggle, address, command;
  if (rc5.read(&toggle, &address, &command)) {
    lastIRCommand = "A:" + String(address) + " C:" + String(command);
#ifdef DEBUG_ENABLE
    Serial.println("IR: " + lastIRCommand);
#endif

    // Map IR commands
    switch (command) {
      case 1: // Tactic 0
      case 2: // Tactic 1
      case 3: // Tactic 2
      case 4: // Tactic 3
        currentTactic = command - 1;
        // Flash to confirm
        setLED(COLOR_ORANGE);
        delay(100);
        break;
      case 12: // Power/Start-Stop toggle
        if (running) {
          stopRobot();
        } else {
          running = true;
        }
        break;
    }
  }
}

// ==================== OLED Display ====================
#ifdef USE_DISPLAY
void updateDisplay() {
  oled.clear();

  bool fr = statusFR();
  bool fl = statusFL();
  bool r = statusR();
  bool l = statusL();
  int lineRVal = analogRead(SEN_Line_R);
  int lineLVal = analogRead(SEN_Line_L);

  // Row 0: Sensor rectangles
  drawSensorRect(0, 0, fl);    // Front-Left
  drawSensorRect(34, 0, fr);   // Front-Right
  drawSensorRect(68, 0, l);    // Side-Left
  drawSensorRect(102, 0, r);   // Side-Right

  // Row 2: Line sensor values
  oled.setCursor(0, 2);
  oled.print("L:");
  oled.print(lineLVal);
  oled.print(" R:");
  oled.print(lineRVal);

  // Row 3: Tactic and state
  oled.setCursor(0, 3);
  oled.print("T:");
  oled.print(currentTactic);
  oled.print(running ? " RUN" : " STOP");

  // IR command
  oled.setCursor(70, 3);
  oled.print(lastIRCommand);

  oled.update();
}

void drawSensorRect(int x, int y, bool filled) {
  if (filled) {
    oled.rect(x, y, x + 30, y + 7, OLED_FILL);
  } else {
    oled.rect(x, y, x + 30, y + 7, OLED_STROKE);
  }
}
#endif

// ==================== Setup ====================
void setup() {
#ifdef DEBUG_ENABLE
  Serial.begin(115200);
#endif

  // Motor pins
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);

  // Opponent sensor pins (active-low, need pullup)
  pinMode(SEN_FR, INPUT);
  pinMode(SEN_FL, INPUT);
  pinMode(SEN_R, INPUT);
  pinMode(SEN_L, INPUT);

  // Button setup
  butt1.setDebounce(50);
  butt1.setTimeout(300);
  butt1.setClickTimeout(600);
  butt1.setType(HIGH_PULL);
  butt1.setDirection(NORM_OPEN);

  // NeoPixel
  pixels.begin();
  pixels.setBrightness(50);
  setLED(COLOR_WHITE);

  // Servo
  plow.attach(SERVO_PIN);
  plow.write(90); // neutral position

  // OLED
#ifdef USE_DISPLAY
  oled.init();
  oled.clear();
  oled.setScale(1);
  oled.home();
  oled.print("Joltik V2 Ready");
  oled.update();
#endif

  // Stop motors
  drive(0, 0);

  // ---- Wait for start (button press) ----
  setLED(COLOR_WHITE);

#ifdef DEBUG_ENABLE
  Serial.println("Waiting for start...");
#endif

  bool started = false;
  while (!started) {
    butt1.tick();

    // Single click to start
    if (butt1.isSingle()) {
      started = true;
      break;
    }

    // Double click to cycle tactics while waiting
    if (butt1.isDouble()) {
      currentTactic = (currentTactic + 1) % TACTIC_COUNT;
#ifdef DEBUG_ENABLE
      Serial.print("Tactic selected: ");
      Serial.println(currentTactic);
#endif
      // Flash tactic number
      for (uint8_t i = 0; i <= currentTactic; i++) {
        setLED(COLOR_YELLOW);
        delay(150);
        setLED(COLOR_OFF);
        delay(150);
      }
      setLED(COLOR_WHITE);
    }

#ifdef USE_DISPLAY
    updateDisplay();
#endif

#ifdef DEBUG_ENABLE
    // Print sensor debug info periodically
    static uint32_t lastDebug = 0;
    if (millis() - lastDebug > 500) {
      lastDebug = millis();
      Serial.print("FL:");
      Serial.print(statusFL());
      Serial.print(" FR:");
      Serial.print(statusFR());
      Serial.print(" L:");
      Serial.print(statusL());
      Serial.print(" R:");
      Serial.print(statusR());
      Serial.print(" LineL:");
      Serial.print(analogRead(SEN_Line_L));
      Serial.print(" LineR:");
      Serial.print(analogRead(SEN_Line_R));
      Serial.print(" Tactic:");
      Serial.println(currentTactic);
    }
#endif
  }

  // Start!
  running = true;
  setLED(COLOR_GREEN);

  // 5-second countdown (competition rules)
  for (int i = 5; i > 0; i--) {
#ifdef DEBUG_ENABLE
    Serial.println(i);
#endif
#ifdef USE_DISPLAY
    oled.clear();
    oled.setScale(3);
    oled.setCursor(50, 0);
    oled.print(i);
    oled.update();
#endif
    setLED(i % 2 ? COLOR_GREEN : COLOR_OFF);
    delay(1000);
  }
  setLED(COLOR_GREEN);

#ifdef USE_DISPLAY
  oled.clear();
  oled.setScale(1);
  oled.home();
  oled.print("GO!");
  oled.update();
#endif

  // Execute selected tactic start routine
  tactics(currentTactic);
}

// ==================== Main Loop ====================
void loop() {
  butt1.tick();

  // Button press stops the robot
  if (butt1.isSingle()) {
    stopRobot();
    // When stopRobot returns, we were restarted
    tactics(currentTactic);
    return;
  }

  // Check line sensors (always active)
  bool lineL = lineSensL();
  bool lineR = lineSensR();

  if (!lineL && !lineR) {
    backoff(RIGHT);
    return;
  }
  if (!lineL) {
    // Left edge - back off turning right
    backoff(RIGHT);
    return;
  }
  if (!lineR) {
    // Right edge - back off turning left
    backoff(LEFT);
    return;
  }

  // No opponent detected - search
  if (!opponentDetected()) {
    search();
    count = 0;
#ifdef DEBUG_ENABLE
    Serial.println("Searching");
#endif
  }
  // Opponent detected - attack
  else {
    attack();
#ifdef DEBUG_ENABLE
    Serial.println("Attacking");
#endif
  }

#ifdef USE_DISPLAY
  static uint32_t lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 100) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
#endif
}
