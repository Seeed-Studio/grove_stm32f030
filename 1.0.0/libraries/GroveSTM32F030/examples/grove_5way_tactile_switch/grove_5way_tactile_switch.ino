/*
 * Name: grove_5way_tactile_switch.ino
 *   Grove 5-Way Tactile & Grove 6-Position DIP Switch device end firmware.
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

/***************************************************************
 Device type & version
 ***************************************************************/
#define _DEV_TYPE			_DEV_5_WAY_TACTILE_SWITCH
#define _DEV_5_WAY_TACTILE_SWITCH	0
#define _DEV_6_POS_DIP_SWITCH		1

#define VERSIONS_SZ			10
const char versions[VERSIONS_SZ] =
#if   _DEV_TYPE == _DEV_5_WAY_TACTILE_SWITCH
"BN-5E-"				// 5 buttons, with event support
#elif _DEV_TYPE == _DEV_6_POS_DIP_SWITCH
"BN-6--"				// 6 buttons, no event
#else
#error Please specify proper device type _DEV_TYPE
#endif
"0.1";					// real version


/***************************************************************
 Pin definitions
 ***************************************************************/
#define GROVE_TWO_RX_PIN_NUM	PF0

#define BUTTON_LED_PIN_NUM	PA13

#if   _DEV_TYPE == _DEV_5_WAY_TACTILE_SWITCH
#define BUTTON_A_PIN_NUM	PA4
#define BUTTON_B_PIN_NUM	PA5
#define BUTTON_C_PIN_NUM	PA6
#define BUTTON_D_PIN_NUM	PA7
#define BUTTON_E_PIN_NUM	PA1
#elif _DEV_TYPE == _DEV_6_POS_DIP_SWITCH
#define BUTTON_A_PIN_NUM	PA1
#define BUTTON_B_PIN_NUM	PA4
#define BUTTON_C_PIN_NUM	PA5
#define BUTTON_D_PIN_NUM	PA6
#define BUTTON_E_PIN_NUM	PA7
#define BUTTON_F_PIN_NUM	PA0
#endif

/***************************************************************

 ***************************************************************/
#define SINGLE_KEY_TIME     3	//  SINGLE_KEY_TIME * 10MS = 30MS
#define KEY_INTERVAL        30	//  KEY_INTERVAL * 10MS    = 300MS
#define LONG_KEY_TIME       200	//  LONG_KEY_TIME * 10MS   = 2S

static uint8_t button_pins[] = {
	BUTTON_A_PIN_NUM,
	BUTTON_B_PIN_NUM,
	BUTTON_C_PIN_NUM,
	BUTTON_D_PIN_NUM,
	BUTTON_E_PIN_NUM,
#if _DEV_TYPE == _DEV_6_POS_DIP_SWITCH
	BUTTON_F_PIN_NUM,
#endif
};
#define BUTTON_NR           (sizeof button_pins / sizeof button_pins[0])

enum {
	KEY_STATE_IDLE = 0,	//  no click
	KEY_STATE_DOWN,		//  single click
	KEY_STATE_ONESHOT,	//  double click
	KEY_STATE_LONG,		//  long press
};
#define DIGITAL_BTN_PRESSED	LOW

enum button_event_t {
	BTN_EV_NO_EVENT     = 0,
	BTN_EV_HAS_EVENT    = 0x80000000UL,
	BTN_EV_RAW_STATUS   = 1UL << 0,
	BTN_EV_SINGLE_CLICK = 1UL << 1,
	BTN_EV_DOUBLE_CLICK = 1UL << 2,
	BTN_EV_LONG_PRESS   = 1UL << 3,
	BTN_EV_LEVEL_CHANGED= 1UL << 4,
};

uint32_t clickPreviousMillis = 0;
uint32_t buttonEventRead(void);
int buttonClearEvent(void);
static int buttonRawRead(void);

/***************************************************************

 ***************************************************************/
#define BUTTON_I2C_ADDRESS		0x03
#define DEVICE_VID			0x2886
#if   _DEV_TYPE == _DEV_5_WAY_TACTILE_SWITCH
#define DEVICE_PID			0x0002
#elif _DEV_TYPE == _DEV_6_POS_DIP_SWITCH
#define DEVICE_PID			0x0003
#endif

#define I2C_DEF_ADDR_FLASH_LOC		0x00
#define I2C_CUR_ADDR_FLASH_LOC		0x01

#define I2C_CMD_GET_DEV_ID		0x00	// gets device ID information
#define I2C_CMD_GET_DEV_EVENT		0x01	// gets device event status
#define I2C_CMD_EVENT_DET_MODE		0x02	// enable button event detect mode
#define I2C_CMD_BLOCK_DET_MODE		0x03	// enable button block detect mode
#define I2C_CMD_SET_ADDR		0xc0	// sets device i2c address
#define I2C_CMD_RST_ADDR		0xc1	// resets device i2c address
#define I2C_CMD_TEST_TX_RX_ON		0xe0	// enable TX RX pin test mode
#define I2C_CMD_TEST_TX_RX_OFF		0xe1	// disable TX RX pin test mode
#define I2C_CMD_TEST_GET_VER		0xe2	// use to get software version
#define I2C_CMD_GET_DEVICE_UID		0xf1	// use to get chip id

#define I2C_CMD_JUMP_TO_BOOT		0xf0	// jump to stm32 (UART) bootloader
#define I2C_CMD_NULL			0xff	//

uint16_t deviceI2CAddress = BUTTON_I2C_ADDRESS;
uint8_t commandReceive = I2C_CMD_NULL;

struct {
	uint16_t devicePID;
	uint16_t deviceVID;
} idBuf;

struct {
	#define _DETECT_MODE_EVENT	0x01
	#define _DETECT_MODE_BLOCK	0x02
	uint16_t detect_mode;
	uint16_t state[BUTTON_NR];
	uint16_t duration[BUTTON_NR];	// press duration
	uint16_t distance[BUTTON_NR];	// from the time last pressing released
	uint8_t last_evts[BUTTON_NR];
	uint8_t dither[BUTTON_NR];

	uint32_t event;
	uint8_t button_evts[BUTTON_NR];
} btnData[1];

uint8_t chipId[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/***************************************************************

 ***************************************************************/
LowPower nrgSave;

#define CLICK_CHECK_TIMEOUT	1000
#define AUTO_SLEEP_TIMEOUT	2000
uint32_t clickCheckPreviousMillis = 0;
uint32_t autoSleepPreviousMillis = 0;
bool autoSleepFlag = false;
bool testFlag = false;

/***************************************************************

 ***************************************************************/

void requestEvent();
void receiveEvent(int howMany);

/***************************************************************

 ***************************************************************/

void setup()
{
	uint8_t i2cDefaultAddr = Flash.read8(I2C_DEF_ADDR_FLASH_LOC);
	uint8_t i2cCurrentAddr = Flash.read8(I2C_CUR_ADDR_FLASH_LOC);

	uint8_t *ptr2 = (uint8_t *) Flash.getChipUniqueID();
	for (uint8_t i = 0; i < sizeof chipId; i++)
		chipId[i] = *(ptr2 + i);

	if (i2cDefaultAddr == 0xff)
		Flash.write8(I2C_DEF_ADDR_FLASH_LOC, BUTTON_I2C_ADDRESS);
	if (i2cCurrentAddr == 0xff)
		Flash.write8(I2C_CUR_ADDR_FLASH_LOC, BUTTON_I2C_ADDRESS);
	else
		deviceI2CAddress = i2cCurrentAddr;

	// The pin need pull up by a resistance
	pinMode(GROVE_TWO_RX_PIN_NUM, INPUT_PULLUP);
	nrgSave.begin(GROVE_TWO_RX_PIN_NUM, dummy, CHANGE);

	for (int i = 0;i < BUTTON_NR; i++) {
		pinMode(button_pins[i], INPUT_PULLUP);
	}

	idBuf.deviceVID = DEVICE_VID;
	idBuf.devicePID = DEVICE_PID;
	buttonRawRead();
	buttonClearEvent();

	Wire.begin(deviceI2CAddress);
	Wire.onReceive(receiveEvent);
	Wire.onRequest(requestEvent);

	wwdg.begin();
}

void loop()
{
	button_event_t event;

	if (btnData->detect_mode == _DETECT_MODE_EVENT)
		event = buttonEventdetect();
	else
		event = buttonBlockDetect();

	if (event != BTN_EV_NO_EVENT) {
		btnData->event = event;
		clickCheckPreviousMillis = millis();
	} else if (btnData->event) {
		uint32_t clickCheckCurrentMillis = millis();

		if (clickCheckCurrentMillis - clickCheckPreviousMillis >= CLICK_CHECK_TIMEOUT) {
			clickCheckPreviousMillis = clickCheckCurrentMillis;
			buttonClearEvent();
		}
	}

	if (commandReceive == I2C_CMD_SET_ADDR) {	// change i2c address
		commandReceive = I2C_CMD_NULL;
		Flash.write8(I2C_CUR_ADDR_FLASH_LOC, deviceI2CAddress);
		Wire.begin(deviceI2CAddress);
	} else if (commandReceive == I2C_CMD_RST_ADDR) {// reset i2c address
		commandReceive = I2C_CMD_NULL;
		deviceI2CAddress = Flash.read8(I2C_DEF_ADDR_FLASH_LOC);
		Flash.write8(I2C_CUR_ADDR_FLASH_LOC, deviceI2CAddress);
		Wire.begin(deviceI2CAddress);
	}

	if (autoSleepFlag) {
		uint32_t autoSleepCurrentMillis = millis();

		if ((autoSleepCurrentMillis - autoSleepPreviousMillis) > AUTO_SLEEP_TIMEOUT) {
			autoSleepPreviousMillis = autoSleepCurrentMillis;

			wwdg.end();
			Wire.end();
			pinMode(PA9, INPUT_PULLUP);
			pinMode(PA10, INPUT_PULLUP);

			nrgSave.standby();

			Wire.begin(deviceI2CAddress);
			Wire.onReceive(receiveEvent);
			Wire.onRequest(requestEvent);
			wwdg.begin();
		}
	}

	if (testFlag) {
		wwdg.end();
		pinMode(GROVE_TWO_RX_PIN_NUM, OUTPUT);

		while (1) {
			digitalWrite(GROVE_TWO_RX_PIN_NUM, HIGH);
			delay(1);

			digitalWrite(GROVE_TWO_RX_PIN_NUM, LOW);
			delay(1);

			if (testFlag == false)
				break;
		}

		wwdg.begin();
		attachInterrupt(GROVE_TWO_RX_PIN_NUM, dummy, CHANGE, INPUT_PULLUP);
	}

	wwdg.reset();
}

button_event_t buttonEventdetect(void)
{
	button_event_t event = BTN_EV_NO_EVENT;
	uint32_t clickCurrentMillis = millis();

	if (clickCurrentMillis - clickPreviousMillis >= 10) {
		clickPreviousMillis = clickCurrentMillis;
		event = (button_event_t) buttonEventRead();
	}
	return event;
}

static int buttonRawRead(void) {
	int r;

	r = BTN_EV_NO_EVENT;
	for (int i = 0;i < BUTTON_NR; i++) {
		if (digitalRead(button_pins[i]) != DIGITAL_BTN_PRESSED) {
			btnData->button_evts[i] |= BTN_EV_RAW_STATUS;
			continue;
		}
		btnData->button_evts[i] &= ~BTN_EV_RAW_STATUS;
		r = BTN_EV_HAS_EVENT;
	}
	return r;
}

button_event_t buttonBlockDetect(void)
{
	button_event_t event = BTN_EV_NO_EVENT;
	uint32_t clickCurrentMillis = millis();

	if (clickCurrentMillis - clickPreviousMillis >= 10) {
		clickPreviousMillis = clickCurrentMillis;

		event = (button_event_t)buttonRawRead();
	}
	return event;
}

void dummy(void)
{
	static uint32_t intStart = 0;
	static uint32_t intEnd = 0;

	autoSleepPreviousMillis = millis();

	if (digitalRead(GROVE_TWO_RX_PIN_NUM) == LOW) {
		intStart = millis();
	} else {
		intEnd = millis();
		if ((intEnd - intStart) > 20) {
			delay(500);
		} else {
			intStart = intEnd;
		}
	}
}

void receiveEvent(int howMany)
{
	uint8_t i = 0, v, receiveBuffer[4] = { 0, };
	// autoSleepPreviousMillis = millis();

	while (Wire.available()) {
		v = Wire.read();
		if (i >= sizeof receiveBuffer) {
			continue;
		}
		receiveBuffer[i++] = v;
	}

	commandReceive = receiveBuffer[0];

	switch (commandReceive) {
	case I2C_CMD_EVENT_DET_MODE:
		btnData->detect_mode = _DETECT_MODE_EVENT;
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_BLOCK_DET_MODE:
		btnData->detect_mode = _DETECT_MODE_BLOCK;
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_SET_ADDR:
		deviceI2CAddress = receiveBuffer[1];
		break;

	case I2C_CMD_TEST_TX_RX_ON:
		testFlag = true;
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_TEST_TX_RX_OFF:
		testFlag = false;
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_JUMP_TO_BOOT:
		commandReceive = I2C_CMD_NULL;
		jumpToBootloader();
		break;

	default:
		break;
	}
}

void requestEvent(void)
{
	// autoSleepPreviousMillis = millis();

	switch (commandReceive) {
	case I2C_CMD_GET_DEV_ID:
		Wire.write((uint8_t*)&idBuf, sizeof idBuf);
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_GET_DEV_EVENT:
		Wire.write((uint8_t*)&btnData->event, sizeof btnData->event
		                            + sizeof btnData->button_evts);
		commandReceive = I2C_CMD_NULL;
		buttonClearEvent();
		break;

	case I2C_CMD_TEST_GET_VER:
		Wire.write(versions, sizeof versions);
		commandReceive = I2C_CMD_NULL;
		break;

	case I2C_CMD_GET_DEVICE_UID:
		Wire.write(chipId, sizeof chipId);
		commandReceive = I2C_CMD_NULL;
		break;

	default:
		break;
	}
}

/***************************************************************

 ***************************************************************/
int buttonEventFSM(int index)
{
	#define KEY_STATE	btnData->state[index]
	#define KEY_DUR		btnData->duration[index]
	#define KEY_LAST	btnData->distance[index]
	uint8_t keyPress;
	int r;

	r = 0;
	keyPress = !(btnData->button_evts[index] & BTN_EV_RAW_STATUS);

	KEY_LAST++;

	switch (KEY_STATE) {
	case KEY_STATE_IDLE:
		if (keyPress) {
			KEY_DUR = 0;
			KEY_STATE = KEY_STATE_DOWN;
		}
		break;

	case KEY_STATE_DOWN:
		if (keyPress) {
			KEY_DUR++;
			if (KEY_DUR >= SINGLE_KEY_TIME) {
				KEY_STATE = KEY_STATE_ONESHOT;
			}
		} else {
			KEY_STATE = KEY_STATE_IDLE;
		}
		break;

	case KEY_STATE_ONESHOT:
		if (!keyPress) {
			r = (KEY_LAST >= KEY_INTERVAL)? BTN_EV_SINGLE_CLICK: BTN_EV_DOUBLE_CLICK;
			KEY_STATE = KEY_STATE_IDLE;
		} if (++KEY_DUR >= LONG_KEY_TIME) {
			r = BTN_EV_LONG_PRESS;
			KEY_STATE = KEY_STATE_LONG;
		}
		break;

	case KEY_STATE_LONG:
		if (!keyPress) {
			KEY_STATE = KEY_STATE_IDLE;
			KEY_LAST = KEY_INTERVAL;
		}
		break;

	default:
		KEY_STATE = KEY_STATE_IDLE;
		break;
	}

	if (r == BTN_EV_DOUBLE_CLICK) {
		KEY_LAST = KEY_INTERVAL;
	} else if (r == BTN_EV_SINGLE_CLICK) {
		KEY_LAST = 0;
	}

	return r;
	#undef KEY_STATE
	#undef KEY_DUR
	#undef KEY_LAST
}

int buttonClearEvent(void) {
	btnData->event = BTN_EV_NO_EVENT;

	for (int i = 0; i < BUTTON_NR; i++) {
		// clear all events except raw
		btnData->button_evts[i] &= BTN_EV_RAW_STATUS;
		btnData->last_evts[i] = btnData->button_evts[i];
	}
	return 0;
}

uint32_t buttonEventRead(void)
{
	button_event_t event;
	int r;

	buttonRawRead();

	event = BTN_EV_NO_EVENT;
	for (int i = 0; i < BUTTON_NR; i++) {
		if ((btnData->button_evts[i] ^ btnData->last_evts[i]) & BTN_EV_RAW_STATUS) {
			if (++btnData->dither[i] > SINGLE_KEY_TIME) {
				btnData->last_evts[i] = btnData->button_evts[i];
				btnData->button_evts[i] |= BTN_EV_LEVEL_CHANGED;
				event = BTN_EV_HAS_EVENT;
			}
		} else {
			btnData->dither[i] = 0;
		}

#if   _DEV_TYPE == _DEV_5_WAY_TACTILE_SWITCH
		if ((r = buttonEventFSM(i)) == 0) {
			continue;
		}
		btnData->button_evts[i] |= r;
		event = BTN_EV_HAS_EVENT;
#endif
	}

	return event;
}
