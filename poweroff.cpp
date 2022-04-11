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

#include "poweroff.h"
#include <unistd.h>
#include <QProcess>
#include <QScreen>
#include <QFrame>

PowerOff::PowerOff(PlcRegister plcreg, int value, int delay_sec, QString message, QWidget *parent):ViewElement("",plcreg,false,parent)
{
    trigger_value=value;
    delay=delay_sec;
    msg=message;

    count_timer.setInterval(1000);
    QObject::connect(&count_timer, &QTimer::timeout, this, &PowerOff::on_timer_tick);

    // думаю окно надо заранее создать, чтобы потом не надо было это делать фпапыхах. Места в памяти они много не займут, не волнуйся.
    msg_text=new QLabel();
    QFont fnt=ViewElement::getDefaultFont();
    fnt.setPointSize(fnt.pointSize()*3);
    msg_text->setFont(fnt);
    QPalette pal(msg_text->palette());
    pal.setColor(QPalette::WindowText, Qt::red);
    msg_text->setPalette(pal);

    QFrame *frame=new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setLayout(new QVBoxLayout);
    frame->layout()->addWidget(msg_text);

    msg_window=new QWidget(nullptr, Qt::Popup|Qt::WindowStaysOnTopHint);
    msg_window->setLayout(new QVBoxLayout);
    msg_window->layout()->addWidget(frame);
}

PowerOff::~PowerOff()
{
    if(msg_window) delete msg_window;
}

void PowerOff::view()
{
    if(value==trigger_value){
        if(!count_timer.isActive()){
            counter=delay;
            count_timer.start();
            if(msg_text) msg_text->setText(msg+"   "+QString::number(counter));
            if(msg_window){
                msg_window->show();
                msg_window->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, msg_window->size(),
                                                            QGuiApplication::screens().at(0)->availableGeometry()));
            }
            qDebug()<<"powerofftriggerdata";
        }
    }
    else{
        count_timer.stop();
        if(msg_window) msg_window->hide();
        //qDebug()<<"poweroffcanceldata";
    }
}

void PowerOff::on_timer_tick()
{
    if(msg_text) msg_text->setText(msg+"   "+QString::number(counter));
    if(msg_window){
        if(msg_window->isVisible()){
            if(msg_window->isMinimized())msg_window->showNormal();
            msg_window->raise();
        }
        else msg_window->show();
    }

    if(--counter<=0){
        count_timer.stop();
        // turn off
#if defined(Q_OS_WIN)
        QProcess::startDetached("shutdown /p /f");
#else
        sync();
        QProcess::startDetached("shutdown -P now");
#endif

    }
}
