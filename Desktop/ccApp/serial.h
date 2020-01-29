#ifndef SERIAL_H
#define SERIAL_H

#include <QDebug>
#include <QSerialPort>

namespace Serial
{
	extern QSerialPort *port;
	int read();
	void write(uint8_t val);
	void setPort(QSerialPort *p);
	void end();
	void endRx();
}

#endif // SERIAL_H
