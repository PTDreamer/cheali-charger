#include "httpserver.h"
#include "mainwindow.h"

static inline QString host(const QHttpServerRequest &request)
{
	return request.headers()[QStringLiteral("Host")].toString();
}
static inline QJsonObject ObjectFromString(const QString& in)
{
	QJsonObject obj;

	QJsonDocument doc = QJsonDocument::fromJson(in.toUtf8());

	// check validity of the document
	if(!doc.isNull())
	{
		if(doc.isObject())
		{
			obj = doc.object();
		}
		else
		{
			qDebug() << "Document is not an object" << endl;
		}
	}
	else
	{
		qDebug() << "Invalid JSON...\n" << in << endl;
	}

	return obj;
}
httpServer::httpServer(MainWindow *parent):QObject(parent), m_win(parent)
{
	connect(m_win, &MainWindow::settingsUpdated, this, &httpServer::onChargerSettingsUpdated);
	qhttpServer.setParent(parent);
	qhttpServer.route("/", []() {
		return "Hello world";
	});

	qhttpServer.route("/query", [] (const QHttpServerRequest &request) {
		qDebug() << request.body();
		if (request.method() == QHttpServerRequest::Method::Post) {
			QJsonObject o = ObjectFromString(request.body());
			qDebug() << o.value("key1").toString();
		}
		else {
			QJsonObject obj;
			obj.insert("backlight", currentChargerSettings->backlight);
			obj.insert("fan_temperature_on", currentChargerSettings->fanTempOn);
			obj.insert("discharge_temperature_off", currentChargerSettings->dischargeTempOff);
			obj.insert("input_voltage_low", currentChargerSettings->inputVoltageLow);
			obj.insert("minimum_ic", currentChargerSettings->minIc);
			obj.insert("minimum_id", currentChargerSettings->minId);
			obj.insert("maximum_ic", currentChargerSettings->maxIc);
			obj.insert("maximum_id", currentChargerSettings->maxId);
			obj.insert("maximum_pc", currentChargerSettings->maxPc);
			obj.insert("audio_beep", currentChargerSettings->audioBeep);
			obj.insert("adc_noise", currentChargerSettings->adcNoise);
		}
		return QString("%1/query/").arg(host(request));

	});

	qhttpServer.route("/query/", [] (qint32 id, const QHttpServerRequest &request) {
		return QString("%1/query/%2").arg(host(request)).arg(id);
	});

	qhttpServer.route("/query/<arg>/log", [] (qint32 id, const QHttpServerRequest &request) {
		return QString("%1/query/%2/log").arg(host(request)).arg(id);
	});

	qhttpServer.route("/query/<arg>/log/", [] (qint32 id, float threshold,
											  const QHttpServerRequest &request) {
		return QString("%1/query/%2/log/%3").arg(host(request)).arg(id).arg(threshold);
	});

	qhttpServer.route("/user/", [] (const qint32 id) {
		return QString("User %1").arg(id);
	});

	qhttpServer.route("/user/<arg>/detail", [] (const qint32 id) {
		return QString("User %1 detail").arg(id);
	});

	qhttpServer.route("/user/<arg>/detail/", [] (const qint32 id, const qint32 year) {
		return QString("User %1 detail year - %2").arg(id).arg(year);
	});

	qhttpServer.route("/json/", [] {
		return QJsonObject{
			{
				{"key1", "1"},
				{"key2", "2"},
				{"key3", "3"}
			}
		};
	});

	qhttpServer.route("/assets/<arg>", [] (const QUrl &url) {
		return QHttpServerResponse::fromFile(QStringLiteral(":/assets/%1").arg(url.path()));
	});

	qhttpServer.route("/remote_address", [](const QHttpServerRequest &request) {
		return request.remoteAddress().toString();
	});
}
bool httpServer::start(quint16 m_port) {
	const auto port = qhttpServer.listen(QHostAddress::Any, m_port);
	if (port == -1) {
		qDebug() << QCoreApplication::translate(
				"QHttpServerExample", "Could not run on http://127.0.0.1:%1/").arg(port);
		return 0;
	}

	qDebug() << QCoreApplication::translate(
			"QHttpServerExample", "Running on http://127.0.0.1:%1/ (Press CTRL+C to quit)").arg(port);
	return true;
}

void httpServer::onChargerSettingsUpdated(Settings *s)
{
	currentChargerSettings = s;
}

Settings *httpServer::currentChargerSettings = nullptr;
