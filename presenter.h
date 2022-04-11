#ifndef PRESENTER_H
#define PRESENTER_H

#include <QObject>
#include <datacollector.h>
#include <mainwindow.h>

class Presenter : public QObject
{
    Q_OBJECT
public:
    explicit Presenter(MainWindow *mainwindow, QObject *parent = 0);

signals:

public slots:
    void askSerialPortList();
    void selectSerialPort(QString port);
    void sendMsg(void);
    void openPort();
    void closePort();

private:
    MainWindow* main;
    DataCollector collector;
    DataCollection *regData;

};

#endif // PRESENTER_H
