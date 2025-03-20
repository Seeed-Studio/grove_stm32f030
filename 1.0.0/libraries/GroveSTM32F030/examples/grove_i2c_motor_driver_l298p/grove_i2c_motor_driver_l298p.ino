#include <Wire.h>
#include <Flash.h>
#include <WatchDog.h>

//Motor 2 Input
#define IN4 1  //PA2
#define IN3 0  //PA3
#define EB 10  //PA4

//Motor 1 Input
#define IN2 11  //PA5
#define IN1 13  //PA7
#define EA 12   //PA6

#define SCL 2  //PA9
#define SDA 3  //PA10

#define MotorSpeedSet 0x82
#define PWMFrequenceSet 0x84
#define DirectionSet 0xaa
#define MotorSetA 0xa1
#define MotorSetB 0xa5
#define GetSpeed 0x90      //new v105
#define GetVersion 0x91    //new V105
#define GetTimedout 0x92   //new V105
#define GetTimeout 0x93    //new V105
#define SetTimeout 0x86    //new V1055:motor stop after timout
#define Timeoutreset 0x88  //new V1055:motor stop after timout

#define Version 105  //new V105 - firmware version number

#define MOTOR1 1
#define MOTOR2 2

#define GROVE_MOTOR_DRIVER_I2C_CMD_NULL 0xff

#define GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH 4

#define ADDR4 8  //PF0
#define ADDR3 9  //PF1
#define ADDR2 4  //PA0
#define ADDR1 5  //PA1

int address = 0;
int speed1 = 0;
int speed2 = 0;
byte Status = GetSpeed;  //default=getSpeed=V103

unsigned long timeout = 0;      //timeout in ms - 0 = no timeout
unsigned long tTimeout = 0;     //timeout timer
unsigned long trefTimeout = 0;  //timeout reference time
bool timedout = false;          //true if stopped by timeout

void setup() {
  //setPWMfrequence(490);
  pinMode(ADDR4, INPUT);
  pinMode(ADDR3, INPUT);
  pinMode(ADDR2, INPUT);
  pinMode(ADDR1, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(EA, OUTPUT);
  pinMode(EB, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  digitalWrite(EB, HIGH);
  digitalWrite(EA, HIGH);

  address = digitalRead(ADDR4) * 8 + digitalRead(ADDR3) * 4 + digitalRead(ADDR2) * 2 + digitalRead(ADDR1);
  Wire.begin(address);
  Wire.onReceive(receiveEvent);  //Triggers an event that receives a host character
  Wire.onRequest(requestEvent);  //Returns the data from the slave
}

void loop() {
  //watchdog
  tTimeout = millis() - trefTimeout;
  if (timeout > 0) {             //timeout active
    if (!timedout) {             //timeout did not occur
      if (tTimeout > timeout) {  //Time over timeout
        directionSet(0);         //stop motor
        timedout = true;         //boolean true
      }
    }
  }
}

void receiveEvent(int a) {
  uint8_t count = 0, receive_buffer[GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH];
  while ((Wire.available() > 0) && (count < 4)) {
    receive_buffer[count++] = Wire.read();
    if (count == GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH)
      count = 0;
  }

  if (MotorSpeedSet == receive_buffer[0]) {
    MotorspeedSet(receive_buffer[1], receive_buffer[2]);
  } else if (PWMFrequenceSet == receive_buffer[0]) {
    PWMfrequenceSet(receive_buffer[1] + (receive_buffer[2] << 8));
  } else if (DirectionSet == receive_buffer[0]) {
    directionSet(receive_buffer[1]);
  } else if (0x90 == (receive_buffer[0] & 0xf0)) {  //if start with 0x9. -> get....
    Status = receive_buffer[0];
  } else if (SetTimeout == receive_buffer[0]) {
    TimeoutSet(receive_buffer[1] + (receive_buffer[2] << 8));
  } else if (Timeoutreset == receive_buffer[0]) {
    TimeoutReset();
  }
}

//  BothClockWise             0x0a=0b1010  两个顺时针
//  BothAntiClockWise         0x05=0b0101  两个逆时针
//  M1CWM2ACW                 0x06=0b0110  1顺时针2逆时针
//  M1ACWM2CW                 0x09=0b1001  1逆时针2顺时针

void directionSet(uint8_t _direction) {
  //Define direction
  if ((_direction & 0b11) == 0b11 or (_direction & 0b1100) == 0b1100) {
    _direction = 0;  //both stop if not valid direction (both HIGH)
  }
  digitalWrite(IN1, bitRead(_direction, 0));
  digitalWrite(IN2, bitRead(_direction, 1));
  digitalWrite(IN3, bitRead(_direction, 2));
  digitalWrite(IN4, bitRead(_direction, 3));
  TimeoutReset();
}

void PWMfrequenceSet(uint32_t _frequence) {
  setPWMfrequence(_frequence);
  TimeoutReset();
}

void MotorspeedSet(uint8_t _speed1, uint8_t _speed2) {
  speed1 = _speed1;
  speed2 = _speed2;
  analogWrite(EA, _speed1);
  analogWrite(EB, _speed2);
  TimeoutReset();
}

void TimeoutSet(uint32_t _timeout) {
  timeout = _timeout;
  TimeoutReset();
}

void TimeoutReset() {
  if (timeout > 0) {
    trefTimeout = millis();
    timedout = false;
  }
}

void requestEvent() {
  switch (Status) {
    case GetSpeed:
      Wire.write(speed1);
      Wire.write(speed2);
      break;
    case GetVersion:
      Wire.write(Version);
      Wire.write(0);
    case GetTimedout:
      Wire.write(timedout);
      Wire.write(0);
    case GetTimeout:
      Wire.write(timeout & 0x000000ff);
      Wire.write((timeout & 0x0000ff00) >> 8);
    default:  //should never occur
      Wire.write(254);
      Wire.write(255);
      break;
  }
}