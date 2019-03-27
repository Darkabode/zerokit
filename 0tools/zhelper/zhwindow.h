#ifndef ZHWINDOW_H
#define ZHWINDOW_H

#include <QSettings>
#include <QtGui/QMainWindow>
#include "ui_zhwindow.h"

class ZHelperWindow : public QMainWindow
{
	Q_OBJECT

public:
	ZHelperWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~ZHelperWindow();

private slots:
	void on_createButton_clicked();

private:
	Ui::ZHelperWindowClass ui;

	QSettings _settings;
};

#endif // ZHWINDOW_H
