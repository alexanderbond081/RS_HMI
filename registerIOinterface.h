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
