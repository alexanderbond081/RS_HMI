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

#ifndef PLCREGISTER_H
#define PLCREGISTER_H

#include <QString>
#include <QPointer>
#include "plcregistervalidator.h"

/*
    Класс регистров различных ПЛК (для связи по MODBUS RTU)
    Его нельзя делать абстрактным, т.к. возможно понадобится испольовать его в качестве ключа в коллекциях.
    В его задачи входит:
    - приводить в понятный для контроллера вид адрес регистра (проверка на валидность) в момент создания,
    - обеспечивать копироване, сравнение регистров, ставнение с QString
    - сложение индекса регистра и целого числа,
    - автоматическое приведение регистра к QString и наоборот.

    Сложность хранения состояния регистра контроллера внутри класса PLCRegister состоит в том, что мы должны иметь
    ОДИН набор состояний, а хранение внутри класса размножит кол-во наборов до неконтролируемого.

    Проверка на валидность для произвольно типа контроллера можно обеспечить если:
    - внутри класса хранить делегата отдельного класса, предназначенного для проверки на валидность
    - вывести единобразный метод проверки и сделать набор шаблонов, определяемых внутри ... например fatekcommrs
*/

/*  что делать если понадобится переинициализировать связь с контроллером, а сделать это можно будет только полностью
    уничтожив объект класса fatekcommrs который собственно и является валидатором?
    стоит ли делать отдельный класс для валиации и сделать его синглтоновским? синлтон не пойдёт т.к. их может
    понадобиться несколько разных.
    можно кстати! инициализировать абстрактный валидатор внутренним методом класса контроллера. Один раз после создания.
    так программе и не нужно будет знать какой же валидатор использовать для данного контроллера, но придётся
    руками контролировать процесс создания и уничтожения.
 */

class PlcRegister
{
public:
    PlcRegister(PLCRegisterValidator *registervalidator=nullptr);
    PlcRegister(const QString &reg,PLCRegisterValidator *registervalidator=nullptr);
    PlcRegister(const PlcRegister &reg);
    virtual ~PlcRegister(); // деструктор не нужен, т.к. нет выделения памяти.

    static void setDefaultValidator(PLCRegisterValidator *defaultValidator);

    PlcRegister operator+(int) const;
    PlcRegister operator-(int) const;
    PlcRegister &operator =(const PlcRegister&);
    PlcRegister &operator =(const QString&);

    // можно было бы добавить ещё дружественный оператор перевода PLCRegistr в QString friend bool operator = (QStrging, const &plcRegister);
    friend bool operator <(const PlcRegister &reg1,const PlcRegister &reg2);
    friend bool operator >(const PlcRegister &reg1, const PlcRegister &reg2);
    friend bool operator==(const PlcRegister &reg1, const PlcRegister &reg2);
    friend bool operator==(const PlcRegister &reg, const QString str);
    friend uint qHash(const PlcRegister &key, uint seed);

    const QString &toString() const;

    inline bool isValid()const {return regtype;}
    inline int getType()const {return regtype;}
    inline int getIndex()const {return index;}
    int getValueLength();

private:
    static QPointer<PLCRegisterValidator> default_validator;
    QPointer<PLCRegisterValidator> validator;
    int index;
    int regtype;
    QString regstring;

    //QString& getregstring(void);
    //void parsestring(const QString &reg);
};

#endif // PLCREGISTER_H
