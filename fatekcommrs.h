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

#ifndef FATEKCOMMRS_H
#define FATEKCOMMRS_H

/*  Класс для коммуникации с контроллером fatek */

#include <QObject>
#include <QtSerialPort/QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "datacollection.h"
#include "plccommrs485.h"
#include "plcregister.h"

#define COUNT(a) sizeof(a)/sizeof(a[0])

class FatekRegisterValidator: public PLCRegisterValidator
{
    Q_OBJECT

public:
    explicit FatekRegisterValidator(QObject *parent=0);
    virtual ~FatekRegisterValidator();

    virtual void parsestring(const QString& _regstr, int& _regtype, int& _index);
    virtual void makestring(int _regtype, int _index, QString& _regstr);
    virtual int valuelength(int _regtype);

private:
    const QString REGISTER_TYPES[17] = {"0-9","X","Y","M","S","T","C","RT","RC","R","D","F","DRT","DRC","DR","DD","DF"};
    const int     REGISTER_CHARS[17] = { 0,    5,  5,  5,  5,  5,  5,  6,   6,   6,  6,  6,  7,    7,    7,   7,   7  };
    const int     REGISTER_BYTES[17] = { 0,    1,  1,  1,  1,  1,  1,  4,   4,   4,  4,  4,  8,    8,    8,   8,   8  };

};

class FatekCommRS: public PlcCommRS485
{
    Q_OBJECT

public:
    explicit FatekCommRS(QObject *parent=0);
    virtual ~FatekCommRS();
    QList<QSerialPortInfo> getPortList(void);

    virtual void askSomeDebugInfo();

    virtual void sendnext(); // следующий шаг обмена данными
    virtual void communicationStart(); // запуск цикла обмена данными
    virtual void communicationStop(); // остановка
    virtual bool communicationIsOn();

    virtual PLCRegisterValidator* makeValidator(void); // создаёт валидатор, который потом программа
                                              // будет передавать создаваемым регистрам.    
    void setDataCollection(DataCollection *data);

private slots:
    virtual void readData();
    void onTimerTick();

public slots:
    void simulateRegUpdate(QString reg, QString val); // это для дебага вьюшки при отсутствии реального контроллера.

private:
    static const char CHR[16], CHR2[16];
    static const char START_CODE, END_CODE;
    static const QString PLCERRORS[16];
    static const QString ACTUAL_DEVICE_NUM;
    static const QString CODE_STATUS, CODE_RUN_STOP;
    static const QString CODE_READ_SINGLE_DISCRETE;
    static const QString CODE_STATUS_READING_DISCRETE;
    static const QString CODE_READ_CONT_DISCRETE;
    static const QString CODE_WRITE_CONT_DISCRETE;
    static const QString CODE_READ_CONT_REGISTERS;
    static const QString CODE_WRITE_CONT_REGISTERS;
    static const QString CODE_READ_RANDOM;
    static const QString CODE_WRITE_RANDOM;
    static const QString CODE_LOOPBACK_TEST;
    static const QString CODE_DETAILED_STATUS;

    QTimer timer; // таймер реализующий периодический обмен данными
    QDateTime lastRequestTime=QDateTime::currentDateTime(); // время последней отосланного запроса контроллеру
    QDateTime lastRecieveTime=QDateTime::currentDateTime(); // время последнего полученного ответа от контроллера
    bool writeRegistersPhase=false; // этап установки регистров
    bool waitingforresponse=false; // ожидание ответа от контроллера
    int requestsCount=0; // количество повторов одной посылки

    QPointer<DataCollection> dataCollection; // устанавливается извне, т.к. её надо шарить между...

    char writeBuffer[1024], readBuffer[1024]; // защита от переполнения буфера отправки будет контролироваться на уровне формирования посылки, а не в методах addToWriteBuffer

    uint requestedDataLength=0; // ожидаемая длина данных (для доп.проверки валидности ответа)
    uint writeBufCount=0,readBufCount=0, // кол-во символов в буфере записи/чтения
        nextValPos=0; // позиция следующего значения регистра в буфере чтения (используется при расшифровке сообщения)

    QString packMsg(const QList<PlcRegister> &regList); // формируем посылку из имеющихся в хранилище данных.
    void packMsg2(const QList<PlcRegister> &regList);
    void packMsg3(const QList<PlcRegister> &regList);
    QList<PlcRegister> registersToRequestList; // список регистров для отсылки контроллеру (чтение или установка определяет submitregvaluesphase)
    QList<PlcRegister> requestedRegisters; // данные чтобы легко было распаковать потом.
    QList<int> recievedRegValues; // сюда расшифровывается ответ контроллера
    QString requestedDeviceNumber;
    QString requestedCommandCode;

    bool unpackMsg();

    /*  инлайновые функции для внутреннего потребления - необходимо добавить контроль переполнения */
    /*  буфер записи  */
    inline int charToInt(char ch){int val=ch-48; if(val>9)val-=7; if(val>15)val-=32; return val&0xF;}
    inline void addToWriteBuffer(char ch) { writeBuffer[writeBufCount++]=ch; }
    inline void addToWriteBuffer(const char* str, int count) { for(int i=0;i<count;i++) writeBuffer[writeBufCount++]=str[i]; }
    inline void addToWriteBuffer(const QString &str) { foreach(QChar ch,str) writeBuffer[writeBufCount++]=ch.toLatin1(); }
    inline void addToWriteBuffer(unsigned int n, int num) {
        for(int shft=(num-1)*4;shft>=0;shft-=4)
            writeBuffer[writeBufCount++]=CHR[(n>>shft)&0xF];
    }
    inline void addLRCToWriteBuffer(void) {
        int LRC=0;
        for(uint i=0;i<writeBufCount;i++) LRC+=writeBuffer[i];
        addToWriteBuffer(LRC,2);
    }
    inline void clearWriteBuffer(void) { writeBufCount=0; nextValPos=6;}

    /*  буфер чтения  */
    inline void clearReadBuffer(void) { readBufCount=0; }

    inline bool checkReadBufLRC(void){
        if(readBufCount<3) return 0; //
        int LRC=0;
        for(uint i=0;i<(readBufCount-3);i++) LRC+=readBuffer[i];
        return (LRC & 0xFF) == (charToInt(readBuffer[readBufCount-3])*0x10 + charToInt(readBuffer[readBufCount-2]));
    }

    inline bool checkReadBufDeviceNum(const QString &devNum) { return (devNum.at(0).toLatin1()==readBuffer[1]) && (devNum.at(1).toLatin1()==readBuffer[2]);}
    inline bool checkReadBufCommandCode(const QString &comCode) { return (comCode.at(0).toLatin1()==readBuffer[3]) && (comCode.at(1).toLatin1()==readBuffer[4]);}
    inline bool checkReadBufLength(){ /*тутачки надо как-то совершить анализ длины всяческого ответа.
                                        для этого у нас должна быть уже заготовлена ожидаемая длина данных.*/
        return (requestedDataLength==(readBufCount-9)); }
    inline int getReadBufErrorCode() { return readBuffer[5];}
    inline int getNextNCharData(uint n) {
        int val=0;
        if((readBufCount-nextValPos-2)>n){ // эта проверка нужна чтобы не выйти за рамки буфера чтения.
            for(int k=(n-1)*4;k>=0;k-=4)
                val+=charToInt(readBuffer[nextValPos++])<<k;
        }
        else nextValPos=readBufCount-3;
        return val;
    }
};

#endif // FATEKCOMMRS_H
