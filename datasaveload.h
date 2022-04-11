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

#ifndef DATASAVELOAD_H
#define DATASAVELOAD_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "datacollection.h"
#include "plcregister.h"
#include "viewelement.h"


class RecipeStorage : public QObject
{
    Q_OBJECT
public:
    explicit RecipeStorage(const QString &name, const QList<PlcRegister> &registers, DataCollection *data, QObject *parent = nullptr);

private:
    QList<PlcRegister> regs; // список регистров. данные нам хранить не нужно.
    QPointer<DataCollection> collection; // указатель на хранилище данных.
    QString caption;
    QString path; // путь определяемый name

    QTimer savetimer;
    QTime savetimeout;
    QString savefilename;
private slots:
    void onTimer();

signals:
    void saveStart();
    void saveFinish();

public slots:
    void save();
    void load();
};


/*******************************
 *   объявление TextfileLog    *
 *******************************/

class TextfileLog: public ViewElement
{
  Q_OBJECT
public:
  explicit TextfileLog(const QString &logName, int interval,
                    const QList<PlcRegister> &logRegisters, const QList<int> &regDecimals,
                    const QList<QString> &regLabels, QWidget *parent=nullptr);

private slots:
    void onTimer();

private:
    void view(); // получение новых данных

    QString name;
    QList<PlcRegister> registers;
    QList<int> decimals;
    QList<int> dividers;
    QList<QString> labels;
    QVector<int> values; // сюда будут складываться значения регистров в методе view()

    QTimer *timer; // а этот таймер будет раз в period складывать значения в файл

};


#endif // DATASAVELOAD_H
