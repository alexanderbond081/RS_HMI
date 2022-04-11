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

#ifndef DATACOLLECTION_H
#define DATACOLLECTION_H

#include <QObject>
#include <QMap>
#include <QSet>
#include "plcregister.h"

/*  Коллекция значений регистров также осуществляет следующие действия:

    1). в случае получания новых данных от контроллера составляется список регистров
    для обновления интерфейса и посылается сигнал DataUpdated.

    2). в случае изменения данных по инициативе интерфейса, составляется список регистров
    для изменения их состояния в контроллере и посылается сигнал DataChanged.

    3). чтобы не получилось накладки, принятые от контроллера данные фильтруются от регистров
    которые также имеются в списке для изменения...

    4). для упрощения работы, списки регистров сделаны в виде QSet.
*/

class BasicRegister
{
public:
    BasicRegister() {}

protected:
    bool readonly; // на всякий случай, хотя вообще это можно на уровне интерфейса ограничить
    bool changed; // если значение были изменено пользователем, то его не надо читать из ПЛК, а сперва записать туда.
    int value;
};

class DataCollection : public QObject
{
    Q_OBJECT

public:

    explicit DataCollection(QObject *parent = 0);
    virtual ~DataCollection();

    // методы работы со значениями регистров
    int getValue(PlcRegister key);
    QList<PlcRegister> getKeyList(void);

    void updateRegisterValues(QMap<PlcRegister, int> values); // посылает сигнал обновления вьюшки
    void submitRegisterValues(const QMap<PlcRegister, int> &values); // добавляет регистры в список отправки
    void submitRegisterValue(const PlcRegister &key, int val); // добавляет регистр в список отправки

    // множество обновлённых
    QList<PlcRegister> takeUpdatedList(void);

    // методы работы с множеством для одноразовых обновлений
    void addToReadOnceList(const QList<PlcRegister> &keys);
    int getReadOnceCount(void);

    // методы работы с множеством для постоянных обновлений
    void addToReadList(const QList<PlcRegister> &keys);
    QList<PlcRegister> getReadList(void);
    void clearReadList(void);

    // методы работы с множеством установки новых значений регистров
    QList<PlcRegister> takeWriteList(void);
    QList<PlcRegister> getWriteList(void);
    void addToWriteList(const QList<PlcRegister> &keys);
    int getWriteCount(void);

    void debugInfo();

    bool fastSync = true; // настройка для быстрой синхронизации представления с моделью, не дожидаясь синхронизации с ПЛК

signals:
    void DataReadingCompleted(void); // сообщение для представления

public slots:

private:
    QSet<PlcRegister> updatedSet; // множество прочитанных регистров

    QSet<PlcRegister> writeSet; // множество регистров для записи
    QSet<PlcRegister> readOnceSet; // множество регистров для однократного обновления
    QSet<PlcRegister> readSet; // множество регистров для регулярного обновления

    QMap<PlcRegister, int> collection; // список всех регистров и их значения
};

#endif // DATACOLLECTION_H
