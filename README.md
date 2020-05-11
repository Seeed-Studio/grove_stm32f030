# Usage for Windows:

1. Install `Arduino` version >= 1.6, `Git for Windows` with MSYS

2. Open `Git for Windows`, then run below commands
   ```
   cd ~/AppData/Local/Arduino15/
   mkdir -p packages/Seeeduino/hardware; mkdir -p packages/Seeeduino/tools
   export GIT_DIR=`pwd`/packages/.git; cd packages/Seeeduino/hardware
   git clone --separate-git-dir $GIT_DIR https://github.com/Seeed-Studio/grove_stm32f030.git Grove_STM32F030
   cp Grove_STM32F030/package_grove_stm32f030_index.json ../../../
   ```

4. Open below .json file by text editor
   ```
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_stm32f030_index.json
   ```
   Find the downloading URL of stm32_serial_upload_tool and arm-none-eabi-gcc，then extract them to
   ```
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\tools
   ```

5. Open `Arduino`, `Tool` -> `Board:`, select the item `Grove STM32F030`.

6. Click `File` -> `Examples` -> `GroveSTM32F030`, then select any one.

7. Click `Verify` button to start compiling.


# Note:
1. Remove all files under
   ```
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\hardware\Grove_STM32F030\
   ```
   but except file `1.0.0` and `*.json`, without this the compiling will be error.

2. After you operate `Arduino Boards Manager`, json file
`C:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_stm32f030_index.json`
will be removed. And board `Grove STM32F030` will miss/disappear in the Boards: menu.
.<br>
You need recopy the file 
`C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\hardware\Grove_STM32F030\package_grove_stm32f030_index.json`
to folder
`C:\Users\<Your-User-Name>\AppData\Local\Arduino15\` again.

3. The target bin or hex file will be in the subfolder of `C:\Users\<Your-User-Name>\AppData\Local\Temp`.
Enable the compiling log feature of Arduino to find the specific path in the log.
If you use bin target file, careful the downloading address of MCU is 0x8000000.


# burn/flash image/elf to STM32F030
Most of time, we can use JLink to flash the MCU STM32F030 device, I think this is a simple method.  
For advanced user play with Grove-RPi-Base-Hat, also can use RPi as a OpenOCD device,  see
[Programming Microcontrollers using OpenOCD on a Raspberry Pi](https://learn.adafruit.com/programming-microcontrollers-using-openocd-on-raspberry-pi?view=all)


# Projects Location
1. Grove Base Hat for PI/PI-Zero
   ```
   1.0.0\libraries\GroveSTM32F030\examples\grove_rpi_base_hat
   ```
   In this example, STM32F030 act as a I2C slave, and capture multichannel ADC results.

2. Grove 5-Way Tactile Switch
   ```
   1.0.0\libraries\GroveSTM32F030\examples\grove_5way_tactile_switch
   ```
   and check the source, make sure below line defined 
   ```
   #define _DEV_TYPE			_DEV_5_WAY_TACTILE_SWITCH
   ```

3. Grove 6-Position DIP Switch
   ```
   1.0.0\libraries\GroveSTM32F030\examples\grove_5way_tactile_switch
   ```
   and check the source, make sure below line defined 
   ```
   #define _DEV_TYPE			_DEV_6_POS_DIP_SWITCH
   ```
