#ifndef EXTCONTROL_H_
#define EXTCONTROL_H_

#include <inttypes.h>
#ifndef QT
#include <avr/io.h>
#include "ProgramData.h"
#include "Program.h"
#include "Settings.h"
#else
#include <QObject>
#include "serial.h"
#include "helper_and_stubs.h"
#endif
//#define EXTCONTROL_DEBUG
#define MAGIC1 0xFE
#define MAGIC2 0xEF
#define ALL_BAT_SETTINGS 255
#ifdef ENABLE_EXTERNAL_CONTROL
#define EXTCONTROL_DEBUG
namespace ExtControl {
#ifdef EXTCONTROL_DEBUG
extern char debug[];
extern void sendDebug(char *);
#endif

struct batteryInfo {
	uint8_t index;
	ProgramData::Battery battery;
};

struct extraValues {
	uint16_t RthCell1;
	uint16_t RthCell2;
	uint16_t RthCell3;
	uint16_t RthCell4;
	uint16_t RthCell5;
	uint16_t RthCell6;
	uint16_t BattRth;
	uint16_t WiresRth;
	uint8_t percentCompleted;
	uint32_t ETATime;
	uint16_t FreeStack;
	uint16_t NeverUsedStackSize;
	uint16_t PID;
	uint16_t Balance;
};
struct realInputs {
	uint16_t Vout_plus_pin;
	uint16_t Vout_minus_pin;
	uint16_t Ismps;
	uint16_t Idischarge;
	uint16_t VoutMux;
	uint16_t Tintern;
	uint16_t Vin;
	uint16_t Textern;
	uint16_t Vb0_pin;
	uint16_t Vb1_pin;
	uint16_t Vb2_pin;
	uint16_t Vb3_pin;
	uint16_t Vb4_pin;
	uint16_t Vb5_pin;
	uint16_t Vb6_pin;
	uint16_t IsmpsSet;
	uint16_t IdischargeSet;
	uint8_t  strategy;
};

struct virtualInputs {
	uint16_t Vout;
	uint16_t Vbalancer;
	uint16_t VoutBalancer;
	uint16_t VobInfo;
	uint16_t VbalanceInfo;
	uint16_t Iout;
	uint16_t Pout;
	uint16_t Cout;
	uint16_t Eout;
	uint16_t deltaVout;
	uint16_t deltaVoutMax;
	uint16_t deltaTextern;
	uint16_t deltaLastCount;
	uint16_t Vb1;
	uint16_t Vb2;
	uint16_t Vb3;
	uint16_t Vb4;
	uint16_t Vb5;
	uint16_t Vb6;
};

struct largestSize {
	union {
		batteryInfo b;
		Settings s;
		virtualInputs v;
		realInputs r;
	};
};

enum command_type {
	CMD_START, CMD_SETUP, CMD_STOP, CMD_IDLE, LAST_COMMAND_TYPE
};
enum strategy_type { STR_NONE, STR_CHARGE, STR_DISCHARGE, STR_E, STR_BALANCE, STR_DELAY, LAST_STRATEGY_TYPE
};

enum current_state_type {
	STATE_NOT_CONTROLING,
	STATE_BEGIN,
	STATE_IDLE,
	STATE_STOPPED,
	STATE_ERROR,
	STATE_RUNNING,
	STATE_COMPLETED,
	STATE_SETUP_COMPLETED,
	LAST_STATE_TYPE
};
enum error_type {
	ERROR_NONE = 0,
	ERROR_BALANCE = 1,
	ERROR_BATTERY = 2,
	ERROR_STRATEGY,
	ERROR_MON_BATT_DISC,
	ERROR_MON_INT_TEMP_CUTOFF,
	ERROR_MON_BALANCER_DISC,
	ERROR_MON_HW_FAILURE,
	ERROR_MON_V_TOO_LOW,
	ERROR_MON_CAPACITY_CUTOFF,
	ERROR_MON_TIME_LIMIT,
	ERROR_MON_EXT_TEMP_CUTOFF,
	ERROR_WRONG_SESSION_ID_RECEIVED,
	LAST_ERROR_TYPE

};

extern void parseErrorFromString(const char*);
//TODO
enum message_type {
	ACK,
	NACK,
	GET_OPTIONS,
	GET_BAT_SETTINGS,
	SET_BAT_SETTINGS,
	SET_VOLATILE_BAT,
	PUT_OPTIONS,
	SET_OPTIONS,
	PUT_BAT_SETTINGS,
	COMMAND,
	STATE_CHANGED,
	ERROR_OCORRED,
	PUT_REAL_INPUTS,
	PUT_VIRTUAL_INPUTS,
	PUT_EXTRA_VALUES,
	PROGRAM_CHANGED,
#ifdef EXTCONTROL_DEBUG
	DEBUG_MSG,
#endif
	LAST_MESSAGE_TYPE
};

struct commandDataType {
	uint8_t data1;
	uint8_t data2;
}__attribute__((packed));

struct commandType {
	uint8_t command;
	commandDataType data;
}__attribute__((packed));

extern command_type currentCommand;
extern commandDataType commandData;
extern ProgramData::Battery volatileBattery;
extern bool isSendingAllBattery;
extern error_type current_error;

extern current_state_type currentState;


#ifdef QT
extern helper *help;
void setSessionID(uint8_t sessionID);
#endif
void setError(error_type error);
void init();
void doIdle();
uint8_t getCommand();
commandDataType getCommandData();
ProgramData::Battery * getVolatileBattery();
void setState(current_state_type state);
void setCurrentProgram(Program::ProgramType prog);
bool getConnected(); //TODO
uint8_t processInputByte(int c);
struct sendPackageResult {
	bool sent;
	uint8_t number;
};
sendPackageResult sendPackage(uint8_t *buffer, message_type type, bool isResend = false, bool requireACK = true);

bool externalControlRequested();
enum packet_process_status {
	WAITING_MAGIC1,
	WAITING_MAGIC2,
	WAITING_NUMBER,
	WAITING_SESSION_ID,
	WAITING_TYPE,
	WAITING_DATA,
	WAITING_CRC1,
	WAITING_CRC2,
	LAST_PROCESS_STATUS
};

uint16_t crc16_update(uint16_t crc, uint8_t a);

void processGetOptions();
void processSetOptions();
void processGetBatSettings(uint8_t index);
void processSetBatSettings(uint8_t *buffer);
void processVolatileBat();
void processCommand(uint8_t session_id);
//magic1
//magic2
//number
//type
//data
//crc1
//crc2
}
#endif
#endif /* EXTCONTROL_H_ */
