/*    
  DIY Arduino based RC Transmitter Project
              == Receiver Code ==
  by Paul Choin 4/17/2021
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
const byte address[6] = "00001"; //The 5 digit address can be any combo of letters or numbers as long as they are the same on both xmitter and receiver. this is how you can keep your controller controlling your project!
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

// Inputs
int xAxis;
int yAxis;
// Initial Values
int motorSpeedSpin = 0;
int nMotMixL = 0;
int nMotMixR = 0;

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

// Temp variables
  float fPivYLimit = 32.0; // Threshold at which pivot action starts.  > value is increase range of pivot (0..127)
  float nMotPremixL; // Motor (left)  premixed output        (-128..+127)
  float nMotPremixR; // Motor (right) premixed output        (-128..+127)
  int nPivSpeed;     // Pivot Speed                          (-128..+127)
  float fPivScale;   // Balance scale b/w drive and pivot    (   0..1   )
  
// Read in Joystick values
  xAxis = data.Steer;
  yAxis = data.Speed;
  
 // Remap to -128 to 127
  xAxis = map(xAxis, 0, 255, -128, 127);
  yAxis = map(yAxis, 0, 255, -128, 127);
  
// Calculate Drive Turn output due to Joystick X input
  if (yAxis >= 0) {
     // Forward
    nMotPremixL = (xAxis>=0)? 127.0 : (127.0 + xAxis);
    nMotPremixR = (xAxis>=0)? (127.0 - xAxis) : 127.0;
  } else {
    // Reverse
    nMotPremixL = (xAxis>=0)? (127.0 - xAxis) : 127.0;
    nMotPremixR = (xAxis>=0)? 127.0 : (127.0 + xAxis);
  }

// Scale Drive output due to Joystick Y input (throttle)
  nMotPremixL = nMotPremixL * yAxis/128.0;
  nMotPremixR = nMotPremixR * yAxis/128.0;

// Now calculate pivot amount
// - Strength of pivot (nPivSpeed) based on Joystick X input
// - Blending of pivot vs drive (fPivScale) based on Joystick Y input
  nPivSpeed = xAxis;
  fPivScale = (abs(yAxis)>fPivYLimit)? 0.0 : (1.0 - abs(yAxis)/fPivYLimit);

// Calculate final mix of Drive and Pivot
  nMotMixL = (1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed);
  nMotMixR = (1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed);

// Convet to Motor PWM range and direction
  if (nMotMixL < 0) {
    nMotMixL = map(nMotMixL, -128, 0, 255, 0);
    LeftMotor->run(BACKWARD);
  } else {
    nMotMixL = map(nMotMixL, 0, 127, 0, 255);
    LeftMotor->run(FORWARD);
  }
  
  if (nMotMixR < 0) {
    nMotMixR = map(nMotMixR, -128, 0, 255, 0);
    RightMotor->run(BACKWARD);
  } else {
    nMotMixR = map(nMotMixR, 0, 127, 0, 255);
    RightMotor->run(FORWARD);
  }

  // Prevent buzzing at low speeds (Adjust according to your motors. My motors couldn't start moving if PWM value was below value of 20)
  if (nMotMixL < 20) {
    nMotMixL = 0;
  }
  if (nMotMixR < 20) {
    nMotMixR = 0;
  }
  
  LeftMotor->setSpeed(nMotMixL); // Send PWM signal to motor A
  RightMotor->setSpeed(nMotMixR); // Send PWM signal to motor B
     
  // Print the data in the Serial Monitor
  Serial.print("Speed: ");
  Serial.print(yAxis);
  Serial.print("; Steer: ");
  Serial.print(xAxis);
  Serial.print("; LeftSpeed: ");
  Serial.print(nMotPremixL);
  Serial.print("; RightSpeed: ");
  Serial.print(nMotPremixR);
  Serial.print("; LeftSpeed: ");
  Serial.print(nMotMixL);
  Serial.print("; RightSpeed: ");
  Serial.print(nMotMixR);
  Serial.print("; Blade: ");
  Serial.println(data.Spin); 


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
