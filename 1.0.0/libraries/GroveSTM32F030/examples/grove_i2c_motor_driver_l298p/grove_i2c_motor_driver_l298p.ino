#include <Wire.h>
#include <Flash.h>
#include <WatchDog.h>

/**************Motor Direction***************/
#define BothClockWise 0x0a      //1010
#define BothAntiClockWise 0x05  //0101
#define M1CWM2ACW 0x06          //0110
#define M1ACWM2CW 0x09          //1001

#define M1CWM2ST 0x02   //0010
#define M1ACWM2ST 0x01  //0001
#define M1STM2CW 0x08   //1000
#define M1STM2ACW 0x04  //0100
#define M1STM2ST 0x00   //0000

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
void setup() {
  setPWMfrequence(490);
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
  delay(1000);
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
  }
}

//  BothClockWise             0x0a  两个顺时针
//  BothAntiClockWise         0x05  两个逆时针
//  M1CWM2ACW                 0x06  1顺时针2逆时针
//  M1ACWM2CW                 0x09  1逆时针2顺时针

void directionSet(uint8_t _direction) {
  //Define direction
  if ((_direction & 0b11) == 0b11 or (_direction & 0b1100) == 0b1100) {
    _direction = 0;  //both stop if not valid direction (both HIGH)
  }
  digitalWrite(IN1, bitRead(_direction, 0));
  digitalWrite(IN2, bitRead(_direction, 1));
  digitalWrite(IN3, bitRead(_direction, 2));
  digitalWrite(IN4, bitRead(_direction, 3));
}

void PWMfrequenceSet(uint32_t _frequence) {
  setPWMfrequence(_frequence);
}

void MotorspeedSet(uint8_t _speed1, uint8_t _speed2) {
  speed1 = _speed1;
  speed2 = _speed2;
  analogWrite(EA, _speed1);
  analogWrite(EB, _speed2);
}

void requestEvent() {
  Wire.write(speed1);
  Wire.write(speed2);
}
