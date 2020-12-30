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
httpServer::httpServer(MainWindow *parent):QObject(parent), m_win(parent), keepalive(nullptr)
{
	currentAction.status = IDLE;
	connect(m_win, &MainWindow::ackReceived, this, &httpServer::ackReceived);
	qnam.setCookieJar(new QNetworkCookieJar(this));
	qhttpServer.setParent(parent);
	qhttpServer.route("/", []() {
		return "Hello world";
	});

	qhttpServer.route("/settings", [this] (const QHttpServerRequest &request) {
		QJsonObject ret;
		QJsonArray result;
		QJsonObject obj;
		ret.insert("error", "none");
		qDebug() << request.body();
		if (request.method() == QHttpServerRequest::Method::Post) {
			QJsonObject o = ObjectFromString(request.body());
			if(o.isEmpty()) {
				ret["error"] = "invalid json received";
			}
			Settings s;
			qDebug() << o.value("backlight") << "..." << o.value("backlight").toString().toLong();
			s.backlight = o.value("backlight").toString().toUShort();
			s.fanTempOn = o.value("fan_temperature_on").toString().toUShort();
			s.dischargeTempOff = o.value("discharge_temperature_off").toString().toUShort();
			s.inputVoltageLow = o.value("input_voltage_low").toString().toUShort();
			s.minIc = o.value("minimum_ic").toString().toUShort();
			s.minId = o.value("minimum_id").toString().toUShort();
			s.maxIc = o.value("maximum_ic").toString().toUShort();
			s.maxId = o.value("maximum_id").toString().toUShort();
			s.maxPc = o.value("maximum_pc").toString().toUShort();
			s.audioBeep = o.value("audio_beep").toString().toUShort();
			s.adcNoise = o.value("adc_noise").toString().toUShort();
			s.fanOn = o.value("fan_on").toString().toUShort();
			s.UART = o.value("uart").toString().toUShort();
			s.UARTspeed = o.value("uart_speed").toString().toUShort();
			s.UARToutput = o.value("uart_output").toString().toUShort();
			s.menuType = o.value("menu_type").toString().toUShort();
			s.menuButtons = o.value("menu_buttons").toString().toUShort();
			this->m_win->loadAndSaveOptions(&s);
		}
		else if (request.method() == QHttpServerRequest::Method::Get) {
			Settings *currentChargerSettings = m_win->getOptions();
			if(currentChargerSettings) {
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
				obj.insert("fan_on", currentChargerSettings->fanOn);
				obj.insert("uart", currentChargerSettings->UART);
				obj.insert("uart_speed", currentChargerSettings->UARTspeed);
				obj.insert("uart_output", currentChargerSettings->UARToutput);
				obj.insert("menu_type", currentChargerSettings->menuType);
				obj.insert("menu_buttons", currentChargerSettings->menuButtons);
				result.append(obj);
				ret.insert("result", result);
			}
			else {
				ret["error"] = "There was a problem fetching settings";
			}
		}
		QHttpServerResponse r = QHttpServerResponse(ret);
		r.setHeader("Access-Control-Allow-Origin", "*");
		return r;
		//return QString("%1/query/").arg(host(request));

	});
	qhttpServer.route("/batlist", [this] (const QHttpServerRequest &request) {
		QJsonObject ret;
		QJsonArray result;
		QJsonObject obj;
		ret.insert("error", "none");
		qDebug() << request.body();
		if (request.method() == QHttpServerRequest::Method::Post) {
			QJsonObject o = ObjectFromString(request.body());
			if(o.isEmpty()) {
				ret["error"] = "invalid json received";
			}
		}
		else if (request.method() == QHttpServerRequest::Method::Get) {
			QMap<int, ProgramData::Battery *> *b = m_win->getBatteries();
			foreach (int key, b->keys()) {
				obj.insert("idx", key);
				obj.insert("cells", b->value(key)->cells);
				obj.insert("capacity", b->value(key)->capacity);
				obj.insert("type", b->value(key)->type);
				result.append(obj);
			}
			ret.insert("result", result);

		}
		QHttpServerResponse r = QHttpServerResponse(ret);
		r.setHeader("Access-Control-Allow-Origin", "*");
		return r;
		//return QString("%1/query/").arg(host(request));

	});

	qhttpServer.route("/action", [this] (const QHttpServerRequest &request) {
		QJsonObject ret;
		QJsonArray result;
		QJsonObject obj;
		ret.insert("error", "none");
		qDebug() << request.body();
		if (request.method() == QHttpServerRequest::Method::Post) {
			QJsonObject o = ObjectFromString(request.body());
			if(o.isEmpty()) {
				ret["error"] = "invalid json received";
			}
			if(o.value("action").toString() != "stop") {
				ProgramData::Battery *bat = new ProgramData::Battery;
				bat->Ic = o.value("Ic").toString().toUShort();
				bat->Id = o.value("Id").toString().toUShort();
				bat->time = o.value("time").toString().toUShort();
				bat->type = o.value("type").toString().toUShort();
				bat->cells = o.value("cells").toString().toUShort();
				bat->minIc = o.value("minIc").toString().toUShort();
				bat->minId = o.value("minId").toString().toUShort();
				bat->deltaT = o.value("delta_t").toString().toUShort();
				bat->deltaV = o.value("delta_v").toString().toShort();
				bat->DCcycles = o.value("dc_cyclbattery_ides").toString().toUShort();
				bat->capacity = o.value("capacity").toString().toUShort();
				bat->capCutoff = o.value("capCutoff").toString().toUShort();
				bat->externTCO = o.value("externTCO").toString().toUShort();
				bat->DCRestTime = o.value("DCRestTime").toString().toUShort();

				switch (ProgramData::batteryClassMap[bat->type]) {
				case ProgramData::ClassPb:
					bat->Vc_per_cell = o.value("Vc_per_cell_forLixx").toString().toUShort();
					break;
				case ProgramData::ClassLED:
					bat->Vc_per_cell = o.value("Vc_per_cell_forLED").toString().toUShort();
					break;
				case ProgramData::ClassLiXX:
					bat->Vc_per_cell = o.value("Vc_per_cell_forLixx").toString().toUShort();
					break;
				case ProgramData::ClassNiXX:
					bat->Vc_per_cell = o.value("Vc_per_cell_forNixx").toString().toUShort();
					break;
				case ProgramData::ClassNiZn:
					bat->Vc_per_cell = o.value("Vc_per_cell_forLixx").toString().toUShort();
					break;
				case ProgramData::ClassUnknown:
					bat->Vc_per_cell = o.value("Vc_per_cell_forUnk").toString().toUShort();
					break;
				}

				bat->Vd_per_cell = o.value("Vd_per_cell").toString().toUShort();
				bat->Vs_per_cell = o.value("Vs_per_cell").toString().toUShort();
				bat->balancerError = o.value("balancer_error").toString().toUShort();
				bat->enable_deltaV = o.value("delta_v_enable").toString().toUShort();
				bat->enable_externT = o.value("enable_externT").toString().toUShort();
				bat->deltaVIgnoreTime = o.value("delta_v_ignore_t").toString().toUShort();
				bat->enable_adaptiveDischarge = o.value("enable_adaptiveDischarge").toString().toUShort();
				unsigned char action = o.value("action").toString().toUShort();
				quint64 bat_id = o.value("battery_id").toString().toULongLong();
				this->processStartProgram(bat, action, bat_id);
			}
			else {
				qDebug() << o.value("session").toDouble();
				qDebug() << o.value("session");
				this->processStopProgram(o.value("session").toDouble());
			}
		}
		else if (request.method() == QHttpServerRequest::Method::Get) {
		}
		QHttpServerResponse r = QHttpServerResponse(ret);
		r.setHeader("Access-Control-Allow-Origin", "*");
		return r;
	});

	qhttpServer.route("/battery/", [this] (qint32 id, const QHttpServerRequest &request) {
		QJsonObject ret;
		QJsonObject obj;
		ret.insert("error", "none");
		if (request.method() == QHttpServerRequest::Method::Get) {
			ProgramData::Battery *bat = m_win->getBatteries()->value(id);
			obj.insert("idx", id);
			obj.insert("cells", bat->cells);
			obj.insert("capacity", bat->capacity);
			obj.insert("type", bat->type);
			obj.insert("Ic",bat->Ic);
			obj.insert("Id",bat->Id);
			obj.insert("Vc_per_cell",bat->Vc_per_cell);
			obj.insert("Vd_per_cell",bat->Vd_per_cell);
			obj.insert("minIc",bat->minIc);
			obj.insert("minId",bat->minId);
			obj.insert("time",bat->time);
			obj.insert("enable_externT",bat->enable_externT);
			obj.insert("externTCO",bat->externTCO);
			obj.insert("enable_adaptiveDischarge",bat->enable_adaptiveDischarge);
			obj.insert("DCRestTime",bat->DCRestTime);
			obj.insert("capCutoff",bat->capCutoff);
			obj.insert("Vs_per_cell",bat->Vs_per_cell);
			obj.insert("balancer_error",bat->balancerError);
			obj.insert("delta_v_enable",bat->enable_deltaV);
			obj.insert("delta_v",bat->deltaV);
			obj.insert("delta_v_ignore_t",bat->deltaVIgnoreTime);
			obj.insert("delta_t",bat->deltaT);
			obj.insert("dc_cycles",bat->DCcycles);
			ret.insert("result", obj);
		}
		QHttpServerResponse r = QHttpServerResponse(ret);
		r.setHeader("Access-Control-Allow-Origin", "*");
		return r;
		//return QString("%1/query/").arg(host(request));

	});

	qhttpServer.route("/getstatus", [this] (const QHttpServerRequest &request) {
		QJsonObject ret;
		QJsonArray result;
		QJsonObject obj;
		ret.insert("error", "none");
		if (request.method() == QHttpServerRequest::Method::Get) {
			obj.insert("error", all_stat.error);
			obj.insert("program", all_stat.program);
			obj.insert("state", all_stat.state);
			obj.insert("errorStr", all_stat.errorStr);
			obj.insert("programStr", all_stat.programStr);
			obj.insert("stateStr", all_stat.stateStr);
			obj.insert("logicState", currentAction.status);
			if(currentAction.status == FINISHED || currentAction.status == FAILED)
				currentAction.status = IDLE;
			result.append(obj);
			ret.insert("result", result);
		}
		QHttpServerResponse r = QHttpServerResponse(ret);
		r.setHeader("Access-Control-Allow-Origin", "*");
		return r;
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

void httpServer::processStartProgram(ProgramData::Battery *bat, unsigned char program, quint64 bat_id)
{
	ExtControl::sendPackageResult res = ExtControl::sendPackage(reinterpret_cast<uint8_t*>(bat), ExtControl::SET_VOLATILE_BAT);
	qDebug() << "ProcessStartProgram program=" << program << " bat_id=" << bat_id;
	if(!res.sent)
	{
		currentAction.status = FAILED;
		return;
	}
	currentAction.bat_id = bat_id;
	currentAction.status = WAIT_FOR_SET_VOL_ACK;
	currentAction.message_ack = res.number;
	currentAction.bat = bat;
	currentAction.program = program;
}

void httpServer::processStopProgram(quint64 session)
{
	qDebug() << "Q" << session;
	ExtControl::commandType cmd;
	cmd.command = ExtControl::CMD_STOP;
	ExtControl::sendPackage(reinterpret_cast<uint8_t*>(&cmd), ExtControl::COMMAND);
	stopSessionID(session);
	ExtControl::setSessionID(0);
	m_win->setCurrentSessionDB(0);
}

void httpServer::serviceRequestFinish(QNetworkReply *rep)
{
	qDebug() << rep->readAll();
	bool sentOk = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
	qDebug() << sentOk << rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

void httpServer::sendInputs(ExtControl::realInputs *data)
{
	QString url = "sessionDB.php";
	QJsonObject json;
	json.insert("Vout_plus_pin", data->Vout_plus_pin);
	json.insert("Vout_minus_pin", data->Vout_minus_pin);
	json.insert("Ismps", data->Ismps);
	json.insert("Idischarge", data->Idischarge);
	json.insert("VoutMux", data->VoutMux);
	json.insert("Tintern", data->Tintern);
	json.insert("Vin", data->Vin);
	json.insert("Textern", data->Textern);
	json.insert("Vb0_pin", data->Vb0_pin);
	json.insert("Vb1_pin", data->Vb1_pin);
	json.insert("Vb2_pin", data->Vb2_pin);
	json.insert("Vb3_pin", data->Vb3_pin);
	json.insert("Vb4_pin", data->Vb4_pin);
	json.insert("Vb5_pin", data->Vb5_pin);
	json.insert("Vb6_pin", data->Vb6_pin);
	json.insert("IsmpsSet", data->IsmpsSet);
	json.insert("IdischargeSet", data->IdischargeSet);
	json.insert("session_id", QJsonValue::fromVariant(m_win->getCurrentSessionDB())); //TODO
	json.insert("strategy", data->strategy);
	QJsonObject jarr;
	jarr.insert("data",json);
	jarr.insert("type", "realInputs");
	sendJSON(QJsonDocument(jarr), url);
}

void httpServer::sendInputs(ExtControl::virtualInputs *data)
{
	QString url = "sessionDB.php";
	QJsonObject json;
	json.insert("Vb1", data->Vb1);
	json.insert("Vb2", data->Vb2);
	json.insert("Vb3", data->Vb3);
	json.insert("Vb4", data->Vb4);
	json.insert("Vb5", data->Vb5);
	json.insert("Vb6", data->Vb6);
	json.insert("Cout", data->Cout);
	json.insert("Eout", data->Eout);
	json.insert("Iout", data->Iout);
	json.insert("Pout", data->Pout);
	json.insert("Vout", data->Vout);
	json.insert("VobInfo", data->VobInfo);
	json.insert("Vbalancer", data->Vbalancer);
	json.insert("deltaVout", data->deltaVout);
	json.insert("VbalanceInfo", data->VbalanceInfo);
	json.insert("VoutBalancer", data->VoutBalancer);
	json.insert("deltaTextern", data->deltaTextern);
	json.insert("deltaVoutMax", data->deltaVoutMax);
	json.insert("deltaLastCount", data->deltaLastCount);

	json.insert("session_id", QJsonValue::fromVariant(m_win->getCurrentSessionDB())); //TODO
	QJsonObject jarr;
	jarr.insert("data",json);
	jarr.insert("type", "virtualInputs");
	sendJSON(QJsonDocument(jarr), url);
}

void httpServer::sendInputs(ExtControl::extraValues *data)
{
	QString url = "sessionDB.php";
	QJsonObject json;
	json.insert("PID", data->PID);
	json.insert("Balance", data->Balance);
	json.insert("BattRth", data->BattRth);
	json.insert("ETATime", QJsonValue::fromVariant(data->ETATime));
	json.insert("RthCell1", data->RthCell1);
	json.insert("RthCell2", data->RthCell2);
	json.insert("RthCell3", data->RthCell3);
	json.insert("RthCell4", data->RthCell4);
	json.insert("RthCell5", data->RthCell5);
	json.insert("RthCell6", data->RthCell6);
	json.insert("WiresRth", data->WiresRth);
	json.insert("FreeStack", data->FreeStack);
	json.insert("percentCompleted", data->percentCompleted);
	json.insert("NeverUsedStackSize", data->NeverUsedStackSize);

	json.insert("session_id", QJsonValue::fromVariant(m_win->getCurrentSessionDB())); //TODO
	QJsonObject jarr;
	jarr.insert("data",json);
	jarr.insert("type", "extraInputs");
	sendJSON(QJsonDocument(jarr), url);
}

void httpServer::sendInputs()
{	//TODO
	ExtControl::realInputs r = ExtControl::realInputs();
	r.Vin = 69;
	sendInputs(&r);
}

void httpServer::ackReceived(uint8_t message_number)
{
	qDebug() << "ack received:" << message_number;
	ExtControl::commandType command;
	ExtControl::sendPackageResult res;
	quint64 sessionID;
	if(message_number == currentAction.message_ack) {
		switch (currentAction.status) {
		case WAIT_FOR_SET_VOL_ACK:
			sessionID = getNewSessionID(currentAction.bat_id, currentAction.program);
			if(sessionID == 0) {
				currentAction.status = FAILED;
				return;
			}
			ExtControl::setSessionID(sessionID);
			m_win->setCurrentSessionDB(sessionID);
			command.command = ExtControl::CMD_SETUP;
			command.data.data1 = MAX_PROGRAMS;
			command.data.data2 = currentAction.program;
			res = ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
			qDebug() << "sendCommand on WAIT_FOR_SET_VOL_ACK" << "program="<< currentAction.program << " number:"<< res.number << " sent=" << res.sent;
			if(!res.sent) {
				currentAction.status = FAILED;
				return;
			}
			currentAction.message_ack = res.number;
			currentAction.status = WAIT_FOR_SETUP;
			break;
		case WAIT_FOR_SETUP:
			command.command = ExtControl::CMD_START;
			res = ExtControl::sendPackage((uint8_t *)&command, ExtControl::COMMAND);
			qDebug() << "sendCommand on WAIT_FOR_SETUP" << "program="<< currentAction.program << " number:"<< res.number << " sent=" << res.sent;
			if(!res.sent) {
				currentAction.status = FAILED;
				return;
			}
			currentAction.message_ack = res.number;
			currentAction.status = WAIT_FOR_START;
			break;
		case WAIT_FOR_START:
			currentAction.status = FINISHED;
			break;
		default:
			break;
		}
	}
}

void httpServer::sendJSON(QJsonDocument data, QString url)
{
	QUrl serviceUrl = QUrl(serverAddress + url);
	qDebug() << serviceUrl;
	QNetworkRequest request(serviceUrl);
	QByteArray jsonData= data.toJson();
	request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
	request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(jsonData.size()));
	//connect(&qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(serviceRequestFinish(QNetworkReply*)), Qt::UniqueConnection);
	qnam.post(request, jsonData);
}

void httpServer::sendServerKeepAlive()
{
		QString ip;
		const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
		for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
			if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
				 ip = address.toString();
		}
		QUrl url = QUrl(serverAddress + "database.php?chargerlive="+clientID+"&chargername="+clientName+"&chargerip="+ip + ":8081");
		ka_reply = qnam.get(QNetworkRequest(url));
		connect(ka_reply, &QNetworkReply::finished, this, &httpServer::httpFinished, Qt::UniqueConnection);

}

quint64 httpServer::getNewSessionID(quint64 bat_id, int op_id)
{
	QUrl url = QUrl(serverAddress + "sessionDB.php?action=getSessionID&chargerID="+clientID + "&bat_id="+QString::number(bat_id) + "&op_id="+QString::number(op_id));
	qDebug() << url;
	reply = qnam.get(QNetworkRequest(url));
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::UniqueConnection);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit, Qt::UniqueConnection);
	timer.start(3000);
	loop.exec();
	if(!timer.isActive()) {
		qDebug() << "timeout requesting new session";
		return 0;
	}
	QByteArray res = reply->readAll();
	QJsonObject o = ObjectFromString(res);
	if(o.isEmpty()) {
		qDebug() << "error requesting new session, empty response:" << res;
		return 0;
	}
	if(o.value("error").toString() != "none") {
		qDebug() << "error timeout requesting new session:" << o.value("error").toString();
		return 0;
	}
	qDebug() << o.value("newID");
	return o.value("newID").toDouble();
}

void httpServer::stopSessionID(quint64 session)
{
	qDebug() << "QQ" << session;
	QUrl url = QUrl(serverAddress + "sessionDB.php?action=stopSession&session=" + QString::number(session));
	qDebug() << url;
	reply = qnam.get(QNetworkRequest(url));
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::UniqueConnection);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit, Qt::UniqueConnection);
	timer.start(3000);
	loop.exec();
	qDebug() << reply->readAll();
}

void httpServer::httpFinished()
{
	QString username = "jose";
	QString password = "deltadelta";
	if (ka_reply->error()) {
		qDebug() << "Error string on httpFinished" << ka_reply->errorString();
		reply->deleteLater();
		return;
	}
	const QVariant redirectionTarget = ka_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if(redirectionTarget.toString().contains("login")) {
		qDebug() << "login needed";
		QByteArray loginData;
		loginData.append("username="+serverUsername+"&password="+serverPassword);
		QUrl url = QUrl(serverAddress + "login.php");
		ka_reply->deleteLater();
		ka_reply = qnam.post(QNetworkRequest(url), loginData);
		connect(ka_reply, &QNetworkReply::finished, this, &httpServer::httpFinished, Qt::UniqueConnection);
	}
	else {
		ka_reply->deleteLater();
	}
}

void httpServer::loadServerDefs()
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
		connect(keepalive, &QTimer::timeout, this, &httpServer::sendServerKeepAlive, Qt::UniqueConnection);
		keepalive->start(5000);
	}
	else {
		if(keepalive)
			keepalive->stop();
	}
}

MainWindow::all_stat_type httpServer::all_stat;
