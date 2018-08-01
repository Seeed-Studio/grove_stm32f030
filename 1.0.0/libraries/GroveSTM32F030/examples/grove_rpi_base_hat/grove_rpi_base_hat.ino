/*
 * Name: grove_rpi_base_hat.ino
 *   Grove Base HAT for Raspberry Pi/Pi-Zero device end firmware.
 *
 *
 * Author: turmary <turmary@126.com>
 * Copyright (c) 2018 Seeed Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <Wire.h>
#include <Flash.h>
#include <LowPower.h>
#include <WatchDog.h>
#define _SER_DEBUG	0

/***************************************************************
 Device type & version
 ***************************************************************/
#define _DEV_TYPE			_DEV_BASE_HAT_AUTO
#define _DEV_BASE_HAT_AUTO		0	// probe by PB1
#define _DEV_BASE_HAT_RPI		1
#define _DEV_BASE_HAT_RPI_ZERO		2

const char* grove_base_hat_rpi      = "GROVE BASE HAT RPI";
const char* grove_base_hat_rpi_zero = "GROVE BASE HAT RPI ZERO";
uint16_t devVersion = 0x0002;		// 0.2


/***************************************************************
 Pin & Register definitions
 ***************************************************************/
#define DEV_I2C_ADDRESS			0x04
#define DEVICE_VID			0x2886
#define RPI_HAT_PID			0x0004
#define RPI_ZERO_HAT_PID		0x0005

#define I2C_DEF_ADDR_FLASH_LOC		0x00
#define I2C_CUR_ADDR_FLASH_LOC		0x01

#define DEV_PROBE_PIN			PB1

#define ADC_NR				10
#define ADC_REF_VOLT			(Vdda)
#define ADC_FULL_RANGE			(1 << ADC_RESOLUTION)
uint32_t Vdda = 3300;


enum {
	REG_PID = 0,
	REG_VID,
	REG_VER,
	REG_NAME,         // single char each read, '\0' ends.
	REG_RAW_0 = 0x10, // Analog raw value
	REG_RAW_1,
	REG_RAW_2,
	REG_RAW_3,
	REG_RAW_4,
	REG_RAW_5,
	REG_RAW_6,
	REG_RAW_7,
	REG_RAW_8,        // reserved
	REG_RAW_9,
	REG_VOL_0 = 0x20, // voltage, unit 1 milli Volt
	REG_VOL_1,
	REG_VOL_2,
	REG_VOL_3,
	REG_VOL_4,
	REG_VOL_5,
	REG_VOL_6,
	REG_VOL_7,
	REG_VOL_8,	  // reserved
	REG_VOL_9,
	REG_RTO_0 = 0x30, // ratio, unit 0.1%
	REG_RTO_1,        // adc-input / sensor-vcc
	REG_RTO_2,
	REG_RTO_3,
	REG_RTO_4,
	REG_RTO_5,
	REG_RTO_6,
	REG_RTO_7,
	REG_RTO_8,	  // reserved
	REG_RTO_9,	  // reserved
	REG_VDDA  = 0x40, // Vdda
	REG_CNT,
	REG_SET_ADDR  = 0xC0, // sets device i2c address
	REG_JUMP_BOOT = 0xF0, // jump to stm32 (UART) bootloader
	REG_NULL      = 0xFF, // dummy command
};

struct {
	uint16_t devPID;
	uint16_t devVID;
	uint16_t devType;
	uint16_t devAddr;
	const char* devName;
	uint16_t adcRaw[ADC_NR];
	uint16_t inpVolt[ADC_NR];// voltage, unit 0.001 Volt
	uint16_t ratio[ADC_NR];
	uint8_t regAddr;
} devData[1];

uint8_t adcPins[] = { PA0, PA1,
#if _SER_DEBUG
NONE, NONE,
#else
PA2, PA3,
#endif
PA4, PA5, PA6, PA7, NONE, PB1 };
uint8_t chipId[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/***************************************************************
 ***************************************************************/ 
void requestEvent();
void receiveEvent(int howMany);

/***************************************************************
 * 4-states debug function                                     *
 **************************************************************/
int debug(int v) {
	#if 0
	/*
	 * disable gpio debug
	 * Cause PA13 & PA14 occupy SWD pins
	 */
	pinMode(PA13, OUTPUT);
	pinMode(PA14, OUTPUT);
	digitalWrite(PA13, (v & 0x01)? HIGH: LOW);
	digitalWrite(PA14, (v & 0x02)? HIGH: LOW);
	#elif _SER_DEBUG
	Serial.print(v);
	#endif
	return 0;
}

void setup()
{
	#if _SER_DEBUG
	Serial.begin(115200);
	#endif

	uint8_t i2cDefaultAddr = Flash.read8(I2C_DEF_ADDR_FLASH_LOC);
	uint8_t i2cCurrentAddr = Flash.read8(I2C_CUR_ADDR_FLASH_LOC);

	uint8_t *ptr2 = (uint8_t *) Flash.getChipUniqueID();
	for (uint8_t i = 0; i < sizeof chipId; i++) {
		chipId[i] = *(ptr2 + i);
	}

	debug(0);

	devData->devAddr = DEV_I2C_ADDRESS;
	if (i2cDefaultAddr == 0xff) {
		Flash.write8(I2C_DEF_ADDR_FLASH_LOC, DEV_I2C_ADDRESS);
	}
	if (i2cCurrentAddr == 0xff) {
		Flash.write8(I2C_CUR_ADDR_FLASH_LOC, DEV_I2C_ADDRESS);
	} else {
		devData->devAddr = i2cCurrentAddr;
	}

	// probe device type
	devData->devType = _DEV_BASE_HAT_AUTO;
	pinMode(DEV_PROBE_PIN, INPUT_PULLUP);
	if (digitalRead(DEV_PROBE_PIN) == LOW) {
		devData->devType = _DEV_BASE_HAT_RPI;
	} else {
		devData->devType = _DEV_BASE_HAT_RPI_ZERO;
	}
	pinMode(DEV_PROBE_PIN, INPUT);

	debug(1);

	// set VID & PID & name
	devData->devVID = DEVICE_VID;
	if (devData->devType == _DEV_BASE_HAT_RPI) {
		devData->devPID = RPI_HAT_PID;
		devData->devName = grove_base_hat_rpi;
	} else {
		devData->devPID = RPI_ZERO_HAT_PID;
		devData->devName = grove_base_hat_rpi_zero;
	}

	debug(2);

	devData->regAddr = REG_NULL;

	Wire.begin(devData->devAddr);
	Wire.onReceive(receiveEvent);
	Wire.onRequest(requestEvent);

	// Maxmize ADC resulotuion
	analogReadResolution(ADC_RESOLUTION);

	wwdg.begin();

	debug(3);

	/* Enable SWD interface */
	pinMode(PA13, ALTERNATE);
	pinMode(PA14, ALTERNATE);

	return;
}

// st.com RM0360.pdf
// Calculating the actual Vdda voltage
// using the interval reference voltage
uint32_t getVDDA(void) {
	static uint32_t VREFINT_CAL;
	uint32_t VREFINT_DATA;

	if (VREFINT_CAL == 0UL) {
		// STM32F030x4/6/8/C DataSheet
		// 3.10.2 Internal voltage reference(Vrefint)
		VREFINT_CAL = *((uint16_t*)0x1FFFF7BA);
	}
	VREFINT_DATA = analogRead(A17);
	return 3300UL * VREFINT_CAL / VREFINT_DATA;
}

void dummy(void) {
	static uint32_t intLast = 0;

	if (intLast == 0) {
		intLast = millis();
	}
	if (millis() >= intLast + 1000UL) {
		intLast = millis();
		debug(9);
	}
}

void loop()
{
	uint32_t v;

	dummy();

	// Update ADC_REF_VOLT
	Vdda = getVDDA();

	// Get ADC Value & Voltage
	for (int i = 0; i < ADC_NR; i++) {
		if (adcPins[i] == NONE) {
			devData->adcRaw[i] = 0;
			devData->inpVolt[i] = 0;
			continue;
		}

		devData->adcRaw[i] = analogRead(adcPins[i]);

		v = (uint32_t)ADC_REF_VOLT * devData->adcRaw[i];
		#if 0
		// divider resistance used for RPI version
		if (devData->devType == _DEV_BASE_HAT_RPI) {
			v <<= 1;
		}
		#endif
		v /= ADC_FULL_RANGE;

		if (i == REG_RAW_9 - REG_RAW_0) {
			#if 1
			// all divider resistance be removed
			v = ADC_REF_VOLT;

			#else
			// ADC_IN9 has a 6.8k and 1K divider resistance
			if (devData->devType == _DEV_BASE_HAT_RPI) {
				v >>= 1;
				v *= 6800 + 1000;
				v /= 1000;
			} else if (devData->devType == _DEV_BASE_HAT_RPI_ZERO) {
				v = ADC_REF_VOLT;
			}
			#endif
		}
		devData->inpVolt[i] = v;
	}

	for (int i = 0; i < ADC_NR - 1; i++) {
		v = devData->inpVolt[i];
		v = 1000UL * v / devData->inpVolt[REG_RAW_9 - REG_RAW_0];
		if (v > 1000UL) {
			v = 1000UL;
		}
		devData->ratio[i] = v;
	}

	// change i2c address
	if (devData->regAddr == REG_SET_ADDR) {
		devData->regAddr = REG_NULL;
		Flash.write8(I2C_CUR_ADDR_FLASH_LOC, devData->devAddr);
		Wire.begin(devData->devAddr);
	}

	wwdg.reset();
}

void receiveEvent(int howMany)
{
	uint8_t i = 0, v, receiveBuffer[4] = { 0, };

	while (Wire.available()) {
		v = Wire.read();
		if (i >= sizeof receiveBuffer) {
			continue;
		}
		receiveBuffer[i++] = v;
	}

	devData->regAddr = receiveBuffer[0];

	switch (devData->regAddr) {
	case REG_SET_ADDR:
		devData->devAddr = receiveBuffer[1];
		break;

	case REG_JUMP_BOOT:
		jumpToBootloader();
		break;

	default:
		break;
	}

	#if 0
	if (devData->regAddr >= REG_CNT) {
		devData->regAddr = REG_NULL;
	}
	#endif
	return;
}

void requestEvent(void)
{
	static const char* name = "";
	uint16_t v;

	if (devData->regAddr == REG_NULL) {
		v = 0;
		Wire.write((uint8_t*)&v, sizeof(v));
		return;
	}

	switch (devData->regAddr) {
	case REG_PID:
		Wire.write((uint8_t*)&devData->devPID, sizeof devData->devPID);
		break;

	case REG_VID:
		Wire.write((uint8_t*)&devData->devVID, sizeof devData->devVID);
		break;

	case REG_VER:
		Wire.write((uint8_t*)&devVersion, sizeof devVersion);
		break;

	case REG_NAME:
		v = (*name) & 0xFF;
		Wire.write((uint8_t*)&v, sizeof v);

		if (!v) {
			name = devData->devName;
		} else {
			name++;
		}
		break;

	default:
		if (REG_RAW_0 <= devData->regAddr && devData->regAddr <= REG_RAW_9) {
			v = devData->regAddr - REG_RAW_0;
			Wire.write((uint8_t*)&devData->adcRaw[v], sizeof(uint16_t));
		} else if (REG_VOL_0 <= devData->regAddr && devData->regAddr <= REG_VOL_9) {
			v = devData->regAddr - REG_VOL_0;
			Wire.write((uint8_t*)&devData->inpVolt[v], sizeof(uint16_t));
		} else if (REG_RTO_0 <= devData->regAddr && devData->regAddr <= REG_RTO_9) {
			v = devData->regAddr - REG_RTO_0;
			Wire.write((uint8_t*)&devData->ratio[v], sizeof(uint16_t));
		} else if (devData->regAddr == REG_VDDA) {
			v = Vdda;
			Wire.write((uint8_t*)&v, sizeof v);
		} else if (devData->regAddr != REG_NULL) {
			v = 0;
			Wire.write((uint8_t*)&v, sizeof v);
		}
		break;
	}

	#if 0
	devData->regAddr = REG_NULL;
	#else
	// continuous reading is allowed
	#endif
	return;
}
