#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQueue>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QtNetwork>
#include <QUrl>
#include "httpserver.h"
MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), keepalive(nullptr), currentSettings(nullptr)
{
	ui->setupUi(this);
	typeToStr.insert(ExtControl::ACK, "ACK");
	typeToStr.insert(ExtControl::NACK, "NACK");
	typeToStr.insert(ExtControl::GET_OPTIONS, "GET_OPTIONS");
	typeToStr.insert(ExtControl::GET_BAT_SETTINGS, "GET_BAT_SETTINGS");
	typeToStr.insert(ExtControl::SET_BAT_SETTINGS, "SET_BAT_SETTINGS");
	typeToStr.insert(ExtControl::SET_VOLATILE_BAT, "SET_VOLATILE_BAT");
	typeToStr.insert(ExtControl::PUT_OPTIONS, "PUT_OPTIONS");
	typeToStr.insert(ExtControl::SET_OPTIONS, "SET_OPTIONS");
	typeToStr.insert(ExtControl::PUT_BAT_SETTINGS, "PUT_BAT_SETTINGS");
	typeToStr.insert(ExtControl::COMMAND, "COMMAND");
	typeToStr.insert(ExtControl::STATE_CHANGED, "STATE_CHANGED");
	typeToStr.insert(ExtControl::ERROR_OCORRED, "ERROR_OCORRED");
	typeToStr.insert(ExtControl::PUT_REAL_INPUTS, "PUT_REAL_INPUTS");
	typeToStr.insert(ExtControl::PUT_VIRTUAL_INPUTS, "PUT_VIRTUAL_INPUTS");
	typeToStr.insert(ExtControl::PUT_EXTRA_VALUES, "PUT_EXTRA_VALUES");
	typeToStr.insert(ExtControl::PROGRAM_CHANGED, "PROGRAM_CHANGED");
	typeToStr.insert(ExtControl::DEBUG_MSG, "DEBUG_MSG");
	typeToStr.insert(ExtControl::LAST_MESSAGE_TYPE, "LAST_MESSAGE_TYPE");

	states.insert(ExtControl::STATE_BEGIN, "STATE_BEGIN");
	states.insert(ExtControl::STATE_IDLE, "STATE_IDLE");
	states.insert(ExtControl::STATE_ERROR, "STATE_ERROR");
	states.insert(ExtControl::STATE_RUNNING, "STATE_RUNNING");
	states.insert(ExtControl::STATE_STOPPED, "STATE_STOPPED");
	states.insert(ExtControl::STATE_COMPLETED, "STATE_COMPLETED");
	states.insert(ExtControl::STATE_NOT_CONTROLING, "STATE_NOT_CONTROLING");
	states.insert(ExtControl::STATE_SETUP_COMPLETED, "STATE_SETUP_COMPLETED");

	errors.insert(ExtControl::ERROR_NONE, "ERROR_NONE");
	errors.insert(ExtControl::ERROR_BATTERY, "ERROR_BATTERY");
	errors.insert(ExtControl::ERROR_BALANCE, "ERROR_BALANCE");
	errors.insert(ExtControl::ERROR_STRATEGY, "ERROR_STRATEGY");
	errors.insert(ExtControl::LAST_ERROR_TYPE, "LAST_ERROR_TYPE");
	/*
	STRING(batteryDisconnected,         "battery disc.");
	STRING(internalTemperatureToHigh,   "int.temp.cutoff");
	STRING(balancePortDisconnected,     "balancer disc.");
	STRING(outputCurrentToHigh,         "HW failure");

	STRING(inputVoltageToLow,           "input V to low");
	STRING(capacityLimit,               "capacity cutoff");
	STRING(timeLimit,                   "time limit");
	STRING(externalTemperatureCutOff,   "ext.temp.cutoff");*/
	errors.insert(ExtControl::ERROR_MON_BATT_DISC, "battery disconected");
	errors.insert(ExtControl::ERROR_MON_INT_TEMP_CUTOFF, "int temperature cutoff");
	errors.insert(ExtControl::ERROR_MON_BALANCER_DISC, "balancer disconnected");
	errors.insert(ExtControl::ERROR_MON_HW_FAILURE, "output current too high");
	errors.insert(ExtControl::ERROR_MON_V_TOO_LOW, "input voltage too low");
	errors.insert(ExtControl::ERROR_MON_CAPACITY_CUTOFF, "capacity limit");
	errors.insert(ExtControl::ERROR_MON_TIME_LIMIT, "time limit");
	errors.insert(ExtControl::ERROR_MON_EXT_TEMP_CUTOFF, "ext temp cutoff");

	programs.insert(Program::Charge, "Charge");
	programs.insert(Program::ChargeBalance, "ChargeBalance");
	programs.insert(Program::Balance, "Balance");
	programs.insert(Program::Discharge, "Discharge");
	programs.insert(Program::FastCharge, "FastCharge");
	programs.insert(Program::Storage, "Storage");
	programs.insert(Program::StorageBalance, "StorageBalance");
	programs.insert(Program::DischargeChargeCycle, "DischargeChargeCycle");
	programs.insert(Program::CapacityCheck, "CapacityCheck");
	programs.insert(Program::EditBattery, "EditBattery");
	programs.insert(Program::Calibrate, "Calibrate");

	setState(ExtControl::STATE_IDLE);
	setError(ExtControl::ERROR_NONE);
	setProgram(Program::Charge);
	const auto infos = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo &info : infos)
		ui->portInfo->addItem(info.portName());
	foreach (qint32 val, QSerialPortInfo::standardBaudRates()) {
		ui->portSpeeds->addItem(QString::number(val));
	}
	port = new QSerialPort(this);
	Serial::setPort(port);
	Time::start();
	helper *help = new helper(this);
	ExtControl::help = help;
	idleTimer.setSingleShot(true);
	idleTimer.start(0);
	QTimer *portsUpdateTimer = new QTimer(this);
	portsUpdateTimer->start(1000);
	connect(portsUpdateTimer, &QTimer::timeout, this, &MainWindow::updatePorts);
	//	connect(port, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
	connect(port, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
	connect(&idleTimer, &QTimer::timeout, this, &MainWindow::idleSlot, Qt::QueuedConnection);
	connect(help, &helper::messageReceived, this, &MainWindow::onMessageReceived);
	connect(help, &helper::messageSent, this, &MainWindow::onMessageSent);
	for(int x = 0; x < MAX_PROGRAMS; ++x) {
		batteries.insert(x, &eeprom::data.battery[x]);
		ui->batteries->addItem(QString::number(x));
	}
	ui->batteries->addItem("Volatile");
	for(int x = 0; x < ui->batteries->count(); ++x) {
		ui->setupBatData1->addItem(ui->batteries->itemText(x));
	}
	batteries.insert(MAX_PROGRAMS, &ExtControl::volatileBattery);
	ui->portInfo->setCurrentText(m_settings.value("port").toString());
	ui->portSpeeds->setCurrentText(m_settings.value("portSpeed").toString());
	qDebug() << "LARGEST:"<<sizeof (ExtControl::largestSize);
	ExtControl::currentState = ExtControl::STATE_IDLE;
	settings.UART = Settings::ExtControl;
	qnam.setCookieJar(new QNetworkCookieJar(this));
	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenTriggered);
	loadServerDefs();
	httpServer *s = new httpServer(this);
	s->start(8081);
}

MainWindow::~MainWindow()
{
	port->close();
	m_settings.setValue("port", ui->portInfo->currentText());
	m_settings.setValue("portSpeed", ui->portSpeeds->currentText());
	delete ui;
}

void MainWindow::on_pbConnect_clicked()
{
	if(!port->isOpen()) {
		port->setPortName(ui->portInfo->currentText());
		port->setBaudRate(ui->portSpeeds->currentText().toInt());
		if (!port->open(QIODevice::ReadWrite)) {
			ui->output->append("COULD NOT OPEN PORT");
		} else {
			ui->pbConnect->setText("Disconnect");
		}
	} else {
		port->close();
		ui->pbConnect->setText("Connect");
	}
}

void MainWindow::handleReadyRead()
{
	//	char c;
	//	QByteArray a = port->readAll();
	//	ui->output->append(a);
	//	foreach(char c, a) {
	//		ExtControl::processInputByte(c);
	//	}
}

void MainWindow::handleError()
{
	if(port->error() != QSerialPort::NoError) {
		ui->output->append(QObject::tr("An I/O error occurred while reading "
									   "the data from port %1, error: %2")
						   .arg(port->portName())
						   .arg(port->errorString()));
		if(port->isOpen()) {
			port->close();
			ui->pbConnect->setText("Connect");
		}
	}
}

void MainWindow::idleSlot()
{
	if(ui->checkBox->isChecked())
		ExtControl::doIdle();
	idleTimer.start(100);
}

void MainWindow::onMessageReceived(int val, uint8_t *buffer, uint8_t sessionID)
{
	ui->current_session->setText(QString::number(sessionID));
	ui->output->append("Received:" + typeToStr.value(ExtControl::message_type(val)));
	switch (val) {
	case ExtControl::PUT_BAT_SETTINGS:
		handleBattery(buffer);
		break;
	case ExtControl::PUT_OPTIONS:
		loadSettings(reinterpret_cast<Settings*>(buffer));
		break;
	case ExtControl::STATE_CHANGED:
		ui->output->append("New State:" + QString::number(buffer[0]) + states.value(ExtControl::current_state_type(buffer[0])));
		setState(ExtControl::current_state_type(buffer[0]));
		break;
	case ExtControl::ERROR_OCORRED:
		ui->output->append("Error:" + QString::number(buffer[0]) + errors.value(ExtControl::error_type(buffer[0])));
		setError(ExtControl::error_type(buffer[0]));
		break;
	case ExtControl::PROGRAM_CHANGED:
		ui->output->append("Program changed:" + QString::number(buffer[0]) + programs.value(Program::ProgramType(buffer[0])));
		setProgram(Program::ProgramType(buffer[0]));
		break;
	case ExtControl::PUT_REAL_INPUTS:
		setRealValues(reinterpret_cast<ExtControl::realInputs*>(buffer));
		break;
	case ExtControl::PUT_VIRTUAL_INPUTS:
		setVirtualValues(reinterpret_cast<ExtControl::virtualInputs*>(buffer));
		break;
	case ExtControl::PUT_EXTRA_VALUES:
		setExtraValues(reinterpret_cast<ExtControl::extraValues*>(buffer));
		break;
	case ExtControl::DEBUG_MSG:
		ui->output->append("DEBUG:" + QString(reinterpret_cast<char*>(buffer)));
		break;
	default:
		break;
	}
}

void MainWindow::onMessageSent(int type, uint8_t *buffer, bool resend)
{
	Q_UNUSED(buffer)
	if(resend)
		ui->output->append("Resent:" + typeToStr.value(ExtControl::message_type(type)));
	else
		ui->output->append("Sent:" + typeToStr.value(ExtControl::message_type(type)));
}

void MainWindow::onMessageHandled(int val)
{
	switch (val) {
	case ExtControl::PUT_OPTIONS:
		break;
	default:
		break;
	}
}

void MainWindow::loadBattery(ProgramData::Battery *bat)
{
	ui->type->setCurrentIndex(bat->type);
	ui->capacity->setValue(bat->capacity);
	ui->cells->setValue(bat->cells);
	ui->ic->setValue(bat->Ic);
	ui->id->setValue(bat->Id);
	ui->vc_per_cell->setValue(bat->Vc_per_cell);
	ui->vd_per_cell->setValue(bat->Vd_per_cell);
	ui->min_ic->setValue(bat->minIc);
	ui->min_id->setValue(bat->minId);
	ui->time->setValue(bat->time);
	ui->enable_ext_temp->setChecked(bat->enable_externT);
	ui->ext_tco->setValue(bat->externTCO);
	ui->enable_adap_disch->setChecked(bat->enable_adaptiveDischarge);
	ui->dc_rest_time->setValue(bat->DCRestTime);
	ui->cap_cut_off->setValue(bat->capCutoff);
	ui->vs_per_cell->setValue(bat->Vs_per_cell);
	ui->balance_error->setValue(bat->balancerError);
	ui->enable_delta_v->setChecked(bat->enable_deltaV);
	ui->deltav->setValue(bat->deltaV);
	ui->deltav_ignoreT->setValue(bat->deltaVIgnoreTime);
	ui->deltat->setValue(bat->deltaT);
	ui->dc_cycles->setValue(bat->DCcycles);
}

void MainWindow::loadSettings(Settings *s)
{
	qDebug() << "received settings backlight:" << s->backlight;
	ui->backlight->setValue(s->backlight);
	ui->fanOn->setCurrentIndex(s->fanOn);
	ui->fanTempOn->setValue(s->fanTempOn);
	ui->dischargeTempOff->setValue(s->dischargeTempOff);
	ui->audioBeep->setChecked(s->audioBeep);
	ui->minIc->setValue(s->minIc);
	ui->maxIc->setValue(s->maxIc);
	ui->maxId->setValue(s->minId);
	ui->maxId->setValue(s->maxId);
	ui->maxPc->setValue(s->maxPc);
	ui->maxPd->setValue(s->maxPd);
	ui->inputVoltageLow->setValue(s->inputVoltageLow);
	ui->adcNoise->setChecked(s->adcNoise);
	ui->UART->setCurrentIndex(s->UART);
	ui->UARTspeed->setCurrentIndex(s->UARTspeed);
	ui->UARToutput->setCurrentIndex(s->UARToutput);
	ui->menuType->setCurrentIndex(s->menuType);
	ui->menuButtons->setCurrentIndex(s->menuButtons);
}

void MainWindow::setBattery(int index)
{
	ProgramData::Battery *bat = batteries[index];
	bat->type = uint16_t (ui->type->currentIndex());
	bat->capacity = uint16_t (ui->capacity->value());
	bat->cells = uint16_t (ui->cells->value());
	bat->Ic = uint16_t (ui->ic->value());
	bat->Id = uint16_t (ui->id->value());
	bat->Vc_per_cell = uint16_t (ui->vc_per_cell->value());
	bat->Vd_per_cell = uint16_t (ui->vd_per_cell->value());
	bat->minIc = uint16_t (ui->min_ic->value());
	bat->minId = uint16_t (ui->min_id->value());
	bat->time = uint16_t (ui->time->value());
	bat->enable_externT = uint16_t (ui->enable_ext_temp->isChecked() ? 1 : 0);
	bat->externTCO = uint16_t (ui->ext_tco->value());
	bat->enable_adaptiveDischarge = uint16_t (ui->enable_adap_disch->isChecked() ? 1: 0);
	bat->DCRestTime = uint16_t (ui->dc_rest_time->value());
	bat->capCutoff = uint16_t (ui->cap_cut_off->value());
	bat->Vs_per_cell = uint16_t (ui->vs_per_cell->value());
	bat->balancerError = uint16_t (ui->balance_error->value());
	bat->enable_deltaV = uint16_t (ui->enable_delta_v->isChecked() ? 1 : 0);
	bat->deltaV = int16_t (ui->deltav->value());
	bat->deltaVIgnoreTime = uint16_t (ui->deltav_ignoreT->value());
	bat->deltaT = uint16_t (ui->deltat->value());
	bat->DCcycles = uint16_t (ui->dc_cycles->value());
	qDebug() << "Set Battery:" << index << bat->type << batteries.value(index)->type << eeprom::data.battery[index].type;
}

void MainWindow::setSettings(Settings *s)
{
	setCurrentSettings(s);
	s->backlight = uint16_t (ui->backlight->value());
	s->fanOn = uint16_t (ui->fanOn->currentIndex());
	s->fanTempOn = uint16_t (ui->fanTempOn->value());
	s->dischargeTempOff = uint16_t (ui->dischargeTempOff->value());
	s->audioBeep = uint16_t (ui->audioBeep->isChecked());
	s->minIc = uint16_t (ui->minIc->value());
	s->maxIc = uint16_t (ui->maxIc->value());
	s->minId = uint16_t (ui->maxId->value());
	s->maxId = uint16_t (ui->maxId->value());
	s->maxPc = uint16_t (ui->maxPc->value());
	s->maxPd = uint16_t (ui->maxPd->value());
	s->inputVoltageLow = uint16_t (ui->inputVoltageLow->value());
	s->adcNoise = uint16_t (ui->adcNoise->isChecked());
	s->UART = uint16_t (ui->UART->currentIndex());
	s->UARTspeed = uint16_t (ui->UARTspeed->currentIndex());
	s->UARToutput = uint16_t (ui->UARToutput->currentIndex());
	s->menuType = uint16_t (ui->menuType->currentIndex());
	s->menuButtons = uint16_t (ui->menuButtons->currentIndex());
}

void MainWindow::setRealValues(ExtControl::realInputs *i)
{
	ui->Vout_plus_pin->setValue(i->Vout_plus_pin);
	ui->Vout_minus_pin->setValue(i->Vout_minus_pin);
	ui->Ismps->setValue(i->Ismps);
	ui->Idischarge->setValue(i->Idischarge);
	ui->VoutMux->setValue(i->VoutMux);
	ui->Tintern->setValue(i->Tintern);
	ui->Vin->setValue(i->Vin);
	ui->Textern->setValue(i->Textern);
	ui->Vb0_pin->setValue(i->Vb0_pin);
	ui->Vb1_pin->setValue(i->Vb1_pin);
	ui->Vb2_pin->setValue(i->Vb2_pin);
	ui->Vb3_pin->setValue(i->Vb3_pin);
	ui->Vb4_pin->setValue(i->Vb4_pin);
	ui->Vb5_pin->setValue(i->Vb5_pin);
	ui->Vb6_pin->setValue(i->Vb6_pin);
	ui->IsmpsSet->setValue(i->IsmpsSet);
	ui->IdischargeSet->setValue(i->IdischargeSet);
}

void MainWindow::setVirtualValues(ExtControl::virtualInputs *i)
{
	ui->Vout->setValue(i->Vout);
	ui->Vbalancer->setValue(i->Vbalancer);
	ui->VoutBalancer->setValue(i->VoutBalancer);
	ui->VobInfo->setValue(i->VobInfo);
	ui->VbalanceInfo->setValue(i->VbalanceInfo);
	ui->Iout->setValue(i->Iout);
	ui->Pout->setValue(i->Pout);
	ui->Cout->setValue(i->Cout);
	ui->Eout->setValue(i->Eout);
	ui->deltaVout->setValue(i->deltaVout);
	ui->deltaVoutMax->setValue(i->deltaVoutMax);
	ui->deltaTextern->setValue(i->deltaTextern);
	ui->deltaLastCount->setValue(i->deltaLastCount);
	ui->Vb1->setValue(i->Vb1);
	ui->Vb2->setValue(i->Vb2);
	ui->Vb3->setValue(i->Vb3);
	ui->Vb4->setValue(i->Vb4);
	ui->Vb5->setValue(i->Vb5);
	ui->Vb6->setValue(i->Vb6);
}

void MainWindow::setExtraValues(ExtControl::extraValues *i)
{
	ui->RthCell1->setValue(i->RthCell1);
	ui->RthCell2->setValue(i->RthCell2);
	ui->RthCell3->setValue(i->RthCell3);
	ui->RthCell4->setValue(i->RthCell4);
	ui->RthCell5->setValue(i->RthCell5);
	ui->RthCell6->setValue(i->RthCell6);
	ui->BattRth->setValue(i->BattRth);
	ui->WiresRth->setValue(i->WiresRth);
	ui->percentCompleted->setValue(i->percentCompleted);
	ui->ETATime->setValue(i->ETATime);
	ui->FreeStack->setValue(i->FreeStack);
	ui->NeverUsedStackSize->setValue(i->NeverUsedStackSize);
	ui->PID->setValue(i->PID);
	ui->Balance->setValue(i->Balance);
}

void MainWindow::handleBattery(uint8_t *buffer)
{
	ExtControl::batteryInfo info;
	memcpy(&info, buffer, sizeof (ExtControl::batteryInfo));
	qDebug() << "Put batttery:" << info.index << " Type:" << info.battery.type;
	if(!batteries.contains(info.index)) {
		batteries.insert(info.index, new ProgramData::Battery);
	}
	memcpy(batteries[info.index], &info.battery, sizeof (ProgramData::Battery));
	loadBattery(batteries.value(ui->batteries->currentIndex()));
}

void MainWindow::loadServerDefs()
{
	clientID = m_settings.value("clientID", 0).toString();
	clientName = m_settings.value("clientName", "").toString();
	serverPassword = m_settings.value("server_password", "").toString();
	serverUsername = m_settings.value("server_username", "").toString();
	serverAddress = m_settings.value("server_address", "").toString();
	connectToServer = m_settings.value("connect_to_server", false).toBool();
	if(connectToServer) {
		if(!keepalive)
			keepalive = new QTimer(this);
		connect(keepalive, &QTimer::timeout, this, &MainWindow::sendServerKeepAlive, Qt::UniqueConnection);
		keepalive->start(5000);
	}
	else {
		if(keepalive)
			keepalive->stop();
	}
}

Settings *MainWindow::getCurrentSettings() const
{
	return currentSettings;
}

void MainWindow::setCurrentSettings(Settings *value)
{
	currentSettings = value;
	emit settingsUpdated(value);
}

void MainWindow::setError(ExtControl::error_type error)
{
	static QTimer onTimer;
	static QTimer offTimer;
	static QQueue<ExtControl::error_type> fifo;
	static bool lastStateOn = false;
	ExtControl::error_type newError;
	onTimer.setInterval(2000);
	onTimer.setSingleShot(true);
	connect(&onTimer, SIGNAL(timeout()), this, SLOT(setError()), Qt::UniqueConnection);
	offTimer.setInterval(500);
	offTimer.setSingleShot(true);
	connect(&offTimer, SIGNAL(timeout()), this, SLOT(setError()), Qt::UniqueConnection);

	if((onTimer.isActive() || offTimer.isActive()) && error != ExtControl::LAST_ERROR_TYPE) {
		fifo.enqueue(error);
		return;
	}
	if(error == ExtControl::LAST_ERROR_TYPE) {
		if(fifo.isEmpty()) {
			ui->error_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			return;
		}
		else if(lastStateOn) {
			ui->error_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			offTimer.start();
			return;
		}
		else {
			newError = fifo.dequeue();
		}
	}
	else {
		newError = error;
	}
	lastStateOn = true;
	ui->error_w->setStyleSheet("background-color:red;");
	ui->error_lbl->setText(errors.value(newError));
	onTimer.start();
}

void MainWindow::setProgram(Program::ProgramType prog)
{
	static QTimer onTimer;
	static QTimer offTimer;
	static QQueue<Program::ProgramType> fifo;
	static bool lastStateOn = false;
	Program::ProgramType newProg;
	onTimer.setInterval(2000);
	onTimer.setSingleShot(true);
	connect(&onTimer, SIGNAL(timeout()), this, SLOT(setProgram()), Qt::UniqueConnection);
	offTimer.setInterval(500);
	offTimer.setSingleShot(true);
	connect(&offTimer, SIGNAL(timeout()), this, SLOT(setProgram()), Qt::UniqueConnection);

	if((onTimer.isActive() || offTimer.isActive()) && prog != Program::LAST_PROGRAM_TYPE) {
		fifo.enqueue(prog);
		return;
	}
	if(prog == Program::LAST_PROGRAM_TYPE) {
		if(fifo.isEmpty()) {
			ui->program_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			return;
		}
		else if(lastStateOn) {
			ui->program_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			offTimer.start();
			return;
		}
		else {
			newProg = fifo.dequeue();
		}
	}
	else {
		newProg = prog;
	}
	lastStateOn = true;
	ui->program_w->setStyleSheet("background-color:red;");
	ui->program_lbl->setText(programs.value(newProg));
	onTimer.start();
}

void MainWindow::setState(ExtControl::current_state_type state)
{
	static QTimer onTimer;
	static QTimer offTimer;
	static QQueue<ExtControl::current_state_type> fifo;
	static bool lastStateOn = false;
	ExtControl::current_state_type newState;
	onTimer.setInterval(2000);
	onTimer.setSingleShot(true);
	connect(&onTimer, SIGNAL(timeout()), this, SLOT(setState()), Qt::UniqueConnection);
	offTimer.setInterval(500);
	offTimer.setSingleShot(true);
	connect(&offTimer, SIGNAL(timeout()), this, SLOT(setState()), Qt::UniqueConnection);

	if((onTimer.isActive() || offTimer.isActive()) && state != ExtControl::LAST_STATE_TYPE) {
		fifo.enqueue(state);
		return;
	}
	if(state == ExtControl::LAST_STATE_TYPE) {
		if(fifo.isEmpty()) {
			ui->status_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			return;
		}
		else if(lastStateOn) {
			ui->status_w->setStyleSheet("background-color:none;");
			lastStateOn = false;
			offTimer.start();
			return;
		}
		else {
			newState = fifo.dequeue();
		}
	}
	else {
		newState = state;
	}
	lastStateOn = true;
	ui->status_w->setStyleSheet("background-color:red;");
	ui->status_lbl->setText(states.value(newState));
	onTimer.start();
}

void MainWindow::on_batteries_currentIndexChanged(int index)
{
	loadBattery(batteries.value(index));
}

void MainWindow::on_pushButton_5_clicked()
{
	setBattery(ui->batteries->currentIndex());
}

void MainWindow::on_pushButton_6_clicked()
{
	qDebug() << "send";
	ExtControl::sendPackage(nullptr, ExtControl::GET_OPTIONS);
}

void MainWindow::on_pushButton_4_clicked()
{
	uint8_t buffer = ALL_BAT_SETTINGS;
	ExtControl::sendPackage(&buffer, ExtControl::GET_BAT_SETTINGS);
}

void MainWindow::on_pushButton_7_clicked()//set Options
{
	setSettings(&settings);
	ExtControl::sendPackage(reinterpret_cast<uint8_t*>(&settings), ExtControl::SET_OPTIONS);
}

void MainWindow::on_pushButton_8_clicked()// send bat settings
{
	setBattery(ui->batteries->currentIndex());
	ExtControl::batteryInfo bat;
	bat.index = uint8_t (ui->batteries->currentIndex());
	bat.battery = *batteries.value(ui->batteries->currentIndex());
	if(ui->batteries->currentIndex() == MAX_PROGRAMS) {
		ExtControl::sendPackage(reinterpret_cast<uint8_t*>(&bat.battery), ExtControl::SET_VOLATILE_BAT);
	}
	else {
		ExtControl::sendPackage(reinterpret_cast<uint8_t*>(&bat), ExtControl::SET_BAT_SETTINGS);
	}
}

void MainWindow::updatePorts()
{
	if(port->isOpen())
		return;
	QHash<int, QString> oldPorts;
	QStringList newPorts;
	for(int x = 0; x < ui->portInfo->count(); ++x) {
		oldPorts.insert(x, ui->portInfo->itemText(x));
	}
	QString current = ui->portInfo->currentText();
	const auto infos = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo &info : infos)
		newPorts.append(info.portName());
	foreach (QString str, oldPorts.values())
	{
		if(!newPorts.contains(str))
			ui->portInfo->removeItem(oldPorts.key(str));
	}
	foreach (QString str, newPorts) {
		if(!oldPorts.values().contains(str))
			ui->portInfo->addItem(str);
	}
	ui->portInfo->setCurrentText(current);
}

eeprom::dataType eeprom::data;

void MainWindow::on_cmdSetup_clicked()
{
	uint8_t session_id = uint8_t(QRandomGenerator::global()->generate());
	ExtControl::setSessionID(session_id);
	ExtControl::commandType command;
	command.command = ExtControl::CMD_SETUP;
	command.data.data1 = ui->setupBatData1->currentIndex();
	command.data.data2 = ui->setupBatData2->currentIndex();
	ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
}

void MainWindow::on_cmdStart_clicked()
{
	ExtControl::commandType command;
	command.command = ExtControl::CMD_START;
	ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
}

void MainWindow::on_cmdStop_clicked()
{
	ExtControl::commandType command;
	command.command = ExtControl::CMD_STOP;
	ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
}

void MainWindow::on_cmdIdle_clicked()
{
	ExtControl::commandType command;
	command.command = ExtControl::CMD_IDLE;
	ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
}

void MainWindow::on_pushButton_clicked()
{
	ui->output->clear();
}

void MainWindow::sendServerKeepAlive()
{
		QString ip;
		const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
		for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
			if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
				 ip = address.toString();
		}
		QUrl url = QUrl(serverAddress + "database.php?chargerlive="+clientID+"&chargername="+clientName+"&chargerip="+ip);
		reply = qnam.get(QNetworkRequest(url));
		connect(reply, &QNetworkReply::finished, this, &MainWindow::httpFinished);
		connect(reply, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);

}

void MainWindow::httpReadyRead()
{
	//qDebug() << reply->readAll();
}

void MainWindow::httpFinished()
{
	QString username = "jose";
	QString password = "deltadelta";
	if (reply->error()) {
		qDebug() << reply->errorString();
		reply->deleteLater();
		return;
	}
	const QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if(redirectionTarget.toString().contains("login")) {
		qDebug() << "login needed";
		QByteArray loginData;
		loginData.append("username="+serverUsername+"&password="+serverPassword);
		QUrl url = QUrl(serverAddress + "login.php");
		reply->deleteLater();
		reply = qnam.post(QNetworkRequest(url), loginData);
		connect(reply, &QNetworkReply::finished, this, &MainWindow::httpFinished);
		connect(reply, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);
	}
	else {
		qDebug() << "OK";
		reply->deleteLater();
	}
}

void MainWindow::onOpenTriggered()
{
	options dialog(this);
	dialog.setSettings(&m_settings);
	int res = dialog.exec();
	if(res == QDialog::Accepted)
		loadServerDefs();

}
