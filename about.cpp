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

#include "about.h"
#include "ui_about.h"
#include "resources.h"
#include <QPlatformSurfaceEvent>
#include <QDebug>

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    // добавляем перевод:
    this->setWindowTitle(Resources::translate("about window"));
    ui->buttonOk->setText(Resources::translate("ok"));
    ui->tabWidget->setTabText(0,Resources::translate("about tab"));
    ui->tabWidget->setTabText(1,Resources::translate("about qt tab"));
    ui->tabWidget->setTabText(2,Resources::translate("about gpl tab"));
    ui->tabWidget->setTabText(3,Resources::translate("about lgpl tab"));
    ui->textEditAbout->clear();
    ui->textEditAbout->append("RS_HMI   0.19.12\n");
    ui->textEditAbout->append(Resources::translate("built with")+" Qt 5.12.2 64bit\n");
    ui->textEditAbout->append(Resources::translate("copyright")+"\n");
    ui->textEditAbout->append(Resources::translate("contact")+": sashka3001@gmail.com\n");
    ui->textEditAbout->append(Resources::translate("licensed"));

    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    this->installEventFilter(this); // нужен чтобы отфильтровать БАГ с QEvent::PlatformSurface
}

About::~About()
{
    delete ui;
}

void About::on_buttonOk_clicked()
{
    hide();
}

bool About::eventFilter(QObject *, QEvent *event)
{
    if(event->type() == QEvent::PlatformSurface){
        QPlatformSurfaceEvent *eve = (QPlatformSurfaceEvent *) event;
        qDebug()<<"stop this event to prevent BUG"<<QEvent::PlatformSurface<<":"<<eve->surfaceEventType();
        return true;
    }
    return false;
}
