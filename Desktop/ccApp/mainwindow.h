#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "../../src/core/drivers/ExtControl.h"
#include <QTimer>
#include <QSettings>
#include <QNetworkAccessManager>
#include <options.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	Settings *getCurrentSettings() const;
	void setCurrentSettings(Settings *value);

private slots:
	void on_pbConnect_clicked();
	void handleReadyRead();
	void handleError();
	void idleSlot();
	void onMessageReceived(int, uint8_t *buffer, uint8_t sessionID);
	void onMessageSent(int, uint8_t *buffer, bool);
	void onMessageHandled(int);
	void on_batteries_currentIndexChanged(int index);
	void on_pushButton_5_clicked();
	void on_pushButton_6_clicked();
	void on_pushButton_4_clicked();
	void on_pushButton_7_clicked();
	void on_pushButton_8_clicked();
	void updatePorts();
	void on_cmdSetup_clicked();

	void on_cmdStart_clicked();

	void on_cmdStop_clicked();

	void on_cmdIdle_clicked();

	void setState(ExtControl::current_state_type state = ExtControl::LAST_STATE_TYPE);
	void setError(ExtControl::error_type error = ExtControl::LAST_ERROR_TYPE);
	void setProgram(Program::ProgramType prog = Program::LAST_PROGRAM_TYPE);
	void on_pushButton_clicked();
	void sendServerKeepAlive();
	void httpReadyRead();
	void httpFinished();
	void onOpenTriggered();
private:
	QList<QNetworkCookie>  cookies;
	QNetworkReply *reply;
	QNetworkAccessManager qnam;
	QHash<ExtControl::message_type, QString> typeToStr;
	QSettings m_settings;
	QTimer idleTimer;
	Ui::MainWindow *ui;
	QSerialPort *port;
	void loadBattery(ProgramData::Battery *bat);
	void loadSettings(Settings *settings);
	void setBattery(int index);
	void setSettings(Settings *s);
	void setRealValues(ExtControl::realInputs *i);
	void setVirtualValues(ExtControl::virtualInputs *i);
	void setExtraValues(ExtControl::extraValues *i);

	QHash<int, ProgramData::Battery *> batteries;
	QHash<ExtControl::current_state_type, QString> states;
	QHash<ExtControl::error_type, QString> errors;
	QHash<Program::ProgramType, QString> programs;

	void handleBattery(uint8_t *buffer);

	QString clientID;
	QString clientName;
	QString serverUsername;
	QString serverPassword;
	QString serverAddress;
	bool connectToServer;
	QTimer *keepalive;
	void loadServerDefs();
	Settings *currentSettings;
signals:
	void settingsUpdated(Settings *s);
};

#endif // MAINWINDOW_H
