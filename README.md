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

4. ���ı��༭����json�ļ�
   ```
   CC:\Users\<Your-User-Name>\AppData\Local\Arduino15\package_grove_stm32f030_index.json
   ```
   �ҵ�stm32_serial_upload_tool��arm-none-eabi-gcc�����ص�ַ�����غ��ѹ��
   ```
   C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\tools
   ```

5. ��Arduino,"����"->"������",ѡ��Ϊ "Grove STM32F030"

6. ��� "�ļ�"->"ʾ��",ѡ��"Examples for Grove STM32F030" ����������

7. ��� "��֤" ����ʼ����


# Note:
ɾ���ļ���
```
C:\Users\<Your-User-Name>\AppData\Local\Arduino15\packages\Seeeduino\hardware\Grove_STM32F030\
```
�³�1.0.0����������ļ����ļ��У�����Arduino���뱨��


# Projects Location
1. Grove Base Hat for PI/PI-Zero
```
   1.0.0\libraries\GroveSTM32F030\examples\grove_rpi_base_hat
```

