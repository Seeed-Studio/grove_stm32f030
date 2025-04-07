# README

Here are some Arduino IDE-based firmware for Grove STM32F030.

No need to build unless you want to modify the firmware.

grove_i2c_motor_driver_stm32f030V105.bin is ready to burn. It must be used with [Grove_I2C_Motor_Driver_v1_3](../../../../Grove_I2C_Motor_Driver_v1_3) V1.05 or higher.

## Build

1. Open a `.ino` firmware in `Arduino`, select `Tool` -> `Board:` -> `Grove STM32F030`.
2. Click `Verify` button to start compiling, the path of `.bin` file can be found in log(when verbose output is enable).

## Burn SEGGER JLink/J-Flash

Connect SEGGER JLink/J-Flash to STM32F030, select device `STM32F030F4`.

## Burn ST-Link

### Hardware:
- SWC - SWCLK，
- SWD - SWDIO
- VCC - 3V3
- GND - GND

Vin: 12V down to 5V can be used

![stlink](https://github.com/benppppp/grove_stm32f030/assets/170195651/bf734e71-2abc-4b47-9015-0d6314b1c0ff)

### Software:

Download and execute STM32CubeProgrammer
1. If your wiring is correct, clicking on Connect will directly recognize your current chip without additional checking.
2. In Erasing and programming, Browse your bin file.
3. Click on Start programming

![STM32cube2](https://github.com/benppppp/grove_stm32f030/assets/170195651/9dd4fc31-21ad-4268-b46d-845b3903b7b0)




