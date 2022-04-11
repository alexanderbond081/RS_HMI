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

#ifndef POWEROFF_H
#define POWEROFF_H

#include <QObject>
#include "viewelement.h"

class PowerOff : public ViewElement
{
    Q_OBJECT

public:
    PowerOff(PlcRegister plcreg, int value, int delay_sec, QString message, QWidget *parent=nullptr);
    ~PowerOff();

private:
    int trigger_value;
    int delay;
    int counter; // тут будет обратный отсчет
    QString msg;

    QTimer count_timer; // будет тикать раз в секунду

    QWidget *msg_window;
    QLabel *msg_text;

    void view(); // получение новых данных

private slots:
    void on_timer_tick();

};

#endif // POWEROFF_H
