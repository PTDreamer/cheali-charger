#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include <QtHttpServer>
#include <QObject>
#include "mainwindow.h"

class httpServer : public QObject
{
	Q_OBJECT
public:
	httpServer(MainWindow *parent);
	static inline QString host(const QHttpServerRequest &request)
	{
		return request.headers()[QStringLiteral("Host")].toString();
	}
	bool start(quint16 port);
	void loadServerDefs();
	quint64 getNewSessionID(quint64 bat_id, int op_id);
	void stopSessionID(quint64 session);
private:
	QHttpServer qhttpServer;
	MainWindow *m_win;
	static MainWindow::all_stat_type all_stat;
	void processStartProgram(ProgramData::Battery *bat, unsigned char program, quint64 bat_id);
	void processStopProgram(quint64 session);
	void sendJSON(QJsonDocument data, QString url);
	QNetworkAccessManager qnam;
	void sendServerKeepAlive();
	QNetworkReply *reply;
	QNetworkReply *ka_reply;
	QString clientID;
	QString clientName;
	QString serverUsername;
	QString serverPassword;
	QString serverAddress;
	QSettings m_settings;
	bool connectToServer;
	QTimer *keepalive;
	enum action_status_type {IDLE, WAIT_FOR_SET_VOL_ACK, WAIT_FOR_SETUP, WAIT_FOR_START, FINISHED, FAILED};
	struct current_action_type {
		ProgramData::Battery *bat;
		unsigned char program;
		action_status_type status;
		uint8_t message_ack;
		quint64 bat_id;
		quint64 session;
	} currentAction;
private slots:
	void serviceRequestFinish(QNetworkReply*);
	void httpFinished();
public slots:
	void sendInputs(ExtControl::realInputs *data);
	void sendInputs(ExtControl::virtualInputs *data);
	void sendInputs(ExtControl::extraValues *data);
	void sendInputs();
	void ackReceived(uint8_t message_number);
};

#endif // HTTPSERVER_H
