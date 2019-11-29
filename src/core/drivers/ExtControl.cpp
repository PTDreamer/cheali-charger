/*
 * ExtControl.cpp
 *
 *  Created on: Nov 22, 2019
 *      Author: jose
 */

#include "ExtControl.h"
#include "Keyboard.h"
#include "Settings.h"
#include "Hardware.h"
#include <string.h>
#include "memory.h"
#include "eeprom.h"
#include "Serial.h"
#include "Buzzer.h"
#include "SerialLog.h"
#include "Time.h"

#ifdef ENABLE_SERIAL_CONTROL

ExtControl::ExtControl() {
	currentProcessStatus = WAITING_MAGIC1;
	currentMessageNumber_sent = 0;
	currentMessageNumber_received = 0;
	messageSizes[ACK] = 1;//messageNumber
	messageSizes[NACK] = 1;//messageNumber
	messageSizes[GET_OPTIONS] = 0;
	messageSizes[GET_BAT_SETTINGS] = 1; //index
	messageSizes[SET_VOLATILE_BAT] = sizeof(ProgramData::Battery); //TODO
	messageSizes[PUT_OPTIONS] = sizeof(Settings);
	messageSizes[PUT_BAT_SETTINGS] = sizeof(batteryInfo);
	messageSizes[COMMAND] = 1;
	currentState = STATE_NOT_CONTROLING;
	isUnderControl = 0;
	uint8_t currentTypeSent = 0;
	uint8_t currentSentAcked = 1;
}
void ExtControl::powerOn() {

}
void ExtControl::doIdle() {
	uint8_t dataReady = 0;
	if (!settings.serialControl)
		return;
	if (!isUnderControl) {
		uint8_t key;
		//BUTTON_INC
		key = Keyboard::getLast();
		if (key == BUTTON_INC && Keyboard::isExtSerialCommand()) {
			Buzzer::soundProgramComplete();
			SerialLog::serialBegin();
			SerialLog::printString("INIT");
			Serial::beginRx(settings.getUARTspeed());
			isUnderControl = 1;
		}
	}
	if (isUnderControl) {
		int c = -1;
		do {
			c = Serial::read();
			if (c != -1) {
				dataReady = processInputByte(c);
			}
		} while (c != -1);
	}

}
uint8_t ExtControl::processInputByte(int c) {
	static uint8_t currentData = 0;
	static uint8_t currentMsgSize = 0;
	static uint16_t currentCrc = 0xffff;
	static uint8_t currentNumber = 0;
	static message_type msgType = ACK;
	static uint16_t receivedCrc = 0;
	SerialLog::printInt(currentProcessStatus);
	switch (currentProcessStatus) {
	case WAITING_MAGIC1:
		if (c == MAGIC1)
			currentProcessStatus = WAITING_MAGIC2;
		break;
	case WAITING_MAGIC2:
		if (c == MAGIC2) {
			currentProcessStatus = WAITING_NUMBER;
			currentData = 0;
			currentCrc = 0xffff;
		}
		break;
	case WAITING_NUMBER:
		currentNumber = c;
		currentCrc = crc16_update(currentCrc, c);
		currentProcessStatus = WAITING_TYPE;
		break;
	case WAITING_TYPE:
		if(c > (LAST_MESSAGE_TYPE -1)) {
			currentProcessStatus = WAITING_MAGIC1;
			return 0;
		}
		currentMsgSize = messageSizes[c];
		msgType = (message_type)c;
		currentCrc = crc16_update(currentCrc, c);
		SerialLog::printUInt(c);
		if(currentMsgSize)
			currentProcessStatus = WAITING_DATA;
		else
			currentProcessStatus = WAITING_CRC1;
		break;
	case WAITING_DATA:
		rx_message[currentData] = c;
		currentCrc = crc16_update(currentCrc, c);
		++currentData;
		if(currentData == currentMsgSize) {
			currentProcessStatus = WAITING_CRC1;
		}
		break;
	case WAITING_CRC1:
		receivedCrc = c;
		currentProcessStatus = WAITING_CRC2;
		break;
	case WAITING_CRC2:
		currentProcessStatus = WAITING_MAGIC1;
		receivedCrc = receivedCrc << 8;
		receivedCrc = receivedCrc | c;
		if(receivedCrc == currentCrc) {
			sendPackage(&currentNumber, ACK);
			switch (msgType) {
				case GET_OPTIONS:
					processGetOptions();
					break;
				case GET_BAT_SETTINGS:
					processGetBatSettings(rx_message[0]);
					break;
				case SET_VOLATILE_BAT:
					processVolatileBat();
					break;
				default:
					break;
			}
			return 1;
		}
		else {
			SerialLog::printString("wrong CRC");
			sendPackage(&currentNumber, NACK);
		}
		break;
	default:
		return 0;
		break;
	}
	return 0;
}

void ExtControl::processVolatileBat() {
	memcpy(&volatileBattery, rx_message, sizeof(ProgramData::Battery));
	ProgramData::check();
}
void ExtControl::processGetOptions() {
	sendPackage((uint8_t*)&settings, PUT_OPTIONS);
}
void ExtControl::processGetBatSettings(uint8_t index) {
	batteryInfo msg;
	msg.index = index;
    eeprom::read(msg.battery, &eeprom::data.battery[index]);
    sendPackage((uint8_t*)&msg, PUT_BAT_SETTINGS);
}
void ExtControl::sendPackage(uint8_t *buffer, message_type type) {
	if(!currentSentAcked)
		return;
	currentSentAcked = 0;
	currentTypeSent = type;
	currentSentTime = Time::getMilisecondsU16();
	memcpy(tx_message, buffer, messageSizes[type]);
	uint16_t crc = 0xffff;
	Serial::write(MAGIC1);
	Serial::write(MAGIC2);
	Serial::write(currentMessageNumber_sent);
	crc = crc16_update(crc, currentMessageNumber_sent);
	++currentMessageNumber_sent;
	Serial::write(type);
	crc = crc16_update(crc, type);
	for(uint8_t i = 0; i < messageSizes[type]; ++i) {
		crc = crc16_update(crc, buffer[i]);
		Serial::write(buffer[i]);
	}
	Serial::write(crc >> 8);
	Serial::write(crc & 0xff);
}
uint16_t ExtControl::crc16_update(uint16_t crc, uint8_t a) {
	int i;

	crc ^= a;
	for (i = 0; i < 8; ++i) {
		if (crc & 1)
			crc = (crc >> 1) ^ 0xA001;
		else
			crc = (crc >> 1);
	}

	return crc;
}
void ExtControl::powerOff() {

}

ExtControl extControl;
#endif
