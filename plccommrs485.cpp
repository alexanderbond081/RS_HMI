/****************************************************************************
**
** Copyright (C) 2018 Aliaksandr Bandarenka, PTI NAS Belarus
** Contact: sashka3001@gmail.com
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

#include "plccommrs485.h"


const QString PlcCommRS485::PORTERRORS[14]={"NoError", "DeviceNotFoundError", "PermissionError", "OpenError", "ParityError",
                          "FramingError", "BreakConditionError", "WriteError", "ReadError", "ResourceError",
                          "UnsupportedOperationError", "UnknownError", "TimeoutError", "NotOpenError"};
const QString PlcCommRS485::BAUDRATES[5]={"9600","19200","38400","57600","115200"};
const QString PlcCommRS485::DATABITS[4]={"5","6","7","8"};
const QString PlcCommRS485::PARITY[3]={"No","Even","Odd"};
const QString PlcCommRS485::STOPBITS[3]={"1","2","1.5"};
//const QString PLCControllerRS485::flowControl[3]={"NoFlowControl", "HardwareControl", "SoftwareControl"};
//const QString PLCControllerRS485::pinounts[2]={"noPinount","noPinouts!"};


PlcCommRS485::PlcCommRS485(QObject *parent): QObject(parent)
{
    #define COUNT(a) sizeof(a)/sizeof(a[0]) // так вот захотелось

    QObject::connect(&port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(portError(QSerialPort::SerialPortError)));
    QObject::connect(&port, SIGNAL(readyRead()), this, SLOT(readData()));
    fillStrList(portErrList, PORTERRORS, COUNT(PORTERRORS));
    fillStrList(baudList, BAUDRATES,COUNT(BAUDRATES));
    fillStrList(bitsList, DATABITS,COUNT(DATABITS));
    fillStrList(parityList, PARITY, COUNT(PARITY));
    fillStrList(stopList, STOPBITS, COUNT(STOPBITS));

    #undef COUNT
}

PlcCommRS485::~PlcCommRS485()
{
    port.close();
    portErrList.clear();
    baudList.clear();
    bitsList.clear();
    parityList.clear();
    stopList.clear();
}

QList<QSerialPortInfo> PlcCommRS485::getPortList(void)
{
  /*  QStringList lst;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        lst << info.portName();
    }
    return lst; */
    return QSerialPortInfo::availablePorts();
}

void PlcCommRS485::setPortBaud(const QString &baud)
{
    port.setBaudRate(baud.toInt()); // это не совсем то что надо, но должно сработать
    //emit msg(QString::number(baud.toInt()));
}

void PlcCommRS485::setPortBits(const QString &bits)
{
    QSerialPort::DataBits d;
    int n = bitsList.indexOf(bits);
    switch(n){
    case 0: d=QSerialPort::Data5;break;
    case 1: d=QSerialPort::Data6;break;
    case 2: d=QSerialPort::Data7;break;
    case 3: d=QSerialPort::Data8;break;
    default: d=QSerialPort::Data8;
    }
    port.setDataBits(d);
    //emit msg(DATABITS[n]);
}

void PlcCommRS485::setPortParity(const QString &parity)
{
    QSerialPort::Parity d;
    int n = parityList.indexOf(parity);
    switch(n){
    case 0: d=QSerialPort::NoParity;break;
    case 1: d=QSerialPort::EvenParity;break;
    case 2: d=QSerialPort::OddParity;break;
    default: d=QSerialPort::NoParity;
    }
    port.setParity(d);
    //emit msg(PARITY[n]);
}

void PlcCommRS485::setPortStopBits(const QString &stop)
{
    QSerialPort::StopBits d;
    int n = stopList.indexOf(stop);
    switch(n){
    case 0: d=QSerialPort::OneStop;break;
    case 1: d=QSerialPort::TwoStop;break;
    case 2: d=QSerialPort::OneAndHalfStop;break;
    default: d=QSerialPort::OneStop;
    }
    port.setStopBits(d);
    //emit msg(STOPBITS[n]);
}

void PlcCommRS485::setPortName(const QString &portname)
{
    port.setPortName(portname);
}

void PlcCommRS485::setPortParameters(const QString &name, const QString &baud, const QString &bits, const QString &parity, const QString &stop)
{
    if(!port.isOpen()){
        port.setPortName(name);
        setPortBaud(baud);
        setPortBits(bits);
        setPortParity(parity);
        setPortStopBits(stop);
    }
    //port.setFlowControl(port.NoFlowControl);
    //port.pinoutSignals() ??
}

bool PlcCommRS485::openPort()
{
    bool result=1;
    //if(!port.isOpen()){ - !!! после тестов вернуть?
        result=port.open(QIODevice::ReadWrite);
    //}
    return result;
}

void PlcCommRS485::closePort()
{
    //if(port.isOpen()) - после тестов вернуть?
    port.close();
}

bool PlcCommRS485::portIsOpen()
{
    return port.isOpen();
}

PlcConnectionStatus PlcCommRS485::plcConnectionStatus()
{
    return plcConStatus;
}

void PlcCommRS485::setRequestDelay(int delay)
{
    requestDelay = delay;
}

void PlcCommRS485::setRequestTimeout(int timeout)
{
    requestTimeout = timeout;
}

void PlcCommRS485::portError(QSerialPort::SerialPortError error)
{
    switch(error){
    case QSerialPort::NoError:
    case QSerialPort::OpenError:
        break;

    case QSerialPort::ParityError:
    case QSerialPort::FramingError:
    case QSerialPort::WriteError:
    case QSerialPort::TimeoutError:
    case QSerialPort::UnsupportedOperationError:
        // пока не знаю что делать в такой ситуации
        // думаю стоит добавить периодически обнуляющийся счётчик,
        // при достижении определённого значения будет происходить закрытие порта
        qDebug()<<"error dunno wat";
        break;

    case QSerialPort::NotOpenError:
        break;

    case QSerialPort::PermissionError:
        // qDebug()<<"error Permission";
        break;
    case QSerialPort::DeviceNotFoundError:
        // qDebug()<<"error DeviceNotFound";
        break;
    case QSerialPort::BreakConditionError:
        qDebug()<<"error BreakConditions";
        //if(port.isOpen()) port.close();
        break;
    case QSerialPort::ReadError:
        if(port.isOpen()){
            port.clearError();
            port.close();
        }
        break;
    case QSerialPort::ResourceError:
        qDebug()<<"error ResoueceError";
        if(port.isOpen()){
            port.clearError();
            port.close();
        }
        qDebug()<<"res err w end";
        break;
    case QSerialPort::UnknownError:
        qDebug()<<"error Unknown";
        //if(port.isOpen()) port.close();
        break;
    default: break;
    }
    plcConStatus=UnknownError; //на любую ошибку можно смело выставлять статус не известно, т.к. если соединение есть, то ближайший ответ от контроллера изменит состояние на Ok.
    emit msg(port.portName()+" : "+ QString::number(error) + " " + PORTERRORS[error]);
}



