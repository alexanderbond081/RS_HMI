#include "plccontrollerrs485.h"


const QString PlcControllerRs485::PORTERRORS[14]={"NoError", "DeviceNotFoundError", "PermissionError", "OpenError", "ParityError",
                          "FramingError", "BreakConditionError", "WriteError", "ReadError", "ResourceError",
                          "UnsupportedOperationError", "UnknownError", "TimeoutError", "NotOpenError"};
const QString PlcControllerRs485::BAUDRATES[5]={"9600","19200","38400","57600","115200"};
const QString PlcControllerRs485::DATABITS[4]={"5","6","7","8"};
const QString PlcControllerRs485::PARITY[3]={"No","Even","Odd"};
const QString PlcControllerRs485::STOPBITS[3]={"1","2","1.5"};
//const QString PLCControllerRS485::flowControl[3]={"NoFlowControl", "HardwareControl", "SoftwareControl"};
//const QString PLCControllerRS485::pinounts[2]={"noPinount","noPinouts!"};


PlcControllerRs485::PlcControllerRs485(QObject *parent): QObject(parent)
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

PlcControllerRs485::~PlcControllerRs485()
{
    port.close();
    portErrList.clear();
    baudList.clear();
    bitsList.clear();
    parityList.clear();
    stopList.clear();
}

QList<QSerialPortInfo> PlcControllerRs485::getPortList(void)
{
  /*  QStringList lst;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        lst << info.portName();
    }
    return lst; */
    return QSerialPortInfo::availablePorts();
}

void PlcControllerRs485::setPortBaud(const QString &baud)
{
    port.setBaudRate(baud.toInt()); // это не совсем то что надо, но должно сработать
    //emit msg(QString::number(baud.toInt()));
}

void PlcControllerRs485::setPortBits(const QString &bits)
{
    QSerialPort::DataBits d;
    int n = bitsList.indexOf(bits);
    switch(n){
    case 0: d=QSerialPort::Data5;
    case 1: d=QSerialPort::Data6;
    case 2: d=QSerialPort::Data7;
    case 3: d=QSerialPort::Data8;
    default: d=QSerialPort::Data8;
    }
    port.setDataBits(d);
    //emit msg(DATABITS[n]);
}

void PlcControllerRs485::setPortParity(const QString &parity)
{
    QSerialPort::Parity d;
    int n = parityList.indexOf(parity);
    switch(n){
    case 0: d=QSerialPort::NoParity;
    case 1: d=QSerialPort::EvenParity;
    case 2: d=QSerialPort::OddParity;
    default: d=QSerialPort::EvenParity;
    }
    port.setParity(d);
    //emit msg(PARITY[n]);
}

void PlcControllerRs485::setPortStopBits(const QString &stop)
{
    QSerialPort::StopBits d;
    int n = stopList.indexOf(stop);
    switch(n){
    case 0: d=QSerialPort::OneStop;
    case 1: d=QSerialPort::TwoStop;
    case 2: d=QSerialPort::OneAndHalfStop;
    default: d=QSerialPort::OneStop;
    }
    port.setStopBits(d);
    //emit msg(STOPBITS[n]);
}

void PlcControllerRs485::setPortName(const QString &portname)
{
    port.setPortName(portname.left(portname.indexOf("\t")));
}

void PlcControllerRs485::setPortParameters(const QString &name, const QString &baud, const QString &bits, const QString &parity, const QString &stop)
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

bool PlcControllerRs485::openPort()
{
    bool result=1;
    //if(!port.isOpen()){ - после тестов вернуть?
        result=port.open(QIODevice::ReadWrite);
    //}
    return result;
}

void PlcControllerRs485::closePort()
{
    //if(port.isOpen()) - после тестов вернуть?
    port.close();
}

bool PlcControllerRs485::portIsOpen()
{
    return port.isOpen();
}

void PlcControllerRs485::setRequestDelay(int delay)
{
    requestDelay = delay;
}

void PlcControllerRs485::setRequestTimeout(int timeout)
{
    requestTimeout = timeout;
}

void PlcControllerRs485::portError(QSerialPort::SerialPortError error)
{
    switch(error){
    case QSerialPort::NoError:
    case QSerialPort::OpenError:
        break;

    case QSerialPort::ParityError:
    case QSerialPort::FramingError:
    case QSerialPort::WriteError:
    case QSerialPort::ReadError:
    case QSerialPort::TimeoutError:
    case QSerialPort::UnsupportedOperationError:
        // пока не знаю что делать в такой ситуации
        // думаю стоит добавить периодически обнуляющийся счётчик,
        // при достижении определённого значения будет происходить закрытие порта
        break;

    case QSerialPort::NotOpenError:
        break;

    case QSerialPort::PermissionError:
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::BreakConditionError:
    case QSerialPort::ResourceError:
    case QSerialPort::UnknownError:
        if(port.isOpen()) port.close();
        break;
    default: break;
    }
    emit msg(port.portName()+" : "+PORTERRORS[error]);
}



