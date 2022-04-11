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

#ifndef HMISECURITY_H
#define HMISECURITY_H

#include <QObject>
#include <QWidget>
#include <QAction>

#include "viewelement.h"
//#include "plcregister.h"

class SecurityLevelHandler: public QObject
{
    Q_OBJECT

private:
    int level;
    QString hash;

public:
    explicit SecurityLevelHandler(int securityLevel, const QString &passMD5Hash,QObject *parent);
    void SecureAccess(QWidget *widget, int visLevel, int accessLevel);
    void SecureAccess(QAction *action, int visLevel, int accessLevel);
    void getAccess();
    bool getAccess(int securityLevel);
    bool getAccess(const QString &passMD5Hash);
    int accessLevel();

signals:
    void enable(bool enabled=true);
    void disable(bool enabled=false);

};

class HmiSecurity : public ViewElement
{
    Q_OBJECT

private:
    int actualLevel=0;
    QList<SecurityLevelHandler*> levelsList;
    void view(); // обработать новое значение принятое с контроллера

public:
    explicit HmiSecurity(PlcRegister plcreg, const QList<int> &securityLevelsList,
                         const QList<QString> &passMD5HashList, QWidget *parent=nullptr);
    void SecureAccess(QWidget *widget, int visLevel=0, int accessLevel=0);
    void SecureAccess(QAction *action, int visLevel=0, int accessLevel=0);
    int actualAccessLevel();

signals:

public slots:
    void passwordDialog();
    void resetAccess();
};


class TabWidgetSetEnabledSignalAdapter: public QAction
{
    Q_OBJECT
private:
    QTabWidget *tabWidg;
    int tabNum;
public:
    explicit TabWidgetSetEnabledSignalAdapter(QTabWidget *tabWidget, int tabNumber);
public slots:
    void setVisible(bool visible);
    void setEnabled(bool enabled);
};

#endif // HMISECURITY_H
