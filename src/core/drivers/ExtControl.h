#ifndef EXTCONTROL_H_
#define EXTCONTROL_H_

#include <inttypes.h>
#include <stddef.h>
#include <avr/io.h>
#include "ProgramData.h"

#define MAGIC1 0xFE
#define MAGIC2 0xEF
#define LOAD_VOLATILE_BATTERY 126
#ifdef ENABLE_SERIAL_CONTROL
class ExtControl
{
public:
	enum command_type {CMD_START, CMD_SETUP, CMD_STOP, CMD_IDLE, LAST_COMMAND_TYPE};
	enum current_state_type {STATE_NOT_CONTROLING, STATE_IDLE, STATE_STOPPED, STATE_ERROR, STATE_RUNNING, STATE_COMPLETED, STATE_SETUP_COMPLETED, LAST_STATE_TYPE};
	ExtControl();
    void powerOn();
    void doIdle();
    void powerOff();
    uint8_t getCommand() { return currentCommand;}
    uint8_t getCommandData() { return 0;}
    uint8_t externalControlRequested() {return false;}
    ProgramData::Battery * getVolatileBattery(){return &volatileBattery;}
    void stopped() {currentState = STATE_STOPPED;}
    void setState(current_state_type state) {currentState = state;}
    bool getConnected() {return isUnderControl;} //TODO
private:
	uint8_t isUnderControl;
	enum message_type {ACK, NACK, GET_OPTIONS, GET_BAT_SETTINGS, SET_VOLATILE_BAT,
						PUT_OPTIONS, PUT_BAT_SETTINGS, COMMAND, LAST_MESSAGE_TYPE};
	enum packet_process_status {WAITING_MAGIC1, WAITING_MAGIC2, WAITING_NUMBER, WAITING_TYPE,
								WAITING_DATA, WAITING_CRC1, WAITING_CRC2, LAST_PROCESS_STATUS};
	packet_process_status currentProcessStatus;
	current_state_type currentState;
	uint16_t crc16_update(uint16_t crc, uint8_t a);
	uint8_t messageSizes[LAST_MESSAGE_TYPE];
	command_type currentCommand;
	ProgramData::Battery volatileBattery;
	struct batteryInfo {
		uint8_t index;
		ProgramData::Battery battery;
	};
	struct largestSize {
		union {
		batteryInfo b;
		Settings s;
		};
	};
	uint8_t rx_message[sizeof(largestSize)];
	uint8_t tx_message[sizeof(largestSize)];
	uint8_t currentMessageNumber_sent;
	uint8_t currentMessageNumber_received;
	uint8_t currentTypeSent;
	uint8_t currentSentAcked;
	uint16_t currentSentTime;
	void processGetOptions();
	void processGetBatSettings(uint8_t index);
	void sendPackage(uint8_t *buffer, message_type type);
	void processVolatileBat();
    uint8_t processInputByte(int c);

//magic1
//magic2
//number
//type
//data
//crc1
//crc2
};
extern ExtControl extControl;
#endif
#endif /* EXTCONTROL_H_ */
