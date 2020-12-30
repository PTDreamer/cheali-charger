#include "helper_and_stubs.h"


QElapsedTimer Time::timer;

void eeprom::read(ProgramData::Battery &bat, ProgramData::Battery *batt){
		memcpy(&bat, batt, sizeof (ProgramData::Battery));
	};

void eeprom::write(ProgramData::Battery *batt, ProgramData::Battery &bat){
		memcpy(batt, &bat, sizeof (ProgramData::Battery));
	};
	void SerialLog::printInt(int val){qDebug() << val;}
	void SerialLog::printUInt(uint val){qDebug() << val;}
	void SerialLog::printString(char *str){qDebug()<< str;}

	uint16_t Time::diffU16(uint16_t start, uint16_t end) {
		return end - start;
	}

	uint32_t Time::getMiliseconds() {return timer.nsecsElapsed()/1000000;}
	uint16_t Time::getMilisecondsU16() {return Time::getMiliseconds();}

	void Time::start() {
		Time::timer.start();
	}

	Settings settings;

	void eeprom::restoreProgramDataCRC()
	{

	}

	void Settings::save() {

	}

	helper::helper(QObject *parent):QObject(parent)
	{

	}

	void helper::trigger_messageReceived(int val, uint8_t *buffer, uint8_t sessionID)
	{
		emit messageReceived(val, buffer, sessionID);
	}

	void helper::trigger_messageSent(int val, uint8_t *buffer, bool isResend)
	{
		emit messageSent(val, buffer, isResend);
	}

	void helper::trigger_messageHandled(int val)
	{
		emit messageHandled(val);
	}


