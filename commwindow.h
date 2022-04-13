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

#ifndef COMMWINDOW_H
#define COMMWINDOW_H

#include <QDialog>
#include "plccommrs485.h"

namespace Ui {
class CommWindow;
}

class CommWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CommWindow(QWidget *parent = nullptr);
    virtual ~CommWindow();

    void setCommunicator(PlcCommRS485 *newcomm);
    void updatePortsList();
    void setSettingsFilename(QString filename);

public slots:
    void getMsg(QString msg);
    void show();

private slots:

    void on_closeWindowButton_clicked();

    void on_sendNextButton_clicked();

    void on_portCloseButton_clicked();

    void on_portOpenButton_clicked();

    void on_commStopButton_clicked();

    void on_commStartButton_clicked();

    void on_applyButton_clicked();

    void on_revertButton_clicked();

signals:

private:
    Ui::CommWindow *ui;
    PlcCommRS485 *communicator=nullptr;
    QString settingsfilename;

protected:
    bool eventFilter(QObject*, QEvent* event) override;// нужен чтобы отфильтровать БАГ с QEvent::PlatformSurface в момент закрытия окна (которого небыло еще в начале 2021, и обнаруженного осенью того же года)
};

#endif // COMMWINDOW_H
