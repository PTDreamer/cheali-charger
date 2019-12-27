/*
 * ExtControl::cpp
 *
 *  Created on: Nov 22, 2019
 *      Author: jose
 */

#include "ExtControl.h"
#ifndef QT
#include "Settings.h"
#include "Hardware.h"
#include <string.h>
#include "memory.h"
#include "eeprom.h"
#include "Serial.h"
#include "Time.h"
#include "TheveninMethod.h"
#include "StackInfo.h"
#include "Balancer.h"
#include "Monitor.h"
#endif
#ifdef ENABLE_EXTERNAL_CONTROL

#define MAX_RESENDS	5
#define SEND_TIMEOUT 1000
#define VIRTUAL_SEND_FREQUENCY 5000
#define EXTRA_SEND_FREQUENCY 8000
#define MAX_BACKLOG 10

namespace ExtControl {
struct fifo {
	uint8_t head;
	uint8_t tail;
	uint8_t elements;
	uint8_t buffer[MAX_BACKLOG];
};
void sendRealValues();
void sendVirtualValues();
void sendExtraValues();
void pushFifo(uint8_t, fifo *);
int8_t popFifo(fifo *);
int8_t fifoSize(fifo *f) {return f->elements;}
static uint8_t rx_message[sizeof(largestSize)];
static uint8_t tx_message[sizeof(largestSize)];
static uint8_t currentMessageNumber_sent = 0;
static uint8_t currentMessageNumber_received = 0;
static message_type currentTypeSent = ACK;
static uint8_t currentSentAcked = true;
static uint16_t currentSentTime = 0;
command_type currentCommand = CMD_IDLE;
commandDataType commandData = {};
ProgramData::Battery volatileBattery;
bool isSendingAllBattery = false;
error_type current_error = ERROR_NONE;
static bool isInit = false;

static uint8_t messageSizes[LAST_MESSAGE_TYPE];
static packet_process_status currentProcessStatus = WAITING_MAGIC1;
current_state_type currentState = STATE_NOT_CONTROLING;
static fifo statesBacklog;
static fifo errorsBacklog;
static fifo programsBacklog;

static uint16_t lastExtraSentTime = 0;
static uint16_t lastVirtualSentTime = 0;
Program::ProgramType currentProgram = Program::LAST_PROGRAM_TYPE;
#ifdef QT
helper *help;
#endif
}

void ExtControl::init() {
	messageSizes[ACK] = 1; //messageNumber
	messageSizes[NACK] = 1; //messageNumber
	messageSizes[GET_OPTIONS] = 0;
	messageSizes[GET_BAT_SETTINGS] = 1; //index
	messageSizes[SET_VOLATILE_BAT] = sizeof(ProgramData::Battery); //TODO
	messageSizes[PUT_OPTIONS] = sizeof(Settings);
	messageSizes[SET_OPTIONS] = sizeof(Settings);
	messageSizes[PUT_BAT_SETTINGS] = sizeof(batteryInfo);
	messageSizes[SET_BAT_SETTINGS] = sizeof(batteryInfo); //index
	messageSizes[COMMAND] = sizeof(commandType);
	messageSizes[STATE_CHANGED] = 1;
	messageSizes[PROGRAM_CHANGED] = 1;
	messageSizes[ERROR_OCORRED] = 1;
	messageSizes[PUT_REAL_INPUTS] = sizeof(realInputs);
	messageSizes[PUT_VIRTUAL_INPUTS] = sizeof(virtualInputs);
#ifdef EXTCONTROL_DEBUG
	messageSizes[DEBUG_MSG] = 10;
#endif
	isInit = true;
}
void ExtControl::doIdle() {
	uint8_t temp;
    static uint16_t analogCount;
	uint8_t dataReady = 0;
	if (!isInit)
		init();
	if (settings.UART != Settings::ExtControl)
		return;
	if(currentState == STATE_NOT_CONTROLING)
		return;
	if (!currentSentAcked
			&& Time::diffU16(currentSentTime, Time::getMilisecondsU16()) > SEND_TIMEOUT)
		sendPackage(tx_message, currentTypeSent, true);
#ifndef QT
	else if (currentSentAcked && isSendingAllBattery)
		processGetBatSettings(ALL_BAT_SETTINGS);
	else if (currentSentAcked && fifoSize(&statesBacklog) > 0) {
		temp = popFifo(&statesBacklog);
		sendPackage((uint8_t *) &temp, ExtControl::STATE_CHANGED);
	}
	else if (currentSentAcked && fifoSize(&errorsBacklog) > 0) {
		temp = popFifo(&errorsBacklog);
		sendPackage((uint8_t *) &temp, ExtControl::ERROR_OCORRED);
	}
	else if (currentSentAcked && fifoSize(&programsBacklog) > 0) {
		temp = popFifo(&programsBacklog);
		sendPackage((uint8_t *) &temp, ExtControl::PROGRAM_CHANGED);
	}
	if(currentSentAcked && (currentState == STATE_RUNNING || currentState == STATE_SETUP_COMPLETED)) {
		    if(!AnalogInputs::isPowerOn()) {
		        analogCount = 0;
	} else {
		uint16_t c = AnalogInputs::getFullMeasurementCount();
		if (analogCount != c) {
			analogCount = c;
			sendRealValues();
		} else if (Time::diffU16(lastVirtualSentTime,
				Time::getMilisecondsU16()) > VIRTUAL_SEND_FREQUENCY) {
			lastVirtualSentTime = Time::getMilisecondsU16();
			sendVirtualValues();
		} else if (Time::diffU16(lastExtraSentTime,
				Time::getMilisecondsU16()) > EXTRA_SEND_FREQUENCY) {
			lastExtraSentTime = Time::getMilisecondsU16();
			sendExtraValues();
		}
	}
	 }
#endif
	 if(true){//TODO MAYBE TAKE OUT THE IF
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
	static message_type msgType = ACK;
	static uint16_t receivedCrc = 0;
	switch (currentProcessStatus) {
	case WAITING_MAGIC1:
		if (c == MAGIC1) {
			currentProcessStatus = WAITING_MAGIC2;
		}
		break;
	case WAITING_MAGIC2:
		if (c == MAGIC2) {
			currentProcessStatus = WAITING_NUMBER;
			currentData = 0;
			currentCrc = 0xffff;
		}
		break;
	case WAITING_NUMBER:
		currentMessageNumber_received = c;
		currentCrc = crc16_update(currentCrc, c);
		currentProcessStatus = WAITING_TYPE;
		break;
	case WAITING_TYPE:
		if (c > (LAST_MESSAGE_TYPE - 1)) {
			currentProcessStatus = WAITING_MAGIC1;
			return 0;
		}
		currentMsgSize = messageSizes[c];
		msgType = (message_type) c;
		currentCrc = crc16_update(currentCrc, c);
		if (currentMsgSize)
			currentProcessStatus = WAITING_DATA;
		else
			currentProcessStatus = WAITING_CRC1;
		break;
	case WAITING_DATA:
		rx_message[currentData] = c;
		currentCrc = crc16_update(currentCrc, c);
		++currentData;
		if (currentData == currentMsgSize) {
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
		if (receivedCrc == currentCrc) {
			if (msgType != ACK && msgType != NACK && msgType != PUT_REAL_INPUTS &&  msgType != PUT_VIRTUAL_INPUTS && msgType != PUT_EXTRA_VALUES)
				sendPackage(&currentMessageNumber_received, ACK);
#ifdef QT
			help->trigger_messageReceived(msgType, rx_message);
#endif
			switch (msgType) {
			case ACK:
				if (rx_message[0] == (currentMessageNumber_sent - 1))
					currentSentAcked = true;
				break;
			case GET_OPTIONS:
				processGetOptions();
				break;
			case SET_OPTIONS:
				processSetOptions();
				break;
			case GET_BAT_SETTINGS:
				processGetBatSettings(rx_message[0]);
				break;
			case SET_BAT_SETTINGS:
				processSetBatSettings(rx_message);
				break;
			case SET_VOLATILE_BAT:
				processVolatileBat();
				break;
			case COMMAND:
				processCommand();
				break;
			default:
				break;
			}
#ifdef QT
			help->trigger_messageHandled(msgType);
#endif
			return 1;
		} else {
			//sendPackage(&currentMessageNumber_received, NACK);
		}
		break;
	default:
		break;
	}
	return 0;
}

void ExtControl::processVolatileBat() {
	memcpy(&volatileBattery, rx_message, sizeof(ProgramData::Battery));
}

void ExtControl::processCommand() {
	ExtControl::commandType command;
	memcpy(&command, rx_message, sizeof(ExtControl::commandType));
	currentCommand = (command_type)command.command;
	commandData = command.data;
	switch (command.command) {
	case ExtControl::CMD_IDLE:
		break;
	case ExtControl::CMD_STOP:
		break;
	case ExtControl::CMD_SETUP:
		break;
	case ExtControl::CMD_START:
		break;
	default:
		break;
	}
}
#ifndef QT
void ExtControl::setState(ExtControl::current_state_type state) {
	if(currentState == STATE_NOT_CONTROLING && state != STATE_BEGIN)
		return;
	if (state != currentState) {
		if(!sendPackage((uint8_t *) &state, ExtControl::STATE_CHANGED)) {
			pushFifo(state, &statesBacklog);
		}
	}
	ExtControl::current_state_type oldState = state;
	switch (state) {
	case ExtControl::STATE_BEGIN:
		Serial::beginRx(settings.getUARTspeed());
		Serial::begin(settings.getUARTspeed());
		currentState = STATE_IDLE;
		break;
	case ExtControl::STATE_STOPPED:
		if (currentCommand == ExtControl::CMD_STOP) {
			currentState = ExtControl::STATE_STOPPED;
			currentCommand = ExtControl::CMD_IDLE;
		}
		break;
	case ExtControl::STATE_IDLE:
		currentState = ExtControl::STATE_IDLE;
		break;
	case ExtControl::STATE_NOT_CONTROLING:
		if(oldState != STATE_NOT_CONTROLING) {
			Serial::endRx();
			Serial::end();
		}
		currentState = ExtControl::STATE_NOT_CONTROLING;
		currentCommand = CMD_IDLE;
		break;
	case ExtControl::STATE_COMPLETED:
		currentState = ExtControl::STATE_COMPLETED;
		break;
	case ExtControl::STATE_SETUP_COMPLETED:
		currentState = ExtControl::STATE_SETUP_COMPLETED;
		currentCommand = ExtControl::CMD_IDLE;
		break;
	case ExtControl::STATE_RUNNING:
		currentState = ExtControl::STATE_RUNNING;
		break;
	default:
		break;
	}
	if (oldState != currentState) {
		if(!sendPackage((uint8_t *) &currentState, ExtControl::STATE_CHANGED)) {
			pushFifo(currentState, &statesBacklog);
		}
	}
}

void ExtControl::setError(ExtControl::error_type error) {
	if (current_error != error) {
		if(!sendPackage((uint8_t *) &error, ExtControl::ERROR_OCORRED)) {
			pushFifo(error, &errorsBacklog);
		}
	}
	current_error = error;
}
#endif
void ExtControl::processGetOptions() {
	sendPackage((uint8_t*) &settings, PUT_OPTIONS);
}

void ExtControl::processSetOptions() {
	memcpy(&settings, rx_message, sizeof(Settings));
	Settings::save();
}
void ExtControl::processGetBatSettings(uint8_t index) {
	static uint8_t sendAllIndex = 0;
	batteryInfo msg;
	if (index == ALL_BAT_SETTINGS) {
		if (!isSendingAllBattery) {
			isSendingAllBattery = true;
			sendAllIndex = 0;
		} else
			++sendAllIndex;
		msg.index = sendAllIndex;
		if (sendAllIndex == MAX_PROGRAMS) {
			isSendingAllBattery = false;
			return;
		}
		eeprom::read(msg.battery, &eeprom::data.battery[sendAllIndex]);
	} else {
		msg.index = index;
		eeprom::read(msg.battery, &eeprom::data.battery[index]);
	}
	sendPackage((uint8_t*) &msg, PUT_BAT_SETTINGS);
}

void ExtControl::processSetBatSettings(uint8_t *buffer) {
	ExtControl::batteryInfo info;
	memcpy(&info, buffer, sizeof(ExtControl::batteryInfo));
	eeprom::write(&eeprom::data.battery[info.index], info.battery);
	eeprom::restoreProgramDataCRC();
}
bool ExtControl::sendPackage(uint8_t *buffer, message_type type,
		bool isResend, bool requireACK) {
#ifdef QT
	help->trigger_messageSent(type, buffer, isResend);
#endif
	static uint8_t resendCounter = 0;
#ifdef EXTCONTROL_DEBUG
	if (!currentSentAcked && !isResend && (type != DEBUG_MSG))
		return false;
#else
	if (!currentSentAcked && !isResend)
		return false;
#endif
	if (isResend) {
		++resendCounter;
		if (resendCounter > MAX_RESENDS) {
			currentSentAcked = 1;
			return false;
		}
		--currentMessageNumber_sent;
	} else
		resendCounter = 0;
	if (type == ACK || !requireACK)
		currentSentAcked = 1;
	else
		currentSentAcked = 0;
	currentTypeSent = type;
	currentSentTime = Time::getMilisecondsU16();
	if (!isResend)
		memcpy(tx_message, buffer, messageSizes[type]);
	uint16_t crc = 0xffff;
	Serial::write(MAGIC1);
	Serial::write(MAGIC2);
	Serial::write(currentMessageNumber_sent);
	crc = crc16_update(crc, currentMessageNumber_sent);
	++currentMessageNumber_sent;
	Serial::write(type);
	crc = crc16_update(crc, type);
	for (uint8_t i = 0; i < messageSizes[type]; ++i) {
		crc = crc16_update(crc, buffer[i]);
		Serial::write(buffer[i]);
	}
	Serial::write(crc >> 8);
	Serial::write(crc & 0xff);
	return true;
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

uint8_t ExtControl::getCommand() {
	return currentCommand;
}

ExtControl::commandDataType ExtControl::getCommandData() {
	return commandData;
}

ProgramData::Battery * ExtControl::getVolatileBattery() {
	return &volatileBattery;
}

bool ExtControl::externalControlRequested() {
	return currentCommand != CMD_IDLE;
}
bool ExtControl::getConnected() {
	return currentState != STATE_NOT_CONTROLING;
} //TODO
#ifdef EXTCONTROL_DEBUG
#include "string.h"
namespace ExtControl {
	char debug[10];
}
	void ExtControl::sendDebug(char *val) {
		strcpy(debug, val);
		sendPackage((uint8_t*)&debug, DEBUG_MSG);
	}
#endif

#ifndef QT

void ExtControl::parseErrorFromString(const char *s) {
	if(s == Monitor::string_batteryDisconnected)
		setError(ERROR_MON_BATT_DISC);
	else if(s == Monitor::string_internalTemperatureToHigh)
		setError(ERROR_MON_INT_TEMP_CUTOFF);
	else if(s == Monitor::string_balancePortDisconnected)
			setError(ERROR_MON_BALANCER_DISC);
	else if(s == Monitor::string_outputCurrentToHigh)
			setError(ERROR_MON_HW_FAILURE);
	else if(s == Monitor::string_inputVoltageToLow)
			setError(ERROR_MON_V_TOO_LOW);
	else if(s == Monitor::string_capacityLimit)
			setError(ERROR_MON_CAPACITY_CUTOFF);
	else if(s == Monitor::string_timeLimit)
			setError(ERROR_MON_TIME_LIMIT);
	else if(s == Monitor::string_externalTemperatureCutOff)
			setError(ERROR_MON_EXT_TEMP_CUTOFF);
}

void ExtControl::sendRealValues() {
	realInputs i;
	i.Idischarge = AnalogInputs::getRealValue(AnalogInputs::Idischarge);
	i.IdischargeSet = AnalogInputs::getRealValue(AnalogInputs::IdischargeSet);
	i.Ismps = AnalogInputs::getRealValue(AnalogInputs::Ismps);
	i.IsmpsSet = AnalogInputs::getRealValue(AnalogInputs::IsmpsSet);
	i.Textern = AnalogInputs::getRealValue(AnalogInputs::Textern);
	i.Tintern = AnalogInputs::getRealValue(AnalogInputs::Tintern);
	i.Vb0_pin = AnalogInputs::getRealValue(AnalogInputs::Vb0_pin);
	i.Vb1_pin = AnalogInputs::getRealValue(AnalogInputs::Vb1_pin);
	i.Vb2_pin = AnalogInputs::getRealValue(AnalogInputs::Vb2_pin);
	i.Vb3_pin = AnalogInputs::getRealValue(AnalogInputs::Vb3_pin);
	i.Vb4_pin = AnalogInputs::getRealValue(AnalogInputs::Vb4_pin);
	i.Vb5_pin = AnalogInputs::getRealValue(AnalogInputs::Vb5_pin);
	i.Vb6_pin = AnalogInputs::getRealValue(AnalogInputs::Vb6_pin);
	i.Vin = AnalogInputs::getRealValue(AnalogInputs::Vin);
	i.VoutMux = AnalogInputs::getRealValue(AnalogInputs::VoutMux);
	i.Vout_minus_pin = AnalogInputs::getRealValue(AnalogInputs::Vout_minus_pin);
	i.Vout_plus_pin = AnalogInputs::getRealValue(AnalogInputs::Vout_plus_pin);
	sendPackage((uint8_t*)&i, PUT_REAL_INPUTS, false, false);
}

void ExtControl::sendVirtualValues() {
	virtualInputs i;
	i.Cout = AnalogInputs::getRealValue(AnalogInputs::Cout);
	i.Eout = AnalogInputs::getRealValue(AnalogInputs::Eout);
	i.Iout = AnalogInputs::getRealValue(AnalogInputs::Iout);
	i.Pout = AnalogInputs::getRealValue(AnalogInputs::Pout);
	i.Vb1 = AnalogInputs::getRealValue(AnalogInputs::Vb1);
	i.Vb2 = AnalogInputs::getRealValue(AnalogInputs::Vb2);
	i.Vb3 = AnalogInputs::getRealValue(AnalogInputs::Vb3);
	i.Vb4 = AnalogInputs::getRealValue(AnalogInputs::Vb4);
	i.Vb5 = AnalogInputs::getRealValue(AnalogInputs::Vb5);
	i.Vb6 = AnalogInputs::getRealValue(AnalogInputs::Vb6);
	i.VbalanceInfo = AnalogInputs::getRealValue(AnalogInputs::VbalanceInfo);
	i.Vbalancer = AnalogInputs::getRealValue(AnalogInputs::Vbalancer);
	i.VobInfo = AnalogInputs::getRealValue(AnalogInputs::VobInfo);
	i.Vout = AnalogInputs::getRealValue(AnalogInputs::Vout);
	i.VoutBalancer = AnalogInputs::getRealValue(AnalogInputs::VoutBalancer);
	i.deltaLastCount = AnalogInputs::getRealValue(AnalogInputs::deltaLastCount);
	i.deltaTextern = AnalogInputs::getRealValue(AnalogInputs::deltaTextern);
	i.deltaVout = AnalogInputs::getRealValue(AnalogInputs::deltaVout);
	i.deltaVoutMax = AnalogInputs::getRealValue(AnalogInputs::deltaVoutMax);
	sendPackage((uint8_t*)&i, PUT_VIRTUAL_INPUTS, false, false);
}

void ExtControl::sendExtraValues() {
	extraValues i;
    i.RthCell1 = TheveninMethod::getReadableRthCell(1);
    i.RthCell1 = TheveninMethod::getReadableRthCell(2);
    i.RthCell1 = TheveninMethod::getReadableRthCell(3);
    i.RthCell1 = TheveninMethod::getReadableRthCell(4);
    i.RthCell1 = TheveninMethod::getReadableRthCell(5);
    i.RthCell1 = TheveninMethod::getReadableRthCell(6);
    i.BattRth = TheveninMethod::getReadableBattRth();
    i.WiresRth = TheveninMethod::getReadableWiresRth();
    i.percentCompleted = Monitor::getChargeProcent();
    i.ETATime = Monitor::getETATime();
    i.Balance = Balancer::balance;
    i.PID = hardware::getPIDValue();
    i.NeverUsedStackSize = StackInfo::getNeverUsedStackSize();
    i.FreeStack = StackInfo::getFreeStackSize();
	sendPackage((uint8_t*)&i, PUT_EXTRA_VALUES, false, false);
}
void ExtControl::setCurrentProgram(Program::ProgramType prog) {
	if(currentProgram != prog) {
		if(!sendPackage((uint8_t*)&prog, PROGRAM_CHANGED)) {
			pushFifo(prog, &programsBacklog);
		}
	}
}

void ExtControl::pushFifo(uint8_t val, fifo *f) {
	  if(f->elements == MAX_BACKLOG) {
	    return;
	  }
	  else {
	    f->elements++;
	    if(f->elements > 1) {
	      f->tail++;
	      f->tail %= MAX_BACKLOG;
	    }
	    //Store data into array
	    f->buffer[f->tail] = val;
	  }
}

int8_t ExtControl::popFifo(fifo *f) {
	 if(f->elements == 0) {
	    return -1;
	  }
	  else {
	    f->elements--;
	    uint8_t data = f->buffer[f->head];
	    if(f->elements >= 1) {
	      f->head++;
	      f->head %= MAX_BACKLOG;
	    }
	    return data;
	  }
}

#endif
#endif
