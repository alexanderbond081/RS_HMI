#ifndef PLCCONTROLLERRS485_H
#define PLCCONTROLLERRS485_H

/* данный класс реализует подключение порта для связи с контроллером
   и предоставляет интерфейс для работы с отправкой и приёмом данных
   сам механизм построения и отправки посылок надо реализовывать в наследниках */

#include <QObject>
#include <QList>
#include <QtSerialPort/QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "datacollection.h"
#include "plcregistervalidator.h"

class PlcControllerRs485: public QObject
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

    explicit PlcControllerRs485(QObject *parent);
    virtual ~PlcControllerRs485();

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
    virtual QString getStopBits() {
        switch(port.stopBits()){
        case 2: return STOPBITS[1];
        case 3: return STOPBITS[2];
        default: return STOPBITS[0];
    }}

    void setPortParameters(const QString &name, const QString &baud="38400", const QString &bits="8", const QString &parity="Even", const QString &stop="1");
    bool openPort();
    void closePort();
    bool portIsOpen();
    void setRequestDelay(int delay);
    void setRequestTimeout(int timeout);

    virtual void askSomeDebugInfo()=0;

    virtual void sendnext()=0; // следующий шаг обмена данными
    virtual void communicationStart()=0; // запуск цикла обмена данными
    virtual void communicationStop()=0; // остановка

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
    virtual void portError(QSerialPort::SerialPortError error);

protected:
    QSerialPort port;
    int requestDelay=100; // msec
    int requestTimeout=500; // msec
    int reopenPortDelay=1000;// msec

private:
    QStringList portErrList, baudList, bitsList, parityList, stopList;
    inline void fillStrList(QStringList &lst, const QString ARRAY[], uint n) { lst.clear(); for(uint i=0;i<n;i++) lst.append(ARRAY[i]); }

};

#endif // PLCCONTROLLERRS485_H
