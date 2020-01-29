#include "serial.h"

QSerialPort *Serial::port = nullptr;

int Serial::read() {
	if(!Serial::port->bytesAvailable())
		return -1;
	int data;
	char d;
	Serial::port->read(&d, 1);
	data = (uint8_t)d;
	return data;
}
void Serial::write(uint8_t val) {
	Serial::port->write((char*)&val, 1);
};

void Serial::setPort(QSerialPort *p) {
	Serial::port = p;
}

void Serial::endRx()
{

}
