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

#include <qregexp.h>
#include <QtMath>
#include <QtCore>
//#include <QPointer>
#include "plcregister.h"

QPointer<PLCRegisterValidator> PlcRegister::default_validator=0;

PlcRegister::PlcRegister(PLCRegisterValidator *registervalidator)
{
    if(registervalidator)validator=registervalidator;
    else validator=default_validator;

    index=0;
    regtype=0;
    //regstring.clear();
}

PlcRegister::PlcRegister(const QString &reg, PLCRegisterValidator *registervalidator)
{
    if(registervalidator)validator=registervalidator;
    else validator=default_validator;

    //regstring.clear();

    if(validator){
        validator->parsestring(reg,regtype,index);
        validator->makestring(regtype,index,regstring); // если есть возможность лучше создавать корректный вид строки сразу.
    }
    else {
        index=0;
        regtype=0;
    }
}

PlcRegister::PlcRegister(const PlcRegister &reg)
{
    validator = reg.validator;
    index = reg.index;
    regtype = reg.regtype;
    regstring = reg.regstring;
}

PlcRegister::~PlcRegister()
{

}

void PlcRegister::setDefaultValidator(PLCRegisterValidator *defaultValidator)
{
    default_validator = defaultValidator;
}

PlcRegister PlcRegister::operator +(int i) const
{
    PlcRegister reg(validator);
    if(regtype)
    {
        reg.regtype = regtype;
        reg.index = index+i;
        if(reg.index<0)reg.index=0; // надо бы ещё проверку на переполнение добавить
        if(validator) validator->makestring(reg.regtype,reg.index,reg.regstring);
    }
    return reg;
}

PlcRegister PlcRegister::operator -(int i) const
{
    PlcRegister reg(validator);
    if(regtype)
    {
        reg.regtype = regtype;
        reg.index = index-i;
        if(reg.index<0)reg.index=0; // надо бы ещё проверку на переполнение добавить
        if(validator) validator->makestring(reg.regtype,reg.index,reg.regstring);
    }
    return reg;
}

PlcRegister &PlcRegister::operator =(const PlcRegister &reg)
{
    if(this!=&reg){
        validator = reg.validator;
        index = reg.index;
        regtype = reg.regtype;
        regstring = reg.regstring;
    }
    return *this;
}

PlcRegister &PlcRegister::operator =(const QString &str)
{
    if(validator) validator->parsestring(str,regtype,index);
    return *this;
}

bool operator <(const PlcRegister &reg1,const PlcRegister &reg2)
{
    return (reg1.regtype<reg2.regtype)||((reg1.regtype==reg2.regtype)&&(reg1.index<reg2.index));
}

bool operator >(const PlcRegister &reg1,const PlcRegister &reg2)
{
    return (reg1.regtype>reg2.regtype)||((reg1.regtype==reg2.regtype)&&(reg1.index>reg2.index));
}

bool operator ==(const PlcRegister &reg1,const PlcRegister &reg2)
{
    return ((reg1.index==reg2.index)&&(reg1.regtype==reg2.regtype));
}

bool operator ==(const PlcRegister &reg,const QString &str)
{
    return reg == PlcRegister(str);
    // reg.toString() == str;
}

const QString &PlcRegister::toString() const
{
    return regstring;
}

int PlcRegister::getValueLength()
{
    if((!regtype)||(!validator)) return 0;
    else return validator->valuelength(regtype);
}

uint qHash(const PlcRegister &key, uint seed=0)
{
    return qHash((key.index<<8)^key.regtype, seed); // эта функция нужна, чтобы QSet смог хранить объекты нашего класса
}
