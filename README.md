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

3. 用文本编辑器打开json文件
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_zero_boards_index.json
   找到stm32_serial_upload_tool和arm-none-eabi-gcc的下载地址，下载后解压到
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\tools

4. 打开Arduino,工具->开发板,选择为 "Grove STM32F030"

5. 点击 文件->示例,选择"Examples for Grove STM32F030" 中任意例程

6. 点击 验证即开始编译


