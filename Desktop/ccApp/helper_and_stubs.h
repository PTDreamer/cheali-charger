#ifndef HELPER_AND_STUBS_H
#define HELPER_AND_STUBS_H

#include <inttypes.h>
#include <stddef.h>
#include <QDebug>
#include "ProgramData.h"
#include <QSerialPort>
#include <QElapsedTimer>
#include <QObject>

namespace Program {

	enum ProgramType {
		Charge, ChargeBalance, Balance, Discharge, FastCharge,
		Storage, StorageBalance, DischargeChargeCycle, CapacityCheck,
		EditBattery,
		Calibrate,
		LAST_PROGRAM_TYPE};

	enum ProgramState {
		Done, InProgress, Error, Info
	};
}
namespace eeprom {
	void read(ProgramData::Battery &bat,  ProgramData::Battery *batt);
	void write(ProgramData::Battery *batt, ProgramData::Battery &bat);
	void restoreProgramDataCRC();
	 extern struct dataType{
		 ProgramData::Battery battery[MAX_PROGRAMS]{};
	 }data;
}

namespace SerialLog {
	void printInt(int val);
	void printUInt(uint val);
	void printString(char *str);
}

namespace Time {
	extern QElapsedTimer timer;
	void start();
	uint16_t diffU16(uint16_t start, uint16_t end);
	uint32_t getMiliseconds();
	uint16_t getMilisecondsU16();

}
class helper: public QObject {
	Q_OBJECT
public:
	helper(QObject *parent);
	void trigger_messageReceived(int, uint8_t *buffer, uint8_t sessionID);
	void trigger_messageSent(int val, uint8_t *buffer, bool isResend);
	void trigger_messageHandled(int val);
signals:
	void messageReceived(int, uint8_t *buffer, uint8_t sessionID);
	void messageSent(int, uint8_t *buffer, bool);
	void messageHandled(int);
};

#endif // HELPER_AND_STUBS_H
