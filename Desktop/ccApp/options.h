#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class options;
}

class options : public QDialog
{
	Q_OBJECT

public:
	explicit options(QWidget *parent = nullptr);
	void setSettings(QSettings *set);
	~options();

private:
	QSettings *m_settings;
	Ui::options *ui;
private slots:
	void onAccepted();
};

#endif // OPTIONS_H
