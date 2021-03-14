/*
    DIY Arduino based RC Transmitter Project
              == Receiver Code ==
  by Dejan Nedelkovski, www.HowToMechatronics.com
  Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/
#include <SPI.h>
#include <RF24.h>
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
int motorSpeedLeft = 0;
int motorSpeedRight = 0;
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

//Set thrust
  motorSpeedLeft = data.Speed;
  motorSpeedRight = data.Speed;
  
  LeftMotor->setSpeed(motorSpeedLeft); 
  LeftMotor->run(FORWARD);

  RightMotor->setSpeed(motorSpeedRight); 
  RightMotor->run(FORWARD);

//Adjust thrust based on steer
  xAxis = data.Steer;
  if (xAxis < 117) {
    // Convert the declining X-axis readings from 0 to 110 into increasing 0 to 255 value
    int xMapped = map(xAxis, 110, 0, 0, 255);
    motorSpeedLeft = xMapped;
    motorSpeedRight = xMapped;
    LeftMotor->setSpeed(motorSpeedLeft); 
    LeftMotor->run(BACKWARD);
    RightMotor->setSpeed(motorSpeedRight); 
    RightMotor->run(FORWARD);
  }
    
  if (xAxis > 137) {
    // Convert the declining X-axis readings from 137 to 255 into increasing 0 to 255 value
    int xMapped = map(xAxis, 137, 255, 0, 255);
    motorSpeedLeft = xMapped;
    motorSpeedRight = xMapped;
    LeftMotor->setSpeed(motorSpeedLeft); 
    LeftMotor->run(FORWARD);
    RightMotor->setSpeed(motorSpeedRight); 
    RightMotor->run(BACKWARD);
  }

// drive the blade
  motorSpeedSpin = data.Spin;
   
  BladeMotor->setSpeed(motorSpeedSpin); 
  LeftMotor->run(FORWARD);
    
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
  data.Steer = 0;
  data.Speed = 0;
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
