#include "options.h"
#include "ui_options.h"
#include <QValidator>
options::options(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::options)
{
	ui->setupUi(this);
	QValidator *validator = new QIntValidator(0, 99999999, this);
	ui->le_clientID->setValidator(validator);
	connect(this, &QDialog::accepted, this, &options::onAccepted);
}

void options::setSettings(QSettings *set)
{
	 m_settings = set;
	 ui->le_clientID->setText(set->value("clientID", 0).toString());
	 ui->le_password->setText(set->value("server_password", "").toString());
	 ui->le_username->setText(set->value("server_username", "").toString());
	 ui->le_serverAdress->setText(set->value("server_address", "").toString());
	 ui->le_clientName->setText(set->value("clientName", "").toString());
	 ui->le_add_to_send->setText(set->value("server_add_to_send").toString());
	 ui->cb_connectToServer->setChecked(set->value("connect_to_server", false).toBool());
}

options::~options()
{
	delete ui;
}

void options::onAccepted()
{
	QString s_address = ui->le_serverAdress->text();
	if(!s_address.endsWith("/"))
		s_address.append("/");
	if(!s_address.startsWith("http"))
		s_address.prepend("http://");
	m_settings->setValue("clientID", ui->le_clientID->text());
	m_settings->setValue("server_password", ui->le_password->text());
	m_settings->setValue("server_username", ui->le_username->text());
	m_settings->setValue("server_address", s_address);
	m_settings->setValue("server_add_to_send", ui->le_add_to_send->text());
	m_settings->setValue("clientName", ui->le_clientName->text());
	m_settings->setValue("connect_to_server", ui->cb_connectToServer->isChecked());
}
