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

#ifndef PLCREGISTERVALIDATOR_H
#define PLCREGISTERVALIDATOR_H

#include <qstring.h>
#include <qobject.h>

/* интерфейс для передачи правил валидации классу PLCRegister */

class PLCRegisterValidator: public QObject
{
    Q_OBJECT

public:
    explicit PLCRegisterValidator(QObject *parent=nullptr):QObject(parent){}
    virtual ~PLCRegisterValidator(){}

    virtual void parsestring(const QString& _regstr, int& _regtype, int& _index)=0;
    virtual void makestring(int _regtype, int _index, QString& _regstr)=0;
    virtual int valuelength(int _regtype)=0;

};

#endif // PLCREGISTERVALIDATOR_H
