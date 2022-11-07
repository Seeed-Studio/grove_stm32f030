/*
 * Multichannel_gas_sensor_V2.0_Firmware.ino
 * A Firmware for Grove- Multichannel_gas_sensor V2.0
 * Note: 
 *
 * Copyright (c) 2019 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Hontai Liu (lht856@foxmail.com)
 * Created Time: June 2019
 * Modified Time:
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <Wire.h>
#include <Flash.h>

#define DEBUG 0   // DEBUG enable:1 disable:0

#define FLASH_ADDRESS 0x10 //  I2C address storage

//host command
#define GM_102B 0x01
#define GM_302B 0x03
#define GM_402B 0x04
#define GM_502B 0x05
#define GM_702B 0x07
#define GM_802B 0x08
#define CHANGE_I2C_ADDR 0x55
#define WARMING_UP 0xFE
#define WARMING_DOWN  0xFF

// the real channel with Sensor in stm32F030F4
#define CHANNEL_GM_102B A0
#define CHANNEL_GM_302B A1
#define CHANNEL_GM_402B A4
#define CHANNEL_GM_502B A5
#define CHANNEL_GM_702B A6
#define CHANNEL_GM_802B A7

#define POWER_PIN PB1

uint8_t address = 0x08;
uint8_t channel;

void setup() {
  #if DEBUG
    Serial.begin(9600);
  #endif
  address = readAddress();
  if(address==00 || address > 127) // if undefine the I2C address, then define the I2C address 0x04
  {
    writeAddress(0x08);
    address = 0x08; // default i2c address;
    #if DEBUG
      Serial.print("addressFirst:");
      Serial.println(address);
    #endif
  }
  Wire.begin((uint8_t)(address<<1));   // join i2c bus with address 
  Wire.onReceive(receiveEvent);       // register receive event
  Wire.onRequest(requestEvent);       // register request event

  //pre heated
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  channel = 0;
}

void loop() {
  #if DEBUG
  address = readAddress();
  Serial.print("address:");
  Serial.println(address);
  if(Serial.available()){
    uint8_t temp = Serial.read();
    Serial.write(temp);
    writeAddress(temp);
    address = readAddress();
    Wire.begin((uint8_t)(address<<1));
   }
  #endif
  delay(100);
}

uint8_t readAddress()
{
  return Flash.read8(FLASH_ADDRESS); 
}

void writeAddress(uint8_t i2cAddr)
{
  Flash.write8(FLASH_ADDRESS, i2cAddr); 
  #if DEBUG
      Serial.print("address:");
      Serial.println(i2cAddr);
  #endif
}
void report(uint8_t channel)
{
  uint32_t value = 0;
  uint8_t *p;
  
  if(channel != 0)
    value = analogRead(channel); //get the value
    
  p = (uint8_t *)&value;

  //split uint32_t into uint8_t and write to I2C  
  Wire.write(*((uint8_t *)p+0));
  Wire.write(*((uint8_t *)p+1));
  Wire.write(*((uint8_t *)p+2));
  Wire.write(*((uint8_t *)p+3));

  #if DEBUG
     Serial.print("CHANNEL:"); 
     Serial.print(channel); 
     Serial.print("   VALUE:"); 
     Serial.println(value);   
  #endif
}

void changeI2CAddr()
{
    uint8_t temp = Wire.read();
 
    writeAddress(temp);
    address = readAddress();
    Wire.begin((uint8_t)(address<<1));
    
    #if DEBUG
      Serial.print("CHANGE:");
      Serial.println(address);
    #endif
}

void receiveEvent(int numBytes)
{
    uint8_t cmd = Wire.read(); // receive byte as a character    
    switch(cmd)
    {
      case GM_102B:
        channel = CHANNEL_GM_102B;
        break;
      case GM_302B:     
        channel = CHANNEL_GM_302B;
        break;
      case GM_402B:    
        channel = CHANNEL_GM_402B; 
        break;
      case GM_502B:
        channel = CHANNEL_GM_502B;  
        break;  
      case GM_702B:     
        channel = CHANNEL_GM_702B; 
        break;
      case GM_802B:    
        channel = CHANNEL_GM_802B; 
        break;   
      case CHANGE_I2C_ADDR:
        changeI2CAddr();
        break;
      case WARMING_UP:
        digitalWrite(POWER_PIN, HIGH);
        break;
      case WARMING_DOWN:
        digitalWrite(POWER_PIN, LOW);
        break;
      default:
        channel = 0;
        break;
    }
}

void requestEvent()
{
  report(channel);  //report the value of sensor
}
