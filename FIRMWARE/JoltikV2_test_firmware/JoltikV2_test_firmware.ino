#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <RC5.h>
#include <GyverButton.h>

#define DEBUG true  // Enable debug output
#define UseGyverMotor 1 // 0 - Simple Drive, 1 - GyverMotor

#define ReverseMotorL 1
#define ReverseMotorR 1



// Pin definitions
#define AIN_1 5    // Left motor (direction 1)
#define AIN_2 6    // Left motor (direction 2)
#define BIN_1 7    // Right motor (direction 1)
#define BIN_2 8    // Right motor (direction 2)

#define SEN_Line_R A2  // Right line sensor
#define SEN_Line_L A3  // Left line sensor
#define SEN_FR 13      // Front right sensor (45°)
#define SEN_FL 12      // Front left sensor (45°)
#define SEN_R 11       // Right side sensor
#define SEN_L 10       // Left side sensor
#define BTN_PIN 15     // Start button
#define RGB_led 16     // WS2812 LED
#define IR_PIN 26      // IR receiver pin

Adafruit_NeoPixel pixel(1, RGB_led, NEO_GRB + NEO_KHZ800);
RC5 rc5(IR_PIN);
GButton button(BTN_PIN);

#if (UseGyverMotor == 1)
#include <GyverMotor2.h>
GMotor2<DRIVER2WIRE_PWM> motorR(BIN_1, BIN_2);
GMotor2<DRIVER2WIRE_PWM> motorL(AIN_1, AIN_2);

#endif

// Robot settings
const int BASE_SPEED = 150;
const int TURN_SPEED = 200;
const int REVERSE_TIME = 150;
const int TURN_TIME = 200;
const int LINE_THRESHOLD = 500;

// Robot states
enum State {
  WAITING,      // Waiting for start
  SEARCHING,    // Searching for opponent
  ATTACKING,    // Attacking
  AVOIDING      // Avoiding line
};

// Start modes
enum StartMode {
  FACE_TO_FACE,
  BACK_TO_BACK
};

// Settings
struct Settings {
  byte dohyoCommand;
  StartMode startMode;
  bool lineSensorEnabled;
};

State robotState = WAITING;
Settings settings = {18, FACE_TO_FACE, true};
unsigned long lastActionTime = 0;

// RC5 variables
const byte PROG_ADDRESS = 0x0B;
byte stopCommand = 0;
byte startCommand = 0;
bool emergencyStop = false;

// Read opponent sensor with inversion
bool readOpponentSensor(int pin) {
  return !digitalRead(pin); // Invert value
}

void setup() {
  Serial.begin(115200);
  if(DEBUG) Serial.println("Initializing...");
  

  #if (UseGyverMotor == 1)
    #if (ReverseMotorL== 1)
      motorL.reverse(1);   // min. PWM
    #endif
    #if (ReverseMotorR== 1)
      motorR.reverse(1);   // min. PWM
    #endif
  motorR.setMinDuty(70);   // min. PWM
  motorL.setMinDuty(70);   // min. PWM
  #endif



  #if (UseGyverMotor == 0)
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);
  #endif

  
  // Initialize sensors
  pinMode(SEN_FR, INPUT_PULLUP);
  pinMode(SEN_FL, INPUT_PULLUP);
  pinMode(SEN_R, INPUT_PULLUP);
  pinMode(SEN_L, INPUT_PULLUP);
  pinMode(SEN_Line_R, INPUT);
  pinMode(SEN_Line_L, INPUT);
  
  // Button setup
  button.setDebounce(50);
  button.setTimeout(300);
  button.setClickTimeout(600);
  button.setType(HIGH_PULL);
  button.setDirection(NORM_OPEN);
  
  // Initialize LED
  pixel.begin();
  pixel.clear();
  
  // Initialize EEPROM
  EEPROM.begin(512);
  loadSettings();
  
  // Calculate commands
  stopCommand = settings.dohyoCommand;
  startCommand = (settings.dohyoCommand & 0xFE) + 1;
  
  if(DEBUG) {
    Serial.println("Settings loaded:");
    Serial.print("STOP command: "); Serial.println(stopCommand);
    Serial.print("START command: "); Serial.println(startCommand);
    Serial.print("Start mode: ");
    Serial.println(settings.startMode == FACE_TO_FACE ? "FACE_TO_FACE" : "BACK_TO_BACK");
    Serial.print("Line sensors: ");
    Serial.println(settings.lineSensorEnabled ? "ON" : "OFF");
  }
  
  setColor(0, 0, 255); // Blue - waiting
  pixel.show();
}

void loop() {
  button.tick();
  if(handleButtonCommands()) return;
  if(processRemoteCommands()) return;
  if(emergencyStop) return;
  
  if(robotState != WAITING) {
    if(DEBUG) printDebugInfo();
    
    switch(robotState) {
      case SEARCHING: handleSearchingState(); break;
      case ATTACKING: handleAttackingState(); break;
      case AVOIDING: handleAvoidingState(); break;
      default: break;
    }
    
    if(settings.lineSensorEnabled && checkLine()) {
      if(DEBUG) Serial.println("Line detected!");
      avoidLine();
    }
  }
}

void printDebugInfo() {
  static unsigned long lastDebug = 0;
  if(millis() - lastDebug > 500) {
    lastDebug = millis();
    Serial.print("Sensors: FR="); Serial.print(readOpponentSensor(SEN_FR));
    Serial.print(" FL="); Serial.print(readOpponentSensor(SEN_FL));
    Serial.print(" R="); Serial.print(readOpponentSensor(SEN_R));
    Serial.print(" L="); Serial.print(readOpponentSensor(SEN_L));
    Serial.print(" LineR="); Serial.print(analogRead(SEN_Line_R));
    Serial.print(" LineL="); Serial.println(analogRead(SEN_Line_L));

    Serial.print("State: ");
    switch(robotState) {
      case WAITING: Serial.println("WAITING"); break;
      case SEARCHING: Serial.println("SEARCHING"); break;
      case ATTACKING: Serial.println("ATTACKING"); break;
      case AVOIDING: Serial.println("AVOIDING LINE"); break;
    }
  }
}

bool handleButtonCommands() {
  if(button.isDouble()) {
    settings.startMode = (settings.startMode == FACE_TO_FACE) ? BACK_TO_BACK : FACE_TO_FACE;
    if(DEBUG) Serial.println("Start mode changed");
    blinkColor(0, 255, 255, 3);
    saveSettings();
    return false;
  }
  
  if(button.isHolded()) {
    settings.lineSensorEnabled = !settings.lineSensorEnabled;
    if(DEBUG) {
      Serial.print("Line sensors: ");
      Serial.println(settings.lineSensorEnabled ? "ON" : "OFF");
    }
    blinkColor(settings.lineSensorEnabled ? 0 : 255, 
               settings.lineSensorEnabled ? 255 : 0, 
               0, 2);
    saveSettings();
    return false;
  }
  
  if(button.isSingle()) {
    if(robotState == WAITING) {
      if(DEBUG) Serial.println("Starting from button");
      startWithDelay();
    } else {
      if(DEBUG) Serial.println("Stopping from button");
      emergencyStopRobot();
    }
    return true;
  }
  return false;
}

bool processRemoteCommands() {
  unsigned char toggle, address, command;
  if (rc5.read(&toggle, &address, &command)) {
    if(DEBUG) {
      Serial.print("IR command: address=0x"); Serial.print(address, HEX);
      Serial.print(" command=0x"); Serial.println(command, HEX);
    }
    
    if(address == PROG_ADDRESS) {
      if(settings.dohyoCommand != command) {
        settings.dohyoCommand = command;
        stopCommand = settings.dohyoCommand;
        startCommand = (settings.dohyoCommand & 0xFE) + 1;
        if(DEBUG) Serial.println("New dohyo command saved");
        saveSettings();
        blinkColor(0, 255, 255, 3);
      }
      return false;
    }
    
    if(command == startCommand && robotState == WAITING) {
      if(DEBUG) Serial.println("Starting via IR");
      startRobot();
      return false;
    }
    
    if(command == stopCommand) {
      if(DEBUG) Serial.println("Stopping via IR");
      emergencyStopRobot();
      return true;
    }
  }
  return false;
}

void handleSearchingState() {
  if(settings.startMode == FACE_TO_FACE) {
    drive(100, -100); // Slow spin
  } else {
    drive(-100, 100); // Spin in opposite direction
  }
  
  if(readOpponentSensor(SEN_FR) || readOpponentSensor(SEN_FL)) {
    robotState = ATTACKING;
    setColor(255, 0, 0);
    if(DEBUG) Serial.println("Opponent detected ahead");
  } else if(readOpponentSensor(SEN_R)) {
    drive(150, 50);
    if(DEBUG) Serial.println("Opponent on right");
  } else if(readOpponentSensor(SEN_L)) {
    drive(50, 150);
    if(DEBUG) Serial.println("Opponent on left");
  }
}

void handleAttackingState() {
  bool frontRight = readOpponentSensor(SEN_FR);
  bool frontLeft = readOpponentSensor(SEN_FL);
  bool right = readOpponentSensor(SEN_R);
  bool left = readOpponentSensor(SEN_L);
  
  if(frontRight && frontLeft) {
    drive(BASE_SPEED, BASE_SPEED);
    if(DEBUG) Serial.println("Attacking straight");
  } else if(frontRight) {
    drive(BASE_SPEED, BASE_SPEED/2);
    if(DEBUG) Serial.println("Attacking with right offset");
  } else if(frontLeft) {
    drive(BASE_SPEED/2, BASE_SPEED);
    if(DEBUG) Serial.println("Attacking with left offset");
  } else if(right) {
    drive(TURN_SPEED, -TURN_SPEED);
    if(DEBUG) Serial.println("Turning left");
  } else if(left) {
    drive(-TURN_SPEED, TURN_SPEED);
    if(DEBUG) Serial.println("Turning right");
  } else {
    robotState = SEARCHING;
    setColor(0, 255, 0);
    if(DEBUG) Serial.println("Opponent lost, returning to search");
  }
}

void handleAvoidingState() {
  if(millis() - lastActionTime > REVERSE_TIME + TURN_TIME) {
    robotState = SEARCHING;
    setColor(0, 255, 0);
    if(DEBUG) Serial.println("Line avoidance complete");
  }
}

bool checkLine() {
  int lineRight = analogRead(SEN_Line_R);
  int lineLeft = analogRead(SEN_Line_L);
  return (lineRight > LINE_THRESHOLD || lineLeft > LINE_THRESHOLD);
}

void avoidLine() {
  robotState = AVOIDING;
  lastActionTime = millis();
  setColor(255, 255, 0);
  
  drive(-BASE_SPEED, -BASE_SPEED);
  delay(REVERSE_TIME);
  
  int lineRight = analogRead(SEN_Line_R);
  int lineLeft = analogRead(SEN_Line_L);
  
  if(lineRight > lineLeft) {
    drive(-BASE_SPEED, BASE_SPEED);
  } else {
    drive(BASE_SPEED, -BASE_SPEED);
  }
}

void emergencyStopRobot() {
  robotState = WAITING;
  emergencyStop = true;
  MotorStop();
  setColor(0, 0, 255);
  
  if(DEBUG) Serial.println("!!! EMERGENCY STOP !!!");
  
  delay(100);
  emergencyStop = false;
}

void startWithDelay() {
  robotState = SEARCHING;
  emergencyStop = false;
  setColor(0, 255, 0);
  
  if(DEBUG) Serial.println("Countdown 5 sec...");
  
  for(int i = 5; i > 0; i--) {
    if(checkEmergency()) return;
    setColor(0, 0, 255);
    if(DEBUG) Serial.println(i);
    delay(250);
    if(checkEmergency()) return;
    setColor(0, 0, 0);
    delay(250);
  }
  
  if(settings.startMode == BACK_TO_BACK) {
    if(DEBUG) Serial.println("Starting backwards");
    drive(-BASE_SPEED, -BASE_SPEED);
    delay(300);
  } else {
    if(DEBUG) Serial.println("Starting forward");
  }
}

void startRobot() {
  robotState = SEARCHING;
  emergencyStop = false;
  setColor(0, 255, 0);
  
  if(settings.startMode == BACK_TO_BACK) {
    drive(-BASE_SPEED, -BASE_SPEED);
    delay(300);
  }
}

bool checkEmergency() {
  button.tick();
  if(button.isSingle()) {
    emergencyStopRobot();
    return true;
  }
  
  unsigned char toggle, address, command;
  if(rc5.read(&toggle, &address, &command)) {
    if(command == stopCommand) {
      emergencyStopRobot();
      return true;
    }
  }
  
  return false;
}

void loadSettings() {
  EEPROM.get(0, settings);
  if(settings.dohyoCommand == 0xFF) {
    settings.dohyoCommand = 18;
    settings.startMode = FACE_TO_FACE;
    settings.lineSensorEnabled = true;
  }
}

void saveSettings() {
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void MotorStop() {
  #if (UseGyverMotor == 0)
  digitalWrite(AIN_1, LOW);
  digitalWrite(AIN_2, LOW);
  digitalWrite(BIN_1, LOW);
  digitalWrite(BIN_2, LOW);
  #endif
  #if (UseGyverMotor == 1)
  motorL.brake();
  motorR.brake();
  #endif
}

void drive(int leftSpeed, int rightSpeed) {

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  
  #if (UseGyverMotor == 1)
  motorR.setSpeed(rightSpeed);
  motorL.setSpeed(leftSpeed);
  #endif

  #if (UseGyverMotor == 0)
  // Left motor
    #if (ReverseMotorL == 1)
      leftSpeed = leftSpeed * (-1);   // min. PWM
    #endif
    #if (ReverseMotorR == 1)
      rightSpeed = rightSpeed * (-1);   // min. PWM
    #endif
  if(leftSpeed >= 0) {
    digitalWrite(AIN_1, HIGH);
    digitalWrite(AIN_2, LOW);
  } else {
    digitalWrite(AIN_1, LOW);
    digitalWrite(AIN_2, HIGH);
  }
  analogWrite(AIN_1, abs(leftSpeed));
  
  // Right motor
  if(rightSpeed >= 0) {
    digitalWrite(BIN_1, LOW);
    digitalWrite(BIN_2, HIGH);
  } else {
    digitalWrite(BIN_1, HIGH);
    digitalWrite(BIN_2, LOW);
  }
  analogWrite(BIN_2, abs(rightSpeed));

  #endif
}

void setColor(int r, int g, int b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void blinkColor(int r, int g, int b, int count) {
  for(int i = 0; i < count; i++) {
    setColor(r, g, b);
    delay(200);
    setColor(0, 0, 0);
    delay(200);
  }
  if(robotState == WAITING) setColor(0, 0, 255);
  else if(robotState == SEARCHING) setColor(0, 255, 0);
  else if(robotState == ATTACKING) setColor(255, 0, 0);
  else setColor(255, 255, 0);
}