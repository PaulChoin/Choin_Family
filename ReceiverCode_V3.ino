/*
    DIY Arduino based RC Transmitter Project
              == Receiver Code ==
  by Paul Choin 3/19/2021
  Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/
#include <SPI.h>
#include <RF24.h>Dejan Nedelkovski, www.HowToMe
#include <nRF24L01.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

//call the Adafruit motor shield and define the motor ports
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *LeftMotor = AFMS.getMotor(3);
Adafruit_DCMotor *RightMotor = AFMS.getMotor(4);
Adafruit_DCMotor *BladeMotor = AFMS.getMotor(2);

bool radioNumber = 0; // set as receiver

RF24 radio(18, 19);   // nRF24L01 (CE, CSN)
const byte address[6] = "00001";
unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;
// Max size of this struct is 32 bytes - NRF24L01 buffer limit
struct Data_Package {
  byte Spin;
  byte Steer;
  byte Speed;
  //byte j2PotX;
  //byte j2PotY;
  //byte j2Button;
  //byte pot1;
  //byte pot2;
  //byte tSwitch1;
  //byte tSwitch2;
  //byte button1;
  //byte button2;
  //byte button3;
  //byte button4;
};
Data_Package data; //Create a variable with the above structure

int xAxis;
int yAxis;
int motorSpeedLeft = 127;
int motorSpeedRight = 127;
int motorSpeedSpin = 0;

void setup() {
  Serial.begin(9600);
  AFMS.begin();
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW); //power mode LOW for debug, HIGH for application
  radio.startListening(); //  Set the module as receiver
  resetData(); // clear the values at every start
}
void loop() {
  // Check whether there is data to be received
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package)); // Read the whole data and store it into the 'data' structure
    lastReceiveTime = millis(); // At this moment we have received the data
  }
  // Check whether we keep receving data, or we have a connection between the two modules
  currentTime = millis();
  if ( currentTime - lastReceiveTime > 1000 ) { // If current time is more then 1 second since we have recived the last data, that means we have lost connection
    resetData(); // If connection is lost, reset the data. It prevents unwanted behavior, for example if a drone has a throttle up and we lose connection, it can keep flying unless we reset the values
  }
  
// drive the blade
  motorSpeedSpin = data.Spin;
   
  BladeMotor->setSpeed(motorSpeedSpin); 
  BladeMotor->run(FORWARD);

// Steer and Speed

  xAxis = data.Steer;
  yAxis = data.Speed;

  // Y-axis used for forward and backward control
  if (yAxis < 110) {
    // Set Motor Left Motor backward
    LeftMotor->setSpeed(motorSpeedLeft); 
    LeftMotor->run(BACKWARD);
 
    // Set Motor Right backward
    RightMotor->setSpeed(motorSpeedRight); 
    RightMotor->run(BACKWARD);
    
    // Convert the declining Y-axis readings for going backward from 110 to 0 into 0 to 255 value for the PWM signal for increasing the motor speed
    motorSpeedLeft = map(yAxis, 110, 0, 0, 255);
    motorSpeedRight = map(yAxis, 110, 0, 0, 255);

  }
  else if (yAxis > 140) {
    // Set Motor Left forward
    LeftMotor->run(FORWARD);
    // Set Motor B forward
    RightMotor->run(FORWARD);
    // Convert the increasing Y-axis readings for going forward from 140 to 255 into 0 to 255 value for the PWM signal for increasing the motor speed
    motorSpeedLeft = map(yAxis, 140, 255, 0, 255);
    motorSpeedRight = map(yAxis, 140, 255, 0, 255);
  }
  // If joystick stays in middle the motors are not moving
  else {
    motorSpeedLeft = 0;
    motorSpeedRight = 0;
  }
  // X-axis used for left and right control
  if (xAxis < 110) {
    // Convert the declining X-axis readings from 140 to 255 into increasing 0 to 255 value
    int xMapped = map(xAxis, 110, 0, 0, 255);
    // Move to left - decrease left motor speed, increase right motor speed
    motorSpeedLeft = motorSpeedLeft - xMapped;
    motorSpeedRight = motorSpeedRight + xMapped;
    // Confine the range from 0 to 255
    if (motorSpeedLeft < 0) {
      motorSpeedLeft = 0;
    }
    if (motorSpeedRight > 255) {
      motorSpeedRight = 255;
    }
  }
  if (xAxis > 140) {
    // Convert the increasing X-axis readings from 110 to 0 into 0 to 255 value
    int xMapped = map(xAxis, 140, 255, 0, 255);
    // Move right - decrease right motor speed, increase left motor speed
    motorSpeedLeft = motorSpeedLeft + xMapped;
    motorSpeedRight = motorSpeedRight - xMapped;
    // Confine the range from 0 to 255
    if (motorSpeedLeft > 255) {
      motorSpeedLeft = 255;
    }
    if (motorSpeedRight < 0) {
      motorSpeedRight = 0;
    }
  }
  // Prevent buzzing at low speeds (Adjust according to your motors. My motors couldn't start moving if PWM value was below value of 70)
  if (motorSpeedLeft < 70) {
    motorSpeedLeft = 0;
  }
  if (motorSpeedRight < 70) {
    motorSpeedRight = 0;
  }

  LeftMotor->setSpeed(motorSpeedLeft); // Send PWM signal to motor A
  RightMotor->setSpeed(motorSpeedRight); // Send PWM signal to motor B
     
  // Print the data in the Serial Monitor
  //Serial.print("pot1: ");
  //Serial.print(data.pot1);
  Serial.print("Speed: ");
  Serial.print(data.Speed);
  Serial.print("; Steer: ");
  Serial.print(data.Steer);
  Serial.print("; Lift: ");
  Serial.println(data.Spin); 
  Serial.print("; LeftMotorSpeed: ");
  Serial.println(motorSpeedLeft); 

}
void resetData() {
  // Reset the values when there is no radio connection - Set initial default values
  data.Spin = 0;
  data.Steer = 127;
  data.Speed = 127;
  //data.j2PotY = 0;
  //data.j1Button = 1;
  //data.j2Button = 1;
  //data.pot1 = 1;
  //data.pot2 = 1;
  //data.tSwitch1 = 1;
  //data.tSwitch2 = 1;
  //data.button1 = 1;
  //data.button2 = 1;
  //data.button3 = 1;
  //data.button4 = 1;
}
