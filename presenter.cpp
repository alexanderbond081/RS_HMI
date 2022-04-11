#include "presenter.h"

Presenter::Presenter(MainWindow *mainwindow, QObject *parent) :
    QObject(parent)
{
    main = mainwindow;
    regData = new DataCollection;
    collector.setDataCollection(regData);
    QObject::connect(main, SIGNAL(askSerilPortList()), this, SLOT(askSerialPortList()));
    QObject::connect(main, SIGNAL(askSerilPortList()), this, SLOT(selectSerialPort(QString)));
    QObject::connect(main, SIGNAL(updatePortData()), this, SLOT(sendMsg()));
    QObject::connect(main, SIGNAL(openPort()), this, SLOT(openPort()));
    QObject::connect(main, SIGNAL(closePort()), this, SLOT(closePort()));
    QObject::connect(&collector, SIGNAL(msg(QString)), main, SLOT(getMsg(QString)));
}

void Presenter::askSerialPortList(void){
}

void Presenter::selectSerialPort(QString port){
}

void Presenter::sendMsg()
{
}

void Presenter::openPort()
{
}

void Presenter::closePort()
{
}
