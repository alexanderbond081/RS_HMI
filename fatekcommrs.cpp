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

#include "fatekcommrs.h"
#include "datacollection.h"

#include <QObject>

/*
    типы запросов:
    40 The gist read the system status of PLC −
    41 Control RUN/STOP of PLC −
    42 Single discrete control [1 point]
    43 The status reading of ENABLE/DISABLE of continuous discrete [1~256 points]
    44 The status reading of continuous discrete [1~256 points]
    45 Write the status to continuous discrete [1~256 points]
    46 Read the data from continuous registers [1~64 Words]
    47 Write to continuous registers [1~64 Words]
    48 Mixed read the random discrete status of register data [1~64 points or Words]
    49 Mixed write the random discrete status of register data [1~32 points or Words]
    4E Loop back testing [0~256 characters]
    53 The detail read the system status of PLC
*/

 const char FatekCommRS::CHR[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
 const char FatekCommRS::CHR2[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
 const char FatekCommRS::START_CODE = 0x02;
 const char FatekCommRS::END_CODE = 0x03;
 const QString FatekCommRS::PLCERRORS[16] = {"NoError", "unknown", "IllegalValue", "unknown",
                                             "IlligalFormat", "Can'tRun:LadderChecksum", "Can'tRun:PLC",
                                             "Can'tRun:SyntaxErr", "Can'tRun:FunctionUnsupported",
                                             "IlligalAddress", "unknown", "unknown", "unknown", "unknown",
                                             "unknown"};
 const QString FatekCommRS::ACTUAL_DEVICE_NUM = "01";
 const QString FatekCommRS::CODE_STATUS = "40";
 const QString FatekCommRS::CODE_RUN_STOP = "41";
 const QString FatekCommRS::CODE_READ_SINGLE_DISCRETE = "42";
 const QString FatekCommRS::CODE_STATUS_READING_DISCRETE = "43";
 const QString FatekCommRS::CODE_READ_CONT_DISCRETE = "44";
 const QString FatekCommRS::CODE_WRITE_CONT_DISCRETE = "45";
 const QString FatekCommRS::CODE_READ_CONT_REGISTERS = "46";
 const QString FatekCommRS::CODE_WRITE_CONT_REGISTERS = "47";
 const QString FatekCommRS::CODE_READ_RANDOM = "48";
 const QString FatekCommRS::CODE_WRITE_RANDOM = "49";
 const QString FatekCommRS::CODE_LOOPBACK_TEST = "4E";
 const QString FatekCommRS::CODE_DETAILED_STATUS = "53";


FatekCommRS::FatekCommRS(QObject *parent): PlcCommRS485(parent)
{
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerTick()));
}

FatekCommRS::~FatekCommRS()
{
}

PLCRegisterValidator *FatekCommRS::makeValidator()
{
    return new FatekRegisterValidator();
}

void FatekCommRS::setDataCollection(DataCollection *data)
{
    dataCollection = data;
}

void FatekCommRS::communicationStart()
{
    timer.start(requestDelay/2);
    lastRecieveTime=QDateTime::currentDateTime(); // эту переменную необходимо инициализировать. иначе обмен может повиснуть не начавшись.
}

void FatekCommRS::communicationStop()
{
    timer.stop();
}

bool FatekCommRS::communicationIsOn()
{
    return timer.isActive();
}

void FatekCommRS::askSomeDebugInfo()
{
    dataCollection->debugInfo();
}

void FatekCommRS::sendnext()
{
    // надо всё-таки разделить коктель из приёма и передачи, не смотря на то, что технически они почти не отличаются.
    // потому что сразу сложно понять что конкретно выполняется в одной фазе и другой.

    // тут проверяем удачные/неудачные запросы
    // 3 неудачи передачи регистров и не переданные регистры надо вернуть обратно в dataCollection
    if(requestedRegisters.count()>0){
        requestsCount++;
        if(requestsCount>2){
            if(writeRegistersPhase)dataCollection->addToWriteList(requestedRegisters);
            requestedRegisters.clear();
            requestsCount=0;
        }
    }

    // логика определения фазы (чтение передача регистров)
    if(requestedRegisters.count()==0){ // если это не перезапрос, то формируем набор регистров заново
        writeRegistersPhase = (writeRegistersPhase && registersToRequestList.count()) || // это условие означает продолжение передачи контроллеру регистров для изменения
                (!writeRegistersPhase && !registersToRequestList.count() && dataCollection->getWriteCount()); // а это - начало передачи, при условии, что есть что передавать


        if(registersToRequestList.count()==0){
            if(writeRegistersPhase) registersToRequestList = dataCollection->takeWriteList();
                else registersToRequestList = dataCollection->getReadList();
            }

        int c,i=0;
        if(writeRegistersPhase)c=32;else c=64;
        while((i<c) && (!registersToRequestList.isEmpty())){
            PlcRegister reg = registersToRequestList.takeFirst();
            if(reg.isValid()){ // для оптимизации скорости это можно было бы не делать, но так надёжнее.
                requestedRegisters << reg;
                i++;
            }
        }
    }

    QString str; // для дебага
    if(requestedRegisters.count()) // на всякий случай. Вдруг какая-то из страниц не будет содержать вообще ничего.
    {
        writeRegistersPhase ? packMsg3(requestedRegisters) : packMsg2(requestedRegisters); // эта функция просто формирует посылку в writeBuffer
        quint64 n=0;
        qint64 c;
        while(n<(quint64)writeBufCount){
            c=port.write(writeBuffer+n,writeBufCount-n); // данные за один раз могут и не уйти
            n+=c;
            if(c==-1){
                portError(QSerialPort::UnknownError); // может ли эта ошибка возникать в случаях кроме неподключенного порта? QSerialPort::NotOpenError
                break;
            }
        }

        waitingforresponse=true;
        lastRequestTime=QDateTime::currentDateTime(); // может лучше использовать милисекунды?
        str=QString::fromLatin1(writeBuffer, writeBufCount); // дебаг-индикация обмена данными
    }
    else str="no data for request";

    emit msg(str); // дебаг или индикация обмена данными
}

void FatekCommRS::readData()
{
    /* эта функция собирает куски посылки в один буфер и, по приходу символа конца посылки, отправляет на расшифровку */
    QByteArray data = port.readAll();

    bool complete=false;
    foreach (char ch, data) {
        if(ch==START_CODE)readBufCount=0;
        if(readBufCount<sizeof(readBuffer))readBuffer[readBufCount++] = ch;
        if((ch==END_CODE)&&(readBufCount>=6)) complete=true;
    }

    // !!! анализ исправности посылки делает функция unpackMsg(), но возвращает только bool.
    // стоит ли делать более детальный анализ именно в ЭТОЙ функции?

    if(complete){
        plcConStatus=Ok;

        QString str = QString::fromLatin1(readBuffer, readBufCount) + "  LRC check: "+ QString::number(checkReadBufLRC(),16);
        emit msg(str);

        // регистры значения которых к этому моменту уже изменены вьюшкой
        // должен фильтровать DataCollection, как и в случае формирования посылки
        // вызов дешифратора ответа
        if(unpackMsg()){
            if(writeRegistersPhase){
                //dataCollection->removeFromWriteList(requestedRegisters);
            }
            else{
                if(requestedRegisters.count()==recievedRegValues.count()){
                    QMap<PlcRegister,int> newdata;
                    for(int i=0;i<recievedRegValues.count();i++){
                        PlcRegister reg = requestedRegisters.at(i);
                        int val = recievedRegValues.at(i);
                        newdata.insert(reg,val);
                    }
                    dataCollection->updateRegisterValues(newdata);
                }
            }
            requestedRegisters.clear();
            recievedRegValues.clear();
        }

        // между запросом и ответом может проскочить мусор,
        // поэтому статус ожидания должен сниматься только завершенной посылкой.
        // если посылка разбилась на 2е части и в конце первой случайно пришел байт завершения,
        // то за 100мсек задержки на перезапрос остальная часть успеет прити в любом случае.
        waitingforresponse=false;
    }
    lastRecieveTime=QDateTime::currentDateTime();
}

void FatekCommRS::onTimerTick()
{
    /* определяем этап обмена данными
       и выбираем соответствующее действие */

    static int timeouts=0;

    int msecs = lastRequestTime.msecsTo(QDateTime::currentDateTime());
    bool timeout =  waitingforresponse && (msecs > requestTimeout);
    bool ready = !waitingforresponse && (msecs > requestDelay);

    if(port.isOpen()){
        if(timeout){
            plcConStatus=NoConnection;
            timeouts++;
        }
        if(timeouts>timeoutsToReconnect){
            port.close();
            //noConnectionTime=QDateTime::currentDateTime();
            timeouts=0;
            lastRequestTime=QDateTime::currentDateTime();
            //qDebug()<<QTime::currentTime().toString() << " - Try to reconnect port";
        }
        else if(timeout||ready) sendnext();
    }
    else if(msecs>reopenPortDelay){
        openPort();
        timeouts=0;
        lastRequestTime=QDateTime::currentDateTime();
    }

    // надо бы после очередного тайм-аута объявить о недоступности контроллера и сделать переподключение порта.
    // придумать как осуществлять диагностику и отображение состояния связи с контроллером.
}

void FatekCommRS::simulateRegUpdate(QString reg, QString val)
{
    QMap<PlcRegister,int> mp;
    mp.insert(PlcRegister(reg),val.toInt());
    dataCollection->updateRegisterValues(mp);
}

QString FatekCommRS::packMsg(const QList<PlcRegister> &regList)
{
/* так будет выглядеть вариант формирования запроса в QString */
    QString str;
    str+=START_CODE; // стартовый
    str+="01"; // номер ПЛК в сети RS485 - !!! хардкод. Пока не возникла задача общения с несколькими контроллерами.
    str+="48"; // запрос регистров в произвольной последовательности
    str+=QString("0%1").arg(regList.count(),0,16).rightRef(2); // перевод HEX числа в строку. (rightRef заработает???)
    foreach(PlcRegister reg, regList) str+=reg.toString();
    // подсчёт LRC
    int LRC=0;
    for(int i=0;i<str.count();i++) LRC+=str.at(i).toLatin1();
    str+=QString("0%1").arg(LRC&0xFF,0,16).rightRef(2);
    str+=END_CODE; // стоповый байт

    return str;
}

void FatekCommRS::packMsg2(const QList<PlcRegister> &regList)
{
/* так будет выглядеть вариант формирования запроса в массив символов  */
    clearWriteBuffer();
    addToWriteBuffer(START_CODE);
    addToWriteBuffer(ACTUAL_DEVICE_NUM);
        requestedDeviceNumber=ACTUAL_DEVICE_NUM; // для сверки данных, которые будут приняты.
    addToWriteBuffer(CODE_READ_RANDOM);
        requestedCommandCode=CODE_READ_RANDOM; // та же фигня.
    addToWriteBuffer(regList.count(),2); // перевод HEX числа в строку.
    requestedDataLength=0;
    foreach(PlcRegister reg, regList){
        addToWriteBuffer(reg.toString());
        // на каждый регистр надо добавить соотв. ожидаемое в ответе кол-во байт
        requestedDataLength+=reg.getValueLength();
    }
    addLRCToWriteBuffer();
    addToWriteBuffer(END_CODE); // стоповый байт
}

void FatekCommRS::packMsg3(const QList<PlcRegister> &regList)
{
/* так будет выглядеть вариант формирования запроса в массив символов  */
    clearWriteBuffer();
    addToWriteBuffer(START_CODE);
    addToWriteBuffer(ACTUAL_DEVICE_NUM);
        requestedDeviceNumber=ACTUAL_DEVICE_NUM; // для сверки данных, которые будут приняты.
    addToWriteBuffer(CODE_WRITE_RANDOM);
        requestedCommandCode=CODE_WRITE_RANDOM; // та же фигня.
    addToWriteBuffer(regList.count(),2); // перевод HEX числа в строку.
    requestedDataLength=0;
    foreach(PlcRegister reg, regList){
        addToWriteBuffer(reg.toString());
        addToWriteBuffer(dataCollection->getValue(reg), reg.getValueLength());
        //qDebug()<<reg.toString()<<" - "<<reg.getValueLength();
    }
    addLRCToWriteBuffer();
    addToWriteBuffer(END_CODE); // стоповый байт
}

bool FatekCommRS::unpackMsg()
{   //  в этой функции происходит расшифровка сообщения с занесением данных в recievedRegisterValues

    /*  распаковка сообщения:
        1. Проверяем номер устройства.
        2. Проверяем LRC
        3. Проверям код ошибки
        4. Проверяем код косылки
        5. после всего этого уже расшифровываем данные.

        ??? проверяется ли длина посылки? LRC может совпасть, слишком ненадежен сам по себе.
    */
    if(!checkReadBufLRC()){ emit msg("wrong LRC"); return false; }
    if(!checkReadBufDeviceNum(requestedDeviceNumber)){ emit msg("wrong PLC address"); return false; }
    if(!checkReadBufCommandCode(requestedCommandCode)){ emit msg("wrong command"); return false; }
    if(!checkReadBufLength()){emit msg("wrong data length"); return false; }
    int k;
    if((k=getReadBufErrorCode()) > '0'){ emit msg("PLC communication error code: " + PLCERRORS[charToInt(k)&0xF]); return false; }

    // далее уже совершаем расшифровку эта расшифровка соответствует запросу рандомного списка регистров
    recievedRegValues.clear();
    if(requestedCommandCode==CODE_READ_RANDOM) // пока только такой режим предусматриваем
        foreach(PlcRegister reg, requestedRegisters){
            int c=reg.getValueLength();
            switch(c){
            case 4: recievedRegValues << (qint16)getNextNCharData(c); break;
            case 8: recievedRegValues << (qint32)getNextNCharData(c); break;
            default:recievedRegValues << getNextNCharData(c);
            }
        }
    return true;
}

// ====================================
// реализация FatekRegisterValidator
// наследованый от PLCRegisterValidator
// ====================================

FatekRegisterValidator::FatekRegisterValidator(QObject *parent): PLCRegisterValidator(parent)
{

}

FatekRegisterValidator::~FatekRegisterValidator()
{

}

void FatekRegisterValidator::makestring(int _regtype, int _index, QString &_regstr)
{
    // формируется правильная запись регистра в виде строки.
    // в случае некорректного типа регистра (regtype=0), ничего не происходит.
    if(_regtype){
        int indextemp=_index;
        int indexcharscount=REGISTER_TYPES[_regtype].count();
        int regcharscount=REGISTER_CHARS[_regtype];
        _regstr.fill(QChar('0'), regcharscount);
        QChar *regstrqchar=_regstr.data(); // для увелилчения скорости доступа к отдельным символам
        for(int i=regcharscount-1;i>=indexcharscount;i--){
            regstrqchar[i] = QChar((indextemp % 10) + 48);
            indextemp = indextemp / 10;
        }
        for(int i=0;i<indexcharscount;i++)regstrqchar[i]=REGISTER_TYPES[_regtype].data()[i];
    }
}

void FatekRegisterValidator::parsestring(const QString &_regstr, int &_regtype, int &_index)
{
    const QChar *chrs = _regstr.data();
    int n=0;
    int regcount=_regstr.count();
    while((n<regcount)&&((chrs[n]<'0')||(chrs[n]>'9')))n++;

    QString newregpref=_regstr.left(n).toUpper();

    // переводим текстовый индекс в числовой
    // эта оптимизация выполняется приблизительно в 10 раз быстрее по сравнению с:
    // index = reg.right(regcount-n).toInt();
    // но даст некоторый сбой если в записи числа ошибка
    _index = 0;
    for(int i=n;i<regcount;i++){
        int c=(chrs[i].toLatin1()-48);
        if((c>9)||(c<0))break;
        _index*=10;
        _index+=c;
    }
    // поиск типа регистра. Тут оптимизировать ничего не удалось.
    _regtype=16;
    while((_regtype>0)&&(newregpref!=REGISTER_TYPES[_regtype]))_regtype--;
}

int FatekRegisterValidator::valuelength(int _regtype)
{
    return REGISTER_BYTES[_regtype];
}
