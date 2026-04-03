  // Wiring Connections.

  #include <SharpIR.h>
  #define model 20150


  #define LED0          13  // Onboard LED 0 dublicate on PBC
  #define LED1          4   // PBC LED 1
  #define Start_PIN     2   // Start module start pin
  //#define KILL_PIN      12  // Start module kill pin
  #define BUZZ          9   // Buzzer 
  #define BUTTON        8   // Start button
  #define EDGE_L        A3   // Left edge sensor
  #define EDGE_R        A3   // Right edge sensor
  #define EDGE_M        A3   // Right edge sensor
  #define OPPONENT_L    A5  // Left opponent sensor
  #define OPPONENT_R    A4  // Right opponent sensor
  #define OPPONENT_FR   A1  // Front right opponent sensor
  #define OPPONENT_FC   A2  // Front center opponent sensor
  #define OPPONENT_FL   A0  // Front left opponent sensor

//#define DEBUG_ENABLE

//#define Back_Start

#define UseGyverMotor 0 // 0 - Simple Drive, 1 - GyverMotor


/////////////////////

//#define useLineSen
//#define testAtHohe
/////////////////////
int disR;
int disL;
int sensorValue = 0;
int count = 0;


////////////////////////////////////
//////////// SPEED ////////////////
//////////////////////////////////
#define MAX_SPEED 200
#define STRAIGHT_SPEED 125
#define SEARCH_SPEED_1 100
#define SEARCH_SPEED_2 100
#define ROTATE_TANK_SPEED 125
#define ROTATE_SPEED 125
#define BRAKEOUT_SPEED 160
#define ATACK_SPEED 125

////////////////////////////////

bool mode = 0;        // Mode 0: For 2S
// Mode 1: For 3S


int difference = 0;
uint32_t myTimer1;
uint8_t attackZone = 0; //0-12

SharpIR Sharp_R(OPPONENT_FR, model);
SharpIR Sharp_L(OPPONENT_FL, model);

/*
  int PWMA = 5;
  int PWMB = 6;
  int BIN_1 = 11;
  int BIN_2 = 10;
  int AIN_1 = 9;
  int AIN_2 = 8;
*/




//#ifdef TA6586
#define AIN_1 10
#define AIN_2 11
#define BIN_1 6
#define BIN_2 3

#if (UseGyverMotor == 1)
#include <GyverMotor2.h>
GMotor2<DRIVER2WIRE_PWM> motorR(10, 11);
GMotor2<DRIVER2WIRE_PWM> motorL(6, 3);
#endif


// Direction.
#define LEFT    0
#define RIGHT   1

// Global variables.
uint8_t searchDir = LEFT;


// Configure the motor driver.


/*******************************************************************************
   Start Routine
   This function should be called once only when the game start.
 *******************************************************************************/
void startRoutine() {
  // Start delay.
#ifdef Back_Start

drive(-MAX_SPEED+10, -MAX_SPEED);
  // Turn right around 45 degress.
  delay(400);

  drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED);

  delay(150);
  drive(0,0);
  delay(50);

#endif
  
  // Go straight.
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(150);

  drive(0,0);
  delay(50);
  //drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED);
  //delay(150);
  // Turn left until opponent is detected.
  //drive(-0, ROTATE_TANK_SPEED);

  uint32_t startTimestamp = millis();
  while (StarusFr()) {
    // Quit if opponent is not found after timeout.
    if (millis() - startTimestamp > 200) {
      break;
    }
  }
  //drive(0,0);
  /*

while(true)
{
  blink(1);
  drive(0,0);
}
*/

}


/*******************************************************************************
   Back Off
   This function should be called when the ring edge is detected.
 *******************************************************************************/
void backoff(uint8_t dir) {
  // Stop the motors.

  // Reverse.
  drive(-BRAKEOUT_SPEED, -BRAKEOUT_SPEED);
#ifdef DEBUG_ENABLE
  Serial.println("Reverse");
#endif

  delay(200);

  // Stop the motors.
  drive(0, 0);

  delay(50);

  // Rotate..
  if (dir == LEFT) {
    drive(-ROTATE_SPEED, ROTATE_SPEED);

#ifdef DEBUG_ENABLE
    Serial.println("Dir_Left");
#endif


  } else  {
    drive(ROTATE_SPEED, -ROTATE_SPEED);

#ifdef DEBUG_ENABLE
    Serial.println("Dir_Right");
#endif
  }
  delay(100);

  // Start looking for opponent.
  // Timeout after a short period.
  uint32_t uTurnTimestamp = millis();
  while (millis() - uTurnTimestamp < 200) {
    // Opponent is detected if either one of the opponent sensor is triggered.
    if ( !StarusFr() ||
         !statusL() ||
         !statusR() ||
         digitalRead(OPPONENT_L) ||
         digitalRead(OPPONENT_R) ) {

      // Stop the motors.
      drive(0, 0);
      delay(50);

#ifdef DEBUG_ENABLE
      Serial.println("BrakeOut Trig");
#endif



      // Return to the main loop and run the attach program.
      return;
    }
  }

  // If opponent is not found, move forward and continue searching in the main loop..

  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);

  delay(100);
}


/*******************************************************************************
   Search
 *******************************************************************************/
void search() {
  // Move in circular motion.
  if (searchDir == LEFT) {
    drive(-SEARCH_SPEED_1, SEARCH_SPEED_1);
#ifdef DEBUG_ENABLE
    Serial.println("Search_Left");
#endif
  }
  else {
    drive(SEARCH_SPEED_1, -SEARCH_SPEED_1);
#ifdef DEBUG_ENABLE
    Serial.println("Search_Right");
#endif
  }


}


/*******************************************************************************
   Attack
   Track and attack the opponent in full speed.
   Do nothing if opponent is not found.
 *******************************************************************************/
void attack() {
  uint32_t attackTimestamp = millis();
  if (LineSens() == 0) {
    // Back off and make a U-Turn to the right.
    backoff(RIGHT);

  }

  // Opponent in front center.
  // Go straight in full speed.
  
  if (StarusFr() && statusL() && statusR()) {
    drive((MAX_SPEED + count), (MAX_SPEED + count));
    count++;
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front Max Speed");
#endif
  } 

  else if (StarusFr() && statusL()) {
    drive(ATACK_SPEED, (ATACK_SPEED + 10));
    searchDir = LEFT;
    #ifdef DEBUG_ENABLE
      Serial.println("Attack Front");
    #endif
  }
  else if (StarusFr() && statusR()) {
  drive((ATACK_SPEED + 10), ATACK_SPEED);
  searchDir = RIGHT;
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front");
#endif
  }
  else if (StarusFr()) {
  drive(ATACK_SPEED, ATACK_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front");
#endif
  }

  // Opponent in front left.
  // Turn left.
  else if (statusL()) {
    searchDir = LEFT;
    drive(0, ROTATE_TANK_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front left");
#endif
  }

  // Opponent in front right.
  // Turn right.
  else if (statusR()) {
    searchDir = RIGHT;
    drive(ROTATE_TANK_SPEED, 0);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front right");
#endif
  }

  // Opponent in left side.
  // Rotate left until opponent is in front.
  else if (!digitalRead(OPPONENT_L) == 1) {
    drive(-ROTATE_SPEED, ROTATE_SPEED);
    searchDir = LEFT;
#ifdef DEBUG_ENABLE
    Serial.println("Attack LEFT");
#endif
    while (StarusFr()) {
      // Quit if opponent is not found after timeout.
      if (millis() - attackTimestamp > 200) {
        break;
      }
    }
  }

  // Opponent in right side.
  // Rotate right until opponent is in front.
  else if (!digitalRead(OPPONENT_R) == 1) {
    drive(ROTATE_SPEED, -ROTATE_SPEED);
    searchDir = RIGHT;
#ifdef DEBUG_ENABLE
    Serial.println("Attack RIGHT");
#endif
    while (StarusFr()) {
      // Quit if opponent is not found after timeout.
      if (millis() - attackTimestamp > 200) {
        break;
      }
    }
  }
}




/*******************************************************************************
   Setup
   This function runs once after reset.
 *******************************************************************************/
void setup() {

#ifdef DEBUG_ENABLE
  Serial.begin(9600);
  //Serial.setTimeout(100);
#endif

#if (UseGyverMotor == 1)
motorR.setMinDuty(70);   // мин. ШИМ
motorL.setMinDuty(70);   // мин. ШИМ
#endif

  pinMode(Start_PIN, INPUT);
  //pinMode(KILL_PIN, INPUT);

pinMode(OPPONENT_FC, INPUT);
pinMode(OPPONENT_R, INPUT);
pinMode(OPPONENT_L, INPUT);


  pinMode(BUTTON, INPUT_PULLUP);
  //pinMode(EDGE_L, INPUT);
  //pinMode(EDGE_R, INPUT);

#if (UseGyverMotor == 0)
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);
#endif


  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);

  bool currentMode = 0;


  // Turn off the LEDs.
  // Those LEDs are active low.
  digitalWrite(LED0, 0);
  digitalWrite(LED1, 0);

  // Stop the motors.

  drive(0, 0);
  
  bool FRONT_State;
    bool FRONT_R_State;
    bool FRONT_L_State;
    bool RIGHT_State;
    bool LEFT_State;
    bool LINE_State;
    bool Button_State;

  while (!digitalRead(Start_PIN)) 
  {


    Button_State = digitalRead(BUTTON);

#ifdef DEBUG_ENABLE

    // While waiting, show the status of the edge sensor for easy calibration.
    if (!LineSens()) {
      digitalWrite(LED1, HIGH);
    } else {
      digitalWrite(LED1, LOW);
    }

    FRONT_State = StarusFr();
    FRONT_R_State = statusR();
    FRONT_L_State = statusL();
    RIGHT_State = !digitalRead(OPPONENT_R);
    LEFT_State = !digitalRead(OPPONENT_L);
    LINE_State = LineSens();
    Serial.print(disL);
    Serial.print(" ");
    Serial.print(FRONT_L_State);
    Serial.print(" ");
    Serial.print(FRONT_State);
    Serial.print(" ");
    Serial.print(FRONT_R_State);
    Serial.print(" ");
    Serial.print(disR);
    Serial.print(" ");
    Serial.print(LEFT_State);
    Serial.print(" ");
    Serial.print(RIGHT_State);
    Serial.print(" ");
    Serial.print(Button_State);
    Serial.print(" ");
    Serial.print(!LINE_State);
    Serial.print(" ");
    Serial.print(sensorValue);
    Serial.println(" ");
#endif

if(!Button_State)
    {
      break;
    }
  }

  // Wait until button is released.
  while (!digitalRead(Start_PIN))
  {
    Button_State = digitalRead(BUTTON);
    if(Button_State)
    {
      break;
    }
  }

  // Turn on the LEDs.
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);

  startRoutine();
}

/*******************************************************************************
   Main program loop.
 *******************************************************************************/
void loop() {
#ifdef useLineSen
    
  // Edge is detected on the left.
  if (LineSens() == 0) {
    // Back off and make a U-Turn to the right.
    backoff(RIGHT);

    // Toggle the search direction.
    //searchDir ^= 1;
  }

  // Edge is detected on the right.


  // Edge is not detected.
  
  else {

    // Keep searching if opponent is not detected.
#endif
/*
//bool useLineSen = 0;
  // Edge is detected on the left.
  if (LineSens() == 0) {
    // Back off and make a U-Turn to the right.
    backoff(RIGHT);

    // Toggle the search direction.
    searchDir ^= 1;
  }

  // Edge is detected on the right.


  // Edge is not detected.
  
  else {
   */
    // Keep searching if opponent is not detected.
    if ( !StarusFr() &&
         !statusL() &&
         !statusR() &&
         digitalRead(OPPONENT_L) &&
         digitalRead(OPPONENT_R) ) {
      search();
#ifdef DEBUG_ENABLE
      Serial.println("GO SEARCH void");
#endif

    }

    // Attack if opponent is in view.
    else {
      attack();
#ifdef DEBUG_ENABLE
      Serial.println("GO to Attack void");
#endif
    }
  
  
  // Stop the robot if the button is pressed.
  if (!digitalRead(Start_PIN) || !digitalRead(BUTTON)) 
  //if (!digitalRead(BUTTON)) 
  {
    // Stop the motors.
    drive(0, 0);


    digitalWrite(LED0, LOW);
    // Loop forever here.
    while (1) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, HIGH);
      delay(1000);
      digitalWrite(LED0, HIGH);
      digitalWrite(LED1, LOW);
      delay(1000);
    }
  
  }
  
}
#ifdef useLineSen
}
#endif




void drive(int left, int right) {


#if (UseGyverMotor == 1)
motorR.setSpeed(right);
motorL.setSpeed(left);
#endif

#if (UseGyverMotor == 0)
// функция для управления скоростью и направления вращения моторов
  if (left > 255)
    left = 255;
  if (right > 255)
    right = 255;

  left = constrain(left, -255, 255);   // жестко ограничиваем диапозон значений
  right = constrain(right, -255, 255); // жестко ограничиваем диапозон значений

  if (left >= 0)
  {
    digitalWrite(AIN_2, LOW);
    analogWrite(AIN_1, abs(left));
  }
  else
  {
    digitalWrite(AIN_2, HIGH);
    analogWrite(AIN_1, abs(left));
  }
  if (right >= 0)
  {
    digitalWrite(BIN_1, LOW);
    analogWrite(BIN_2, abs(right));
  }
  else
  {
    digitalWrite(BIN_1, HIGH);
    analogWrite(BIN_2, abs(right));
  }
#endif
  
}

bool statusL()
{
  bool statusLeft;
  disL  = Sharp_L.distance();
  if (disL < 80)
  {
    statusLeft = true;
  }
  else {
    statusLeft = false;
  }
  return statusLeft;

}
bool statusR()
{
  bool statusRight;
  disR = Sharp_R.distance();
  if (disR < 80)
  {
    statusRight = true;
  }
  else {
    statusRight = false;
  }
  return statusRight;

}
bool StarusFr(){
  bool State_FR = digitalRead(OPPONENT_FC);
  return(State_FR);
  //return(median(State_FR));
  }
// медиана на 3 значения со своим буфером
bool median(bool newVal) {
  static bool buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

void blink(uint8_t num)
{
  bool on = 0;
  uint8_t n = 0;
  for (int n = 0; n < num;  n++)
  {
    if (millis() - myTimer1 >= 300)
    { // ищем разницу (300 мс)
      myTimer1 = millis();              // сброс таймера
      if (on)
      {
        digitalWrite(LED0, 0);
      }
      else
      {
        digitalWrite(LED0, 1);
      }
    }
  }
}
bool LineSens()
{
  bool State = 1;
  sensorValue = analogRead(EDGE_L);
  if (sensorValue > 500)
  {
    State = 1;
  }
  else
  {
    State = 0;
  }
  return State;
}


void tactics(byte mode)
{
  switch (mode) {
    case 0:
    {
      long val = 100;
    }
      break;
      
    case 1:
      {
        long val = 100;
      }
      break;
      
    case 2:
      {
        long val = 100;
      }
      break;
      
      case 3:
      {
        long val = 100;
      }
        break;
        
      case 4:
      {
        long val = 100;
      }
        break;
        
      case 5:
      {
        long val = 100;
      }
        break;
    }
}

