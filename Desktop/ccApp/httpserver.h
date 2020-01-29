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
private:
	QHttpServer qhttpServer;
	MainWindow *m_win;
	static Settings *currentChargerSettings;
private slots:
	void onChargerSettingsUpdated(Settings *s);
};

#endif // HTTPSERVER_H
