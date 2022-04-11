#ifndef REGISTERIOINTERFACE_H
#define REGISTERIOINTERFACE_H

#include <QObject>
#include "viewelement.h"
#include "plcregister.h"

class RegisterIOInterface: public QObject
{
    Q_OBJECT
public:
    explicit RegisterIOInterface(QObject *parent=nullptr):QObject(parent){};
    virtual ~RegisterIOInterface(){};

public slots:
    virtual void updateRegisterValue(const PlcRegister &reg, int val)=0;

signals:
    void registerValueUpdated(const PlcRegister &reg, int val);
};

#endif // REGISTERIOINTERFACE_H
