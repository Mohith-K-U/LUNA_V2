
/* 
This is the code for LUNA_V2, a semi-humanoid robot
Author name: Mohith K U
Author email: mohithmandanna29@gmail.com
*/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// RIGHT arm channels
const int RIGHT_SHOULDER = 15;
const int RIGHT_ELBOW    = 14;

// LEFT arm channels
const int LEFT_SHOULDER = 0;
const int LEFT_ELBOW    = 1;

// Store current angles for all servos
int currentAngle[16];  

// Convert angle to PWM pulse
int angleToPulse(int angle) {
  return map(angle, 0, 180, 150, 600);
}

// Move servo slowly to target
void moveServoSlow(int channel, int targetAngle, int stepDelay) {
  if (targetAngle > currentAngle[channel]) {
    for (int a = currentAngle[channel]; a <= targetAngle; a++) {
      pwm.setPWM(channel, 0, angleToPulse(a));
      delay(stepDelay);
    }
  } else {
    for (int a = currentAngle[channel]; a >= targetAngle; a--) {
      pwm.setPWM(channel, 0, angleToPulse(a));
      delay(stepDelay);
    }
  }
  currentAngle[channel] = targetAngle;
}

// RIGHT handshake motion
void rHandshake() {
  moveServoSlow(RIGHT_ELBOW, 75, 35);
  delay(500);
  moveServoSlow(RIGHT_SHOULDER, 45, 35);
  delay(700);

  for (int i = 0; i < 3; i++) {
    moveServoSlow(RIGHT_ELBOW, 90, 30);
    delay(300);
    moveServoSlow(RIGHT_ELBOW, 75, 30);
    delay(300);
  }

  delay(700);
  moveServoSlow(RIGHT_ELBOW, 90, 35);
  delay(500);
  moveServoSlow(RIGHT_SHOULDER, 0, 35);
}

// Hands oscillation (called only while moving)
void handsFront() {
  moveServoSlow(RIGHT_ELBOW, 75, 30);
  moveServoSlow(LEFT_ELBOW, 75, 30);
  delay(100);
  moveServoSlow(RIGHT_ELBOW, 90, 30);
  moveServoSlow(LEFT_ELBOW, 90, 30);
  delay(100);
}


// Motor driver (BTS7960) setup

const int RPWM_L = 8;  
const int LPWM_L = 10; 
const int REN_L  = 7;  
const int LEN_L  = 6;  

const int RPWM_R = 9;  
const int LPWM_R = 11; 
const int REN_R  = 12; 
const int LEN_R  = 13; 

int speedValue = 160;   // Motor speed
unsigned long motorStartTime = 0;
bool moving = false;
enum MotorAction {NONE, FORWARD, BACKWARD, LEFT, RIGHT};
MotorAction currentAction = NONE;


// Motor functions

void moveForward() { 
  analogWrite(RPWM_L, speedValue); analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, speedValue);
  moving = true; currentAction = FORWARD;
  Serial.println("Moving FORWARD (continuous)");
}

void moveBackward() { 
  analogWrite(RPWM_L, 0); analogWrite(LPWM_L, speedValue);
  analogWrite(RPWM_R, speedValue); analogWrite(LPWM_R, 0);
  moving = true; currentAction = BACKWARD;
  Serial.println("Moving BACKWARD (continuous)");
}

void turnRight() {
  // Left tyres stop
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 0);

  // Right tyres move forward
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, speedValue);

  motorStartTime = millis();
  moving = true;
  currentAction = RIGHT;
  Serial.println("Turning RIGHT (for 3 seconds)");
}

void turnLeft() {
  // Right tyres stop
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 0);

  // Left tyres move forward
  analogWrite(RPWM_L, speedValue);
  analogWrite(LPWM_L, 0);

  motorStartTime = millis();
  moving = true;
  currentAction = LEFT;
  Serial.println("Turning LEFT (for 3 seconds)");
}

void stopMotors() { 
  analogWrite(RPWM_L, 0); analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, 0);
  moving = false; currentAction = NONE;
  Serial.println("STOP");
}




void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);

  for (int i = 0; i < 16; i++) {
    currentAngle[i] = 90;
    pwm.setPWM(i, 0, angleToPulse(90));
  }

  currentAngle[RIGHT_SHOULDER] = 0;
  pwm.setPWM(RIGHT_SHOULDER, 0, angleToPulse(0));
  currentAngle[RIGHT_ELBOW] = 90;
  pwm.setPWM(RIGHT_ELBOW, 0, angleToPulse(90));
  currentAngle[LEFT_SHOULDER] = 90;
  pwm.setPWM(LEFT_SHOULDER, 0, angleToPulse(90));
  currentAngle[LEFT_ELBOW] = 90;
  pwm.setPWM(LEFT_ELBOW, 0, angleToPulse(90));

  pinMode(RPWM_L, OUTPUT); pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_R, OUTPUT); pinMode(LPWM_R, OUTPUT);
  pinMode(REN_L, OUTPUT); pinMode(LEN_L, OUTPUT);
  pinMode(REN_R, OUTPUT); pinMode(LEN_R, OUTPUT);
  digitalWrite(REN_L, HIGH); digitalWrite(LEN_L, HIGH);
  digitalWrite(REN_R, HIGH); digitalWrite(LEN_R, HIGH);
  stopMotors();

  Serial.println("Robot Ready!");
  Serial.println("Motor commands: f, b, b, r, s");
  Serial.println("Hand commands: rshake");
}


void loop() {
  // Check serial commands
if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "f") moveForward();
    else if (cmd == "b") moveBackward();
    else if (cmd == "l") turnLeft();
    else if (cmd == "r") turnRight();
    else if (cmd == "s") stopMotors();
    else if (cmd == "rshake") rHandshake();
  }

  // If moving, animate hands
  if (moving) handsFront();

  // Automatically stop LEFT and RIGHT after 3 sec only
  if ((currentAction == LEFT || currentAction == RIGHT) &&
      (millis() - motorStartTime >= 3000)) {
    stopMotors();
    Serial.println("Motors stopped automatically after 3 sec (turn complete)");
  }
}
