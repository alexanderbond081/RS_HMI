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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "fatekcommrs.h"
#include "datacollection.h"
#include "viewelement.h"
#include "viewtabcollection.h"

#include "commwindow.h"
#include "about.h"
#include <QMessageBox>
#include <QGraphicsColorizeEffect>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    // главное окно. кьютом сгенеренное
    Ui::MainWindow *ui;
    QWidget *hmiWidget=nullptr; // попытка избежать двойного удаления объектов (лейаут hmi удаляется при удалении ui, будучи добавленным на форму через addItem)
    //QLayout *hmiLayout;
    // это наша связь с контроллером, и всё что надо для её настройки. Передаётся через setDataCollector.
    PlcCommRS485 *comm=nullptr;
    QList<QSerialPortInfo> portList;
    CommWindow commwindow;
    About about;
    QMessageBox msgBox;
    bool noconnect_msg_may_show=false;
    //QGraphicsColorizeEffect *noconnect_color_effect; // криво работает

    QLabel portName,portStatus,plcStatus,commStatus;
    QTimer statusUpdateTimer;

    bool eventFilter(QObject* /*obj*/, QEvent* event) override;// нужен чтобы отфильтровать скроллинг мышью на ScrollArea, т.к. скроллирует одновременно с некоторыми виджетами

    int val=0;
protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setCommunicator(PlcCommRS485 *newcomm);
    void addHmi(QWidget *wid, QList<QMenu *> menus);

private slots:
    void showCommWindow();
    void showAboutWindow();
    void statusUpdate();

    void testmenu();
      //
     // слоты для обработки динамически сотворённых менюшек
    //
public slots:
    int setAppTitle(QString cap);

};

#endif // MAINWINDOW_H
