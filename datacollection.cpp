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

#include "datacollection.h"
#include <qdebug.h>

/* конструктор */

DataCollection::DataCollection(QObject *parent) :
    QObject(parent)
{
}

DataCollection::~DataCollection()
{
}

int DataCollection::getValue(PlcRegister key)
{
    return collection.value(key);
}

/* описание методов
 * коллекции регистров */

QList<PlcRegister> DataCollection::getKeyList()
{
    QList<PlcRegister> list = collection.keys();
    qSort(list);
    return list;
}

void DataCollection::updateRegisterValues(QMap<PlcRegister, int> values)
{
    foreach(PlcRegister key, values.keys()){
        if(key.isValid()&& !writeSet.contains(key)){
            if(readOnceSet.contains(key))readOnceSet.remove(key);
            collection.insert(key, values.value(key));
            updatedSet.insert(key);
        }
    }
    emit DataReadingCompleted(); // возможно следует активацию сигнала сделать отдельно, в случае если например данные не за 1 раз вызов впихиваются
}

// множество обновлённых регистров

QList<PlcRegister> DataCollection::takeUpdatedList()
{
    QList<PlcRegister> list;
    // !!! пробую сделать так, чтобы представление получало обновленные данные модели сразу, а не после синхронизации с ПЛК !!!
    // !!! для этого вместо "-" пославил "+". Посмотрим, не приведет ли это к багам (например сбоям в синхронизации)
    if(fastSync)
        list = (updatedSet+writeSet).toList();
    else
        list = (updatedSet-writeSet).toList();
    updatedSet.clear();
    qSort(list); // чтобы дебажить было удобнее
    return list;
}

/* описание методов касающихся
 * множества регистров
 * для однократного обновления */

void DataCollection::addToReadOnceList(const QList<PlcRegister> &keys)
{
    QList<PlcRegister> validKeys;
    foreach(PlcRegister key, keys){
        if(key.isValid()) validKeys<<key;
    }
    readOnceSet.unite(validKeys.toSet() - writeSet);
    // readSet вычитать отсюда нельзя! т.к. в любой момент может смениться набор читаемых регистров,
    // и актуальные значения просто не будут получены. Рассчитывать на то, что актуальные значения
    // к этому моменту уже получены, раз эти регистры есть в readSet тоже нельзя.
}

int DataCollection::getReadOnceCount()
{
    return readOnceSet.count();
}

/* описание методов касающихся
 * множества регистров
 * для постоянных обновлений */

void DataCollection::addToReadList(const QList<PlcRegister> &keys)
{
    QList<PlcRegister> validKeys;
    foreach(PlcRegister key, keys){
        if(key.isValid()) validKeys<<key;
    }
    readSet.unite(validKeys.toSet());
}

QList<PlcRegister> DataCollection::getReadList()
{
    QList<PlcRegister> list = (readSet+readOnceSet-writeSet).toList();
    qSort(list);
    return list;
}

void DataCollection::clearReadList()
{
    readSet.clear();
}

/* описание методов работы
 * с множеством регистров
 * новые значения которых нужно передать ПЛК  */

void DataCollection::submitRegisterValues(const QMap<PlcRegister, int> &values)
{
    foreach(PlcRegister key, values.keys()){
        if(key.isValid()){
            collection.insert(key,values.value(key));
            writeSet.insert(key);
        }
    }

    // !!! пробую сделать так, чтобы представление получало обновленные данные модели сразу, а не после синхронизации с ПЛК !!!
    // добавил вызов сигнала обновления данных...    посмотрим, не приведет ли это к багам (например сбоям в синхронизации)
    if(fastSync) emit DataReadingCompleted();
}

void DataCollection::submitRegisterValue(const PlcRegister &key, int val)
{
    if(key.isValid()){
        collection.insert(key,val);
        writeSet.insert(key);
    }

    // !!! пробую сделать так, чтобы представление получало обновленные данные модели сразу, а не после синхронизации с ПЛК !!!
    // добавил вызов сигнала обновления данных...    посмотрим, не приведет ли это к багам (например сбоям в синхронизации)
    if(fastSync) emit DataReadingCompleted();
}

QList<PlcRegister> DataCollection::takeWriteList()
{
    QList<PlcRegister> list = writeSet.toList();
    writeSet.clear();
    return list;
}

QList<PlcRegister> DataCollection::getWriteList()
{
    return writeSet.toList();
}

void DataCollection::addToWriteList(const QList<PlcRegister> &keys)
{
    if(readOnceSet.count())readOnceSet.subtract(keys.toSet());
    writeSet.unite(keys.toSet());
}

int DataCollection::getWriteCount(void)
{
    return writeSet.count();
}

void DataCollection::debugInfo()
{
    qDebug()<<updatedSet.count() << " " << writeSet.count() << " " << readOnceSet.count() << " " << readSet.count() << " " << collection.count();
}
