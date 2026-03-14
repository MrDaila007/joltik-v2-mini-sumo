#include <RC5.h>
#include "GyverButton.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
// RP2040-Zero
// Раскомментируйте следующую строку, если используете дисплей
//#define USE_DISPLAY
// Display I2C 0.91inch 128x32 SSD1306 SDA GPIO2 SCL GPIO3
// Motor Driver TA6586
#ifdef USE_DISPLAY
  #include <GyverOLED.h>
  GyverOLED<SSD1306_128x32, OLED_BUFFER> oled;
#endif

#define AIN_1 5
#define AIN_2 6
#define BIN_1 7 
#define BIN_2 8 

#define SEN_Line_R A2
#define SEN_Line_L A3
#define SEN_FR 13
#define SEN_FL 12
#define SEN_R 11
#define SEN_L 10
#define Servo 9
#define BTN_PIN 15
#define IR_PIN 14
#define RGB_led 16 //ws2812 1шт.

Adafruit_NeoPixel pixels(1, RGB_led, NEO_GRB + NEO_KHZ800);
RC5 rc5(IR_PIN);
GButton butt1(BTN_PIN);

// Режимы работы моторов [левый, правый]
const int modes[][2] = {
  {0, 0},       // 0: оба мотора остановлены
  {100, 100},   // 1: оба мотора вперед
  {-100, -100}, // 2: оба мотора назад
  {-100, 100},  // 3: левый назад, правый вперед
  {100, -100}   // 4: левый вперед, правый назад
};

// Цвета для разных режимов (R,G,B)
const uint32_t modeColors[] = {
  pixels.Color(0, 0, 0),      // 0: выключен
  pixels.Color(0, 255, 0),    // 1: зеленый (вперед)
  pixels.Color(255, 0, 0),    // 2: красный (назад)
  pixels.Color(255, 165, 0),  // 3: оранжевый (разворот)
  pixels.Color(0, 0, 255)     // 4: синий (разворот)
};

int currentMode = 0;
const int modeCount = 5;
String lastIRCommand = "None";

void setup() {
  Serial.begin(9600);
  
  // Инициализация моторов
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);

  // Инициализация сенсоров
  pinMode(SEN_FR, INPUT);
  pinMode(SEN_FL, INPUT);
  pinMode(SEN_R, INPUT);
  pinMode(SEN_L, INPUT);

  // Настройка кнопки
  butt1.setDebounce(50);
  butt1.setTimeout(300);
  butt1.setClickTimeout(600);
  butt1.setType(HIGH_PULL);
  butt1.setDirection(NORM_OPEN);

  #ifdef USE_DISPLAY
    oled.init();
    oled.clear();
    oled.setScale(1);
    oled.home();
    oled.print("Display Ready");
    oled.update();
    delay(1000);
  #else
    Serial.println("Display disabled in code");
  #endif

  // Инициализация светодиода
  pixels.begin();
  pixels.clear();
  pixels.setPixelColor(0, modeColors[currentMode]);
  pixels.show();

  // Инициализация моторов
  drive(modes[currentMode][0], modes[currentMode][1]);
}

void loop() {
  butt1.tick();
  
  // Обработка смены режима
  if (butt1.isDouble()) {
    currentMode = (currentMode + 1) % modeCount;
    drive(modes[currentMode][0], modes[currentMode][1]);
    pixels.setPixelColor(0, modeColors[currentMode]);
    pixels.show();
  }

  #ifdef USE_DISPLAY
    updateDisplay();
  #endif
}

void loop1() {
  processIR();
}

void processIR() {
  unsigned char toggle, address, command;
  if (rc5.read(&toggle, &address, &command)) {
    lastIRCommand = "A:" + String(address) + " C:" + String(command);
    Serial.println("IR: " + lastIRCommand);
    
    // Мигание светодиодом при получении команды
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));
    pixels.show();
    delay(50);
    pixels.setPixelColor(0, modeColors[currentMode]);
    pixels.show();
  }
}

#ifdef USE_DISPLAY
void updateDisplay() {
  oled.clear();
  
  // Чтение цифровых сенсоров
  bool fr = !digitalRead(SEN_FR);
  bool fl = !digitalRead(SEN_FL);
  bool r = !digitalRead(SEN_R);
  bool l = !digitalRead(SEN_L);
  
  // Чтение аналоговых сенсоров
  int lineR = analogRead(SEN_Line_R);
  int lineL = analogRead(SEN_Line_L);
  
  // Отрисовка цифровых сенсоров (4 прямоугольника)
  drawSensorRect(0, 0, fr);   // SEN_FR
  drawSensorRect(34, 0, r);   // SEN_R
  drawSensorRect(68, 0, l);   // SEN_L
  drawSensorRect(102, 0, fl); // SEN_FL
  
  // Вывод аналоговых значений
  oled.setCursor(0, 2);
  oled.print("L:");
  oled.print(lineL);
  oled.print(" R:");
  oled.print(lineR);
  
  // Вывод режима моторов
  oled.setCursor(0, 3);
  oled.print("M:");
  oled.print(modes[currentMode][0]);
  oled.print(",");
  oled.print(modes[currentMode][1]);
  
  // Вывод последней IR команды
  oled.setCursor(70, 3);
  oled.print(lastIRCommand);
  
  oled.update();
}

void drawSensorRect(int x, int y, bool filled) {
  if (filled) {
    oled.rect(x, y, x+30, y+7, OLED_FILL);
  } else {
    oled.rect(x, y, x+30, y+7, OLED_STROKE);
  }
}
#endif

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