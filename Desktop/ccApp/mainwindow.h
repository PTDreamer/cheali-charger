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
#include <QMutex>
#include <QWaitCondition>

namespace Ui {
class MainWindow;
}
class httpServer;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	struct all_stat_type{
		int state;
		QString stateStr;
		int error;
		QString errorStr;
		int program;
		QString programStr;
	};
	void loadAndSaveOptions(Settings *val);
	Settings *getOptions();
	QMap<int, ProgramData::Battery *> *getBatteries();
	quint64 getCurrentSessionDB() const;
	void setCurrentSessionDB(const quint64 &value);//TODO

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
	void onOpenTriggered();
	void onSettingsChanged(Settings *settings);

	void on_pbtest_clicked();

private:
	httpServer *http_server;
	quint64 currentSessionDB;
	QList<QNetworkCookie>  cookies;
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

	QMap<int, ProgramData::Battery *> batteries;
	QHash<ExtControl::current_state_type, QString> states;
	QHash<ExtControl::error_type, QString> errors;
	QHash<Program::ProgramType, QString> programs;

	void handleBattery(uint8_t *buffer);

	all_stat_type all_stat;
	QMutex waitForSettings;
	QWaitCondition waitForSettingsCond;
	Settings lastReceivedSettings;
signals:
	void settingsChanged(Settings *s);
	void ackReceived(uint8_t);
};

#endif // MAINWINDOW_H
