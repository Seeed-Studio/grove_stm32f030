# Usage for Windows:

1. 安装Arduino version >= 1.6, Git for Windows with MSYS

2. 打开Git for Windows,运行如下命令
```
cd ~/AppData/Local/Arduino15/
mkdir -p packages/Seeeduino/hardware
mkdir -p packages/Seeeduino/tools
cd packages/Seeeduino/hardware
git clone https://gitlab.com/seeedstudio/grove_stm32f030.git Grove_STM32F030
cp Grove_STM32F030/package_grove_stm32f030_index.json ../../../
```

4. 用文本编辑器打开json文件
   ```
   CC:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_stm32f030_index.json
   ```
   找到stm32_serial_upload_tool和arm-none-eabi-gcc的下载地址，下载后解压到
   ```
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\tools
   ```

5. 打开Arduino,"工具"->"开发板",选择为 "Grove STM32F030"

6. 点击 "文件"->"示例",选择"Examples for Grove STM32F030" 中任意例程

7. 点击 "验证" 即开始编译


# Note:
1. 删除文件夹
```
C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\hardware\Grove_STM32F030\
```
下除1.0.0以外的所有文件和文件夹(除了.json文件)，否则Arduino编译报错。

2. 当使用arduino IDE的开发板管理功能时，
 C:\Users\<Your-User-Name>\AppData\Local\Arduino15\  目录下的 package_grove_stm32f030_index.json文件会被系统删除，出现在arduino IDE 选择开发板时没有
Grove STM32F030 选项，这时候需要将 C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\hardware\Grove_STM32F030 路径下的 package_grove_stm32f030_index.json 文件
重新复制到 C:\Users\<Your-User-Name>\AppData\Local\Arduino15\ 目录下

3. 例程编译出来的bin文件或hex文件在 C:\Users\<Your-User-Name>\AppData\Local\Temp 目录下，相对应的文件可以查看arduino IDE的下方的编译log.如果是bin文件，文件下载开始地址为0x8000000,SWD下载方式。
# Projects Location
1. Grove Base Hat for PI/PI-Zero
```
   1.0.0\libraries\GroveSTM32F030\examples\grove_rpi_base_hat，//这个例程是多通道ADC，stm32f030作为从机
```

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

