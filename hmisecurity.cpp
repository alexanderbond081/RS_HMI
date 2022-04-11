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

#include <qmessagebox.h>
#include <QCryptographicHash>
#include <QInputDialog>
#include "hmisecurity.h"
#include "resources.h"

SecurityLevelHandler::SecurityLevelHandler(int securityLevel, const QString &passMD5Hash, QObject *parent):QObject(parent)
{
    level=securityLevel;
    hash=passMD5Hash;
}

void SecurityLevelHandler::SecureAccess(QWidget *widget, int visLevel, int accessLevel)
{
    if(accessLevel){
        if(accessLevel<=level) widget->connect(this,SIGNAL(enable(bool)),SLOT(setEnabled(bool)));
        else widget->connect(this,SIGNAL(disable(bool)),SLOT(setEnabled(bool)));
    }

    if(visLevel){
        if(visLevel<=level) widget->connect(this,SIGNAL(enable(bool)),SLOT(setVisible(bool)));
        else widget->connect(this,SIGNAL(disable(bool)),SLOT(setVisible(bool)));
    }
}

void SecurityLevelHandler::SecureAccess(QAction *action, int visLevel, int accessLevel)
{
    if(accessLevel){
        if(accessLevel<=level) action->connect(this,SIGNAL(enable(bool)),SLOT(setEnabled(bool)));
        else action->connect(this,SIGNAL(disable(bool)),SLOT(setEnabled(bool)));
    }

    if(visLevel){
        if(visLevel<=level) action->connect(this,SIGNAL(enable(bool)),SLOT(setVisible(bool)));
        else action->connect(this,SIGNAL(disable(bool)),SLOT(setVisible(bool)));
    }
}

void SecurityLevelHandler::getAccess()
{
    emit enable();
    emit disable();
}

bool SecurityLevelHandler::getAccess(int securityLevel)
{
    if(securityLevel==level){
        emit enable();
        emit disable();
        return true;
    }
    return false;
}

bool SecurityLevelHandler::getAccess(const QString &passMD5Hash)
{
    // некоторые MD5 калькуляторы выдают результат с символами в верхнем регистре, а некоторые в верхнем.
    // где лучше делать перевод в нижний регистр, в этом методе или лучше при создании securityHandler?
    //qDebug()<<"md5 input:  "<<passMD5Hash;
    //qDebug()<<"md5 stored: "<<hash;
    if(passMD5Hash.toLower()==hash.toLower()){
        emit enable();
        emit disable();
        return true;
    }
    return false;
}

int SecurityLevelHandler::accessLevel()
{
    return level;
}

void HmiSecurity::view()
{
    if(newValReg==getPLCregister())
        if(value!=actualLevel){
            actualLevel=value;
            foreach(SecurityLevelHandler *handler, levelsList){
                if(handler->getAccess(actualLevel)) return;
            }
        }
}

HmiSecurity::HmiSecurity(PlcRegister plcreg, const QList<int> &securityLevelsList,
                         const QList<QString> &passMD5HashList, QWidget *parent): ViewElement("",plcreg,false,parent)
{
    //qDebug() << securityLevelsList.count();
    for(int i=0;i<securityLevelsList.count();i++)
    {
        QString pass=""; // дефолтное значение hash
        if(i<passMD5HashList.count())pass=passMD5HashList.at(i);
        levelsList<< new SecurityLevelHandler(securityLevelsList.at(i),pass,this);
    }
}

void HmiSecurity::SecureAccess(QWidget *widget, int visLevel, int accessLevel)
{
    foreach(SecurityLevelHandler *handler, levelsList){
        handler->SecureAccess(widget,visLevel,accessLevel);
    }
}

void HmiSecurity::SecureAccess(QAction *action, int visLevel, int accessLevel)
{
    foreach(SecurityLevelHandler *handler, levelsList){
        handler->SecureAccess(action,visLevel,accessLevel);
    }
}

int HmiSecurity::actualAccessLevel()
{
    return actualLevel;
}

void HmiSecurity::passwordDialog()
{
    QString pass = QInputDialog::getText(nullptr, Resources::translate("advanced access"),
                                         Resources::translate("enter password"), QLineEdit::Password, ""); // "Расширенный доступ", "Введите пароль:"
    //qDebug()<<pass;
    QString hash = QString(QCryptographicHash::hash(pass.toUtf8(),QCryptographicHash::Md5).toHex());
    qDebug()<<"input pass md5: "<<hash;
    if(pass.length()>0){
        bool valuable=false;
        foreach(SecurityLevelHandler *handler, levelsList){
            valuable=handler->getAccess(hash);
            if(valuable){
                actualLevel=handler->accessLevel();
                QMessageBox::information(nullptr, Resources::translate("advanced access"),
                                         Resources::translate("access level")+" "+QString::number(actualLevel));
                break; // в случае совпадения дальнейший обход бессмысленен
            }
        }
        if(!valuable) QMessageBox::warning(nullptr, Resources::translate("advanced access"),
                                           Resources::translate("wrong password")); // "Расширенный доступ","Пароль не подходит"
    }
}

void HmiSecurity::resetAccess()
{
    // надо ли тут проверка переполнения? По идее нет, т.к. некому будет вызывать?
    qDebug()<<levelsList.count();

    if(levelsList.count())levelsList.at(0)->getAccess();
    if(actualLevel!=0){
        actualLevel=0;
        QMessageBox::information(nullptr, Resources::translate("advanced access"),
                                 Resources::translate("access level reset to")+" "+QString::number(actualLevel));
    }
}


// ////////////////////////////////////////////////////
// класс адаптер для вкладок
// потому, что напрямую сигналы вкладкам не передаются

TabWidgetSetEnabledSignalAdapter::TabWidgetSetEnabledSignalAdapter(QTabWidget *tabWidget, int tabNumber): QAction(tabWidget)
{
    tabWidg=tabWidget;
    tabNum=tabNumber;
}

void TabWidgetSetEnabledSignalAdapter::setVisible(bool visible)
{
    tabWidg->setTabEnabled(tabNum,visible);
}

void TabWidgetSetEnabledSignalAdapter::setEnabled(bool enabled)
{
    tabWidg->setTabEnabled(tabNum,enabled);
}
