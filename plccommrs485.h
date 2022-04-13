/****************************************************************************
**
** Copyright (C) 2018 Aliaksandr Bandarenka
** Contact: alexander.bond081@gmail.com
**
** This file is part of the RS_HMI.
**
** RS_HMI is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** RS_HMI is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with RS_HMI.  If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef PLCCOMMRS485_H
#define PLCCOMMRS485_H

/* данный класс реализует подключение порта для связи с контроллером
   и предоставляет интерфейс для работы с отправкой и приёмом данных
   сам механизм построения и отправки посылок надо реализовывать в наследниках */

#include <QObject>
#include <QList>
#include <QtSerialPort/QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
//#include "datacollection.h"
#include "plcregistervalidator.h"

enum PlcConnectionStatus{
    Ok,
    NoConnection,
    HasErrors,
    SoftwareErrors,
    HardwareErrors,
    UnknownError
};

class PlcCommRS485: public QObject
{
    Q_OBJECT

public:

    static const QString PORTERRORS[14];
    static const QString BAUDRATES[5];
    static const QString DATABITS[4];
    static const QString PARITY[3];
    static const QString STOPBITS[3];
    //static const QString flowControl[3];
    //static const QString pinounts[2];

    explicit PlcCommRS485(QObject *parent);
    virtual ~PlcCommRS485();

    QList<QSerialPortInfo> getPortList(void);
    QStringList getBaudList() {return baudList;}
    QStringList getBitsList() {return bitsList;}
    QStringList getParityList() {return parityList;}
    QStringList getStopList() {return stopList;}

    QString getPortName(){return port.portName();}
    QSerialPortInfo getPortInfo(){QSerialPortInfo info; return info;}
    QString getBaud() {return QString::number(port.baudRate());}
    QString getBits() {return DATABITS[(int)port.dataBits()-5];}
    QString getParity() {
        switch(port.parity()){
        case 2: return PARITY[1];
        case 3: return PARITY[2];
        default: return PARITY[0];
    }}
    QString getStopBits() {
        switch(port.stopBits()){
        case 2: return STOPBITS[1];
        case 3: return STOPBITS[2];
        default: return STOPBITS[0];
    }}
    int getRequestDelay() {return requestDelay;}
    int getRequestTimeout() {return requestTimeout;}

    void setPortParameters(const QString &name, const QString &baud="38400", const QString &bits="8", const QString &parity="Even", const QString &stop="1");
    bool openPort();
    void closePort();
    void setRequestDelay(int delay);
    void setRequestTimeout(int timeout);

    bool portIsOpen();
    PlcConnectionStatus plcConnectionStatus();

    virtual void askSomeDebugInfo()=0; // этот метод для дебага нужен

    virtual void sendnext()=0; // следующий шаг обмена данными
    virtual void communicationStart()=0; // запуск цикла обмена данными
    virtual void communicationStop()=0; // остановка
    virtual bool communicationIsOn()=0; // т.к. в этом классе не реализован механизм запуска цикла обмена, то и определить его состояние не представляется возможным

    virtual PLCRegisterValidator* makeValidator(void)=0; // создаёт валидатор, который потом программа
                                              // будет передавать создаваемым регистрам.

public slots:
    void setPortBaud(const QString &baud);
    void setPortBits(const QString &bits);
    void setPortParity(const QString &parity);
    void setPortStopBits(const QString &stop);
    void setPortName(const QString &portname);

signals:
    void error(QString err);
    void msg(QString msg);
    void state(bool connected);

protected slots:
    virtual void readData()=0;
    void portError(QSerialPort::SerialPortError error);

protected:
    PlcConnectionStatus plcConStatus=NoConnection;
    QSerialPort port;
    int requestDelay=200; // msec
    int requestTimeout=1500; // msec
    int reopenPortDelay=5000;// msec
    int timeoutsToReconnect=30;

private:
    QStringList portErrList, baudList, bitsList, parityList, stopList;
    inline void fillStrList(QStringList &lst, const QString ARRAY[], uint n) { lst.clear(); for(uint i=0;i<n;i++) lst.append(ARRAY[i]); }

};

#endif // PLCCOMMRS485_H
