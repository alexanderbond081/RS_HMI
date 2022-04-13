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

#include "commwindow.h"
#include "ui_commwindow.h"
#include "resources.h"
#include <qplaintextedit.h>

CommWindow::CommWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, false);

    // перевод подписей:
    this->setWindowTitle(Resources::translate("device settings"));
    ui->labelDevice->setText(Resources::translate("device"));
    ui->labelBaud->setText(Resources::translate("baud"));
    ui->labelParity->setText(Resources::translate("parity"));
    ui->labelBits->setText(Resources::translate("bits"));
    ui->labelStopBits->setText(Resources::translate("stop bits"));
    ui->labelDelay->setText(Resources::translate("delay"));
    ui->labelTimeout->setText(Resources::translate("timeout"));

    ui->commStartButton->setText(Resources::translate("start"));
    ui->commStopButton->setText(Resources::translate("stop"));
    ui->portOpenButton->setText(Resources::translate("connect"));
    ui->portCloseButton->setText(Resources::translate("disconnect"));
    ui->sendNextButton->setText(Resources::translate("send"));

    ui->applyButton->setText(Resources::translate("apply"));
    ui->revertButton->setText(Resources::translate("revert"));
    ui->closeWindowButton->setText(Resources::translate("close"));

    ui->comboBoxPorts->clear();
    ui->comboBoxBaud->clear();
    ui->comboBoxBits->clear();
    ui->comboBoxParity->clear();
    ui->comboBoxStopBits->clear();

    //ui->msgPlainText->document()->setMaximumBlockCount(100);

    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    this->installEventFilter(this); // нужен чтобы отфильтровать БАГ с QEvent::PlatformSurface
}

CommWindow::~CommWindow()
{
    delete ui;
    communicator=nullptr;
}

void CommWindow::setCommunicator(PlcCommRS485 *newcomm)
{
    if(newcomm){
        communicator=newcomm;

        QObject::connect(communicator, SIGNAL(msg(QString)), this, SLOT(getMsg(QString)));

        // на случай если кто-то решит больше 1го раза вызвать этот метод
        ui->comboBoxBaud->clear();
        ui->comboBoxBits->clear();
        ui->comboBoxParity->clear();
        ui->comboBoxStopBits->clear();

        ui->comboBoxBaud->addItems(communicator->getBaudList());
        ui->comboBoxBits->addItems(communicator->getBitsList());
        ui->comboBoxParity->addItems(communicator->getParityList());
        ui->comboBoxStopBits->addItems(communicator->getStopList());

        // настройки по умолчанию
        QString defaultPort="";
        if(communicator->getPortList().count())
            defaultPort=communicator->getPortList().at(0).portName();
        QString defaultBaud="38400";
        QString defaultBits="8";
        QString defaultParity="Even";
        QString defaultStopBits="1";
        int defaultDelay=200;
        int defaultTimeout=1500;

        // если есть файл настроек, то берём настройки из него
        QFile settingsfile(settingsfilename);
        if(settingsfile.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream loadsettings(&settingsfile);
            if(!loadsettings.atEnd()) defaultPort=loadsettings.readLine();
            if(!loadsettings.atEnd()) defaultBaud=loadsettings.readLine();
            if(!loadsettings.atEnd()) defaultBits=loadsettings.readLine();
            if(!loadsettings.atEnd()) defaultParity=loadsettings.readLine();
            if(!loadsettings.atEnd()) defaultStopBits=loadsettings.readLine();
            if(!loadsettings.atEnd()) {bool ok=0;int val=loadsettings.readLine().toInt(&ok); if(ok) defaultDelay=val;}
            if(!loadsettings.atEnd()) {bool ok=0;int val=loadsettings.readLine().toInt(&ok); if(ok) defaultTimeout=val;}
         }
        communicator->setPortName(defaultPort);
        communicator->setPortBaud(defaultBaud);
        communicator->setPortBits(defaultBits);
        communicator->setPortParity(defaultParity);
        communicator->setPortStopBits(defaultStopBits);
        communicator->setRequestDelay(defaultDelay);
        communicator->setRequestTimeout(defaultTimeout);

        communicator->communicationStart();
    }
}

void CommWindow::updatePortsList()
{
    if(communicator){
        QList<QSerialPortInfo> portList = communicator->getPortList();
        ui->comboBoxPorts->clear();
        ui->comboBoxPorts->addItem(communicator->getPortName() + "  ("+ Resources::translate("current device")+")"); // текущий порт
        ui->comboBoxPorts->insertSeparator(1);
        ui->comboBoxPorts->addItem("  ( " + Resources::translate("empty") + ")"); // пусто
        foreach (QSerialPortInfo portInfo, portList) {
            ui->comboBoxPorts->addItem(portInfo.portName() + "  (" + portInfo.description() +
                                       ", " + portInfo.manufacturer() + ", s/n:" + portInfo.serialNumber() + " )");
        }
    }
}

void CommWindow::setSettingsFilename(QString filename)
{
    settingsfilename=filename;
}

void CommWindow::on_closeWindowButton_clicked()
{
    this->hide();
}

void CommWindow::getMsg(QString msg)
{
/*    QString hexstr;
    for(int i=0;i<msg.length();i++){
        hexstr.append(QString::number(msg.at(i).toLatin1(),16).toUpper() + " ");
    }*/
    ui->msgPlainText->appendPlainText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + ": " + msg);
}

void CommWindow::show()
{
    on_revertButton_clicked();
    QDialog::show();
}

void CommWindow::on_sendNextButton_clicked()
{
    communicator->sendnext();
}

void CommWindow::on_portCloseButton_clicked()
{
    communicator->closePort();
}

void CommWindow::on_portOpenButton_clicked()
{
    communicator->openPort();
}

void CommWindow::on_commStopButton_clicked()
{
    communicator->communicationStop();
}

void CommWindow::on_commStartButton_clicked()
{
    communicator->communicationStart();
}

void CommWindow::on_applyButton_clicked()
{
    communicator->setPortBaud(ui->comboBoxBaud->currentText());
    communicator->setPortBits(ui->comboBoxBits->currentText());
    communicator->setPortParity(ui->comboBoxParity->currentText());
    communicator->setPortStopBits(ui->comboBoxStopBits->currentText());
    communicator->setPortName(ui->comboBoxPorts->currentText().left(ui->comboBoxPorts->currentText().indexOf("  ")));
    communicator->setRequestDelay(ui->spinBoxDelay->value());
    communicator->setRequestTimeout(ui->spinBoxTimeout->value());

    QFile settingsfile(settingsfilename);
    if (!settingsfile.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream savesettings(&settingsfile);
    savesettings<<communicator->getPortName()<<"\n";
    savesettings<<communicator->getBaud()<<"\n";
    savesettings<<communicator->getBits()<<"\n";
    savesettings<<communicator->getParity()<<"\n";
    savesettings<<communicator->getStopBits()<<"\n";
    savesettings<<communicator->getRequestDelay()<<"\n";
    savesettings<<communicator->getRequestTimeout()<<"\n";

    updatePortsList();
}

void CommWindow::on_revertButton_clicked()
{
    ui->comboBoxBaud->setCurrentText(communicator->getBaud());
    ui->comboBoxBits->setCurrentText(communicator->getBits());
    ui->comboBoxParity->setCurrentText(communicator->getParity());
    ui->comboBoxStopBits->setCurrentText(communicator->getStopBits());
    ui->spinBoxDelay->setValue(communicator->getRequestDelay());
    ui->spinBoxTimeout->setValue(communicator->getRequestTimeout());

    updatePortsList();

}

bool CommWindow::eventFilter(QObject *, QEvent *event)
{
    if(event->type() == QEvent::PlatformSurface){
        QPlatformSurfaceEvent *eve = (QPlatformSurfaceEvent *) event;
        qDebug()<<"stop this event to prevent BUG"<<QEvent::PlatformSurface<<":"<<eve->surfaceEventType();
        return true;
    }
    return false;
}
