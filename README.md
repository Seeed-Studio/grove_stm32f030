# Usage for Windows:

1. ��װArduino version >= 1.6, Git for Windows with MSYS

2. ��Git for Windows,������������
```
cd ~/AppData/Local/Arduino15/
mkdir -p packages/Seeeduino/hardware
mkdir -p packages/Seeeduino/tools
cd packages/Seeeduino/hardware
git clone https://gitlab.com/seeedstudio/grove_stm32f030.git Grove_STM32F030
cp Grove_STM32F030/package_grove_stm32f030_index.json ../../../
```

3. ���ı��༭����json�ļ�
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_zero_boards_index.json
   �ҵ�stm32_serial_upload_tool��arm-none-eabi-gcc�����ص�ַ�����غ��ѹ��
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\tools

4. ��Arduino,����->������,ѡ��Ϊ "Grove STM32F030"

5. ��� �ļ�->ʾ��,ѡ��"Examples for Grove STM32F030" ����������

6. ��� ��֤����ʼ����


