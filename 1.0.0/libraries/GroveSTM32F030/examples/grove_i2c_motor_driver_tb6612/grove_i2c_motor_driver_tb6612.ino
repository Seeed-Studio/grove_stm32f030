/*
 * grove_motor_driver(TB6612FNG).ino
 * This is the firmware of Grove - Motor Driver(TB6612FNG), based on STM32F030F4P6 mcu and TB6612FNG motor driver.
 * 
 *
 * Copyright (c) 2018 Seeed Technology Co., Ltd
 * Author        :   JerryYip 
 * Create Time   :   05/30/2018
 * Version       :   0.3
 * Change Log    :   
 *                   v0.1 init
 *                   v0.2 修复使用步进电机时波形失真问题
 *                   v0.3 移到GroveSTM32F030项目中，修复树莓派下i2c read无响应问题；增加 KEEP_RUN 命令；
 *                        关闭 JUMP_TO_BOOT 命令；
 *                   
 * TODO          :   1. 直流电机缓启动，s型启动曲线
 *                   2. 电机状态查询
 *                   3. 连续接收i2c命令时，第二个命令响应不到
 *
 */


#include <Wire.h>
#include <Flash.h>
#include <WatchDog.h>
// #include <Timer14.h>


/***************************************************************
 Pin defines
 ***************************************************************/
#define PWMA 10             // PA4/PWM
#define AIN1 4              // PA0
#define AIN2 5              // PA1
#define PWMB 14             // PB1/PWM
#define BIN1 12             // PA6
#define BIN2 13             // PA7
#define STBY 11             // PA5

// #define I2C_SDA   3         // PA10
// #define I2C_SCL   2         // PA9
// #define USART_TX  1         // PA2
// #define USART_RX  0         // PA3


/***************************************************************
 Communication defines
 ***************************************************************/
#define I2C_DEF_ADDR_FLASH_LOC	0x00
#define I2C_CUR_ADDR_FLASH_LOC	0x01

#define GROVE_MOTOR_DRIVER_DEFAULT_I2C_ADDR         0x14
#define GROVE_MOTOR_DRIVER_I2C_CMD_BRAKE            0x00
#define GROVE_MOTOR_DRIVER_I2C_CMD_STOP             0x01
#define GROVE_MOTOR_DRIVER_I2C_CMD_CW               0x02
#define GROVE_MOTOR_DRIVER_I2C_CMD_CCW              0x03
#define GROVE_MOTOR_DRIVER_I2C_CMD_STANDBY          0x04
#define GROVE_MOTOR_DRIVER_I2C_CMD_NOT_STANDBY      0x05
#define GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_RUN      0x06
#define GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_STOP     0x07
#define GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_KEEP_RUN 0x08

// #define GROVE_MOTOR_DRIVER_I2C_CMD_GET_STATE        0x10
#define GROVE_MOTOR_DRIVER_I2C_CMD_SET_ADDR         0x11
#define GROVE_MOTOR_DRIVER_I2C_CMD_RST_ADDR         0x12

#define GROVE_MOTOR_DRIVER_I2C_CMD_TEST_GET_REV     0xc0
// #define GROVE_MOTOR_DRIVER_I2C_CMD_TEST_TX_RX_ON    0xc1
// #define GROVE_MOTOR_DRIVER_I2C_CMD_TEST_TX_RX_OFF   0xc2
#define GROVE_MOTOR_DRIVER_I2C_CMD_JUMP_TO_BOOT     0xf0
#define GROVE_MOTOR_DRIVER_I2C_CMD_NULL             0xff

#define GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH       8

#define ENABLE_JUMP_TO_BOOT 0

/***************************************************************
Basic defines
 ***************************************************************/

uint8_t software_version[2] = {0,3};
uint8_t command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
uint16_t device_i2c_address = GROVE_MOTOR_DRIVER_DEFAULT_I2C_ADDR;

enum motor_channel_type_t{
    MOTOR_CHA = 0,
    MOTOR_CHB = 1,
};

enum stepper_mode_type_t{
    FULL_STEP = 0,
    WAVE_DRIVE = 1,
    HALF_STEP = 2,
    MICRO_STEPPING = 3,
};


uint8_t pwm_sin[17] = {0, 25, 50, 74, 98, 120, 141, 162, 180, 197, 212, 225, 236, 244, 250, 253, 255};

// 0 means using dcmotor and 1 means using stepper
uint8_t _motor_mode = 0;
stepper_mode_type_t _stepper_mode = MICRO_STEPPING;
// state of stepper, 0=>ccw, 1=>cw, 2=>stop, 3=>idle, 4=>infinite ccw, 5=>infinite cw
uint8_t _stepper_cw = 1;
uint16_t _stepper_target_steps, _stepper_now_steps;

// _100us_per_step means how many 0.1millisseconds(100us) one step cost
// Don't want to use floating number, because it cost too much flash
// Dont' use 1millissecond as base unit, because 0.1millissecond has 10-times accuracy.
uint32_t _100us_per_step = 5;
uint32_t _us_per_step = 0;

// Record the micros() time stamp for stepper
// This number will overflow (go back to zero), after approximately 70 minutes, 
// which means a stepper will lost one step per 70min.
uint32_t last_time = 0;

void requestEvent();
void receiveEvent(int howmany);
/***************************************************************
 Device initialization
 ***************************************************************/
void setup()
{
    uint8_t i2cDefaultAddr = Flash.read8(I2C_DEF_ADDR_FLASH_LOC); 
	uint8_t i2cCurrentAddr = Flash.read8(I2C_CUR_ADDR_FLASH_LOC);

    if(i2cDefaultAddr == 0xff)Flash.write8(I2C_DEF_ADDR_FLASH_LOC, GROVE_MOTOR_DRIVER_DEFAULT_I2C_ADDR);
	if(i2cCurrentAddr == 0xff)Flash.write8(I2C_CUR_ADDR_FLASH_LOC, GROVE_MOTOR_DRIVER_DEFAULT_I2C_ADDR);
    else device_i2c_address = i2cCurrentAddr;

    initialize();
    standby(1);

    Wire.begin(device_i2c_address);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    wwdg.begin();

    // // 直流正反转功能正常
    // _motor_mode = 0;
    // cw(MOTOR_CHA,128);
    // ccw(MOTOR_CHB,128);

    // // 步进电机 HALF_STEP ok
    // _motor_mode = 1;
    // _stepper_mode = HALF_STEP;
    // _stepper_cw = 1; // cw
    // _stepper_target_steps = 2000*2;
    // _stepper_now_steps = 0;
    // _100us_per_step = 100;
    // _us_per_step = ((uint32_t)_100us_per_step * 100 / 16);
    // analogWrite(PWMA, 255);
    // analogWrite(PWMB, 255);

    // // 步进电机 MICRO_STEPPING ok
    // _motor_mode = 1;
    // _stepper_mode = MICRO_STEPPING;
    // _stepper_cw = 1; // cw
    // _stepper_target_steps = 2000*2;
    // _stepper_now_steps = 0;
    // _100us_per_step = 100;
    // _us_per_step = ((uint32_t)_100us_per_step * 100 / 16);
    // analogWrite(PWMA, 255);
    // analogWrite(PWMB, 255);
}

void runStepper()
{
    if (_motor_mode == 1)
    {
        switch (_stepper_cw)
        {
            // ccw
            case 0:
                // _stepper_now_steps start at _stepper_target_steps, end at 0
                if (_stepper_now_steps > 0)
                {
                    stepper(_stepper_mode, _stepper_now_steps);
                    _stepper_now_steps --;
                }
                else 
                {
                    _stepper_cw = 2;
                }
            break;

            // cw
            case 1:
                // _stepper_now_steps start at 0, end at _stepper_target_steps
                if (_stepper_now_steps < _stepper_target_steps)
                {
                    stepper(_stepper_mode, _stepper_now_steps);
                    _stepper_now_steps ++;
                }
                else 
                {
                    _stepper_cw = 2;
                }
            break;

            // stop
            case 2:
                stop(MOTOR_CHA);
                stop(MOTOR_CHB);
                _stepper_cw = 3;
            break;

            // infinite ccw
            case 4:
                stepper(_stepper_mode, _stepper_now_steps);
                _stepper_now_steps --;
            break;

            // infinite cw
            case 5:
                stepper(_stepper_mode, _stepper_now_steps);
                _stepper_now_steps ++;
            break;

            default:
            break;
        }
    }
}

void loop()
{
    uint32_t now_time = micros();
    if (now_time - last_time >= (uint32_t)_100us_per_step*100)
    {
        last_time = now_time;
        runStepper();
    }
    
    if (command_received == GROVE_MOTOR_DRIVER_I2C_CMD_SET_ADDR)
    {
        command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        Flash.write8(I2C_CUR_ADDR_FLASH_LOC, device_i2c_address);
        Wire.begin(device_i2c_address);
    }
    else if (command_received == GROVE_MOTOR_DRIVER_I2C_CMD_RST_ADDR)
    {
        command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        device_i2c_address = GROVE_MOTOR_DRIVER_DEFAULT_I2C_ADDR;
        Flash.write8(I2C_CUR_ADDR_FLASH_LOC, device_i2c_address);
        Wire.begin(device_i2c_address);
    }

    wwdg.reset();
}

void receiveEvent(int howmany)
{
    uint8_t count = 0, receive_buffer[GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH] = {0,};

    while (Wire.available())
    {
        receive_buffer[count ++] = Wire.read();
        if (count == GROVE_MOTOR_DRIVER_I2C_CMD_MAX_LENGTH) count = 0;
    }

    command_received = receive_buffer[0];
    switch(command_received)
    {
        // It's terrible functions for cmd i2cdump
#ifdef ENABLE_JUMP_TO_BOOT
        case GROVE_MOTOR_DRIVER_I2C_CMD_JUMP_TO_BOOT:
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
            jumpToBootloader();
        break;
#endif

        case GROVE_MOTOR_DRIVER_I2C_CMD_STANDBY:
            standby(1);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_NOT_STANDBY:
            standby(0);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_BRAKE:
            _motor_mode = 0;
            if (receive_buffer[1] == 0) 
            {
                // short barke
                cw(MOTOR_CHA, 0);
            }
            else if (receive_buffer[1] == 1)
            {
                // short barke
                cw(MOTOR_CHB, 0);
            }
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_CW:
            _motor_mode = 0;
            if (receive_buffer[1] == 0) cw(MOTOR_CHA, receive_buffer[2]);
            else if (receive_buffer[1] == 1) cw(MOTOR_CHB, receive_buffer[2]);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_CCW:
            _motor_mode = 0;
            if (receive_buffer[1] == 0) ccw(MOTOR_CHA, receive_buffer[2]);
            else if (receive_buffer[1] == 1) ccw(MOTOR_CHB, receive_buffer[2]);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_STOP:
            _motor_mode = 0;
            if (receive_buffer[1] == 0) stop(MOTOR_CHA);
            else if (receive_buffer[1] == 1) stop(MOTOR_CHB);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_RUN:
            _motor_mode = 1;
            // set the stepper working mode
            _stepper_mode = (stepper_mode_type_t)receive_buffer[1];
            // set the direction, receive_buffer[2]: 0=>ccw  1=>cw
            if (receive_buffer[2] == 0) _stepper_cw = 0;
            else _stepper_cw = 1;
            // set target steps
            _stepper_target_steps = receive_buffer[3] + receive_buffer[4]*256;
            if (_stepper_cw > 0) _stepper_now_steps = 0;
            else _stepper_now_steps = _stepper_target_steps;

            // set the speed of stepper
            _100us_per_step = receive_buffer[5] + receive_buffer[6]*256;
            // set _us_per_step for different stepper working mode
            if (_stepper_mode == HALF_STEP) _100us_per_step = _100us_per_step / 2;
            _us_per_step = _100us_per_step * 100 / 16;

            analogWrite(PWMA, 255);
            analogWrite(PWMB, 255);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_STOP:
            _motor_mode = 1;
            // stop
            _stepper_cw = 2;
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_STEPPER_KEEP_RUN:
            _motor_mode = 1;
            // set the stepper working mode
            _stepper_mode = (stepper_mode_type_t)receive_buffer[1];
            // set the direction, receive_buffer[2]: 4=>infinite ccw  5=>infinite cw
            if (receive_buffer[2] == 4) _stepper_cw = 4;
            else _stepper_cw = 5;
            // dont need to set target steps
            // set the speed of stepper
            _100us_per_step = receive_buffer[3] + receive_buffer[4]*256;
            // set _us_per_step for different stepper working mode
            if (_stepper_mode == HALF_STEP) _100us_per_step = _100us_per_step / 2;
            _us_per_step = _100us_per_step * 100 / 16;

            analogWrite(PWMA, 255);
            analogWrite(PWMB, 255);
            command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;

        case GROVE_MOTOR_DRIVER_I2C_CMD_SET_ADDR:
            // in case of i2cdump
            if ((receive_buffer[1] != device_i2c_address) && (receive_buffer[1] > 0x02)) {
                device_i2c_address = receive_buffer[1];
            }
            else {
                command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
            }
        break;

        default:
            // do nothing
            // command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        break;
    }
}

void requestEvent(void)
{		
    if (command_received == GROVE_MOTOR_DRIVER_I2C_CMD_NULL) {
        uint8_t p = 0;
        Wire.write((&p),sizeof(p));
        return ;
    }

	switch(command_received)
	{	
        // case GROVE_MOTOR_DRIVER_I2C_CMD_TEST_GET_REV:
        //     Wire.write(software_version, 2);
        //     command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        // break;

        // case GROVE_MOTOR_DRIVER_I2C_CMD_GET_STATE:
        //     Wire.write(motor_state);
        //     command_received = GROVE_MOTOR_DRIVER_I2C_CMD_NULL;
        // break;

		default:
		break;
	}
}


/***************************************************************
 Motor Driver
 ***************************************************************/
void initialize()
{
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(STBY, OUTPUT);
}

void stop(motor_channel_type_t chl)
{
    switch(chl)
    {
        case MOTOR_CHA:
            digitalWrite(AIN1, LOW);
            digitalWrite(AIN2, LOW);
            analogWrite(PWMA, 0);
        break;

        case MOTOR_CHB:
            digitalWrite(BIN1, LOW);
            digitalWrite(BIN2, LOW);
            analogWrite(PWMB, 0);
        break;

        default:
        break;
    }
}

// pwm = 0~255
void cw(motor_channel_type_t chl, uint8_t pwm)
{
    switch(chl)
    {
        case MOTOR_CHA:
            digitalWrite(AIN1, HIGH);
            digitalWrite(AIN2, LOW);
            analogWrite(PWMA, pwm);
        break;

        case MOTOR_CHB:
            digitalWrite(BIN1, HIGH);
            digitalWrite(BIN2, LOW);
            analogWrite(PWMB, pwm);
        break;

        default:
        break;
    }
}

// pwm = 0~255
void ccw(motor_channel_type_t chl, uint8_t pwm)
{
    switch(chl)
    {
        case MOTOR_CHA:
            digitalWrite(AIN1, LOW);
            digitalWrite(AIN2, HIGH);
            analogWrite(PWMA, pwm);
        break;

        case MOTOR_CHB:
            digitalWrite(BIN1, LOW);
            digitalWrite(BIN2, HIGH);
            analogWrite(PWMB, pwm);
        break;

        default:
        break;
    }
}

// high=1; low=0;
void standby(uint8_t high_low)
{
    digitalWrite(STBY, high_low);
}

// Just run one step for any case, don't need to care about time
void stepper(stepper_mode_type_t mode, uint16_t step_num)
{
    uint8_t which_step;
    switch(mode)
    {
        // 0 2 4 6
        // case FULL_STEP:
        case WAVE_DRIVE:
            which_step = (step_num % 4) * 2;
        break;

        // 1 3 5 7
        // case WAVE_DRIVE:
        case FULL_STEP:
            which_step = (step_num % 4) * 2 + 1;
        break;

        // 0-7
        case HALF_STEP:
            which_step = step_num % 8;
        break;

        // 8-11
        case MICRO_STEPPING:
            which_step = (step_num % 4) + 8;
        break;

        default:
        break;
    }
        
    switch(which_step)
    {
        case 0:
            // oneStep(1,0,0,1);
            oneStep(1,1,0,1);
        break;

        case 1:
            // oneStep(1,0,0,0); 
            oneStep(1,0,0,1); 
        break;

        case 2:
            // oneStep(1,0,1,0); 
            oneStep(1,0,1,1); 
        break;

        case 3:
            // oneStep(0,0,1,0); 
            oneStep(1,0,1,0); 
        break;

        case 4:
            // oneStep(0,1,1,0);
            oneStep(1,1,1,0);
        break;

        case 5:
            // oneStep(0,1,0,0);
            oneStep(0,1,1,0);
        break;

        case 6:
            // oneStep(0,1,0,1);
            oneStep(0,1,1,1);
        break;

        case 7:
            // oneStep(0,0,0,1);
            oneStep(0,1,0,1);
        break;

        case 8:
            if (_stepper_cw == 1 || _stepper_cw == 5) oneMicroStep(1,0,0,-1,_us_per_step); // 0
            else oneMicroStep(0,1,0,-1,_us_per_step); // ccw
        break;

        case 9:
            if (_stepper_cw == 1 || _stepper_cw == 5) oneMicroStep(-1,0,1,0,_us_per_step); // 1
            else oneMicroStep(-1,0,0,1,_us_per_step); // ccw
        break;

        case 10:
            if (_stepper_cw == 1 || _stepper_cw == 5) oneMicroStep(0,1,-1,0,_us_per_step); // 2
            else oneMicroStep(1,0,-1,0,_us_per_step); // ccw
        break;

        case 11:
            if (_stepper_cw == 1 || _stepper_cw == 5) oneMicroStep(0,-1,0,1,_us_per_step); // 3
            else oneMicroStep(0,-1,1,0,_us_per_step); // ccw
        break;  

        default:
        break;
    }
}


void oneStep(bool a1, bool a2, bool b1, bool b2)
{
   // TODO: check this
   // make sure us_per_step never too large
    uint32_t us_per_step = _us_per_step;
    if (_us_per_step > 25)
    {
        // _us_per_step = (uint32_t)(_100us_per_step * 6.25);
        // _us_per_step = _100us_per_step * 100 / 16;
        us_per_step = _us_per_step << 4 - 100; 
    }

    digitalWrite(AIN1, a1);
    digitalWrite(AIN2, a2);
    digitalWrite(BIN1, b1);
    digitalWrite(BIN2, b2);

    // Note: analogWrite works at next period after set, so we can't do this here.
    // analogWrite(PWMA, 255);
    // analogWrite(PWMB, 255);

    // TODO: check this
    delayMicroseconds(us_per_step);
}

// delay = _100us_per_step / 16;
// 0 low 1 rise -1 drop
// _100us_per_step > 2.5?
// TODO: check this
void oneMicroStep(int8_t a1, int8_t a2, int8_t b1, int8_t b2, uint32_t us_per_step)
{    
    // float us_per_step = _100us_per_step * 62.5; // _100us_per_step * 1000 / 16

    digitalWrite(AIN1, abs(a1));
    digitalWrite(AIN2, abs(a2));
    digitalWrite(BIN1, abs(b1));
    digitalWrite(BIN2, abs(b2));

    // 46 is an offset
    if (us_per_step > 46) us_per_step = us_per_step - 46;

    for (uint8_t i=0;i<16;i++)
    {
        if((a1+a2)>0)  analogWrite(PWMA, pwm_sin[i]);
        else analogWrite(PWMA, pwm_sin[15-i]);
        if((b1+b2)>0)  analogWrite(PWMB, pwm_sin[i]);
        else analogWrite(PWMB, pwm_sin[15-i]);
        delayMicroseconds((uint32_t)us_per_step);
    }
}


