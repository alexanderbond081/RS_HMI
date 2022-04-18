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

#include <QApplication>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "mainwindow.h"

#include "fatekcommrs.h"
#include "datacollection.h"
#include "hmi.h"
#include "resources.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(a.arguments().count()>1) Consts::loadPathCfg(a.arguments().at(1));
    else Consts::loadPathCfg("");
    Resources::loadSettings();
    QString lang;
    if(Resources::LANGUAGE.isEmpty()) lang=QLocale().name().split("_").at(0);
    else lang=Resources::LANGUAGE;
    Resources::loadDictionary(lang);

    DataCollection registerData;
    FatekCommRS dataCollector;
    Hmi hmi;
    MainWindow w;
    QSharedMemory titleMem;

    if(Resources::FULL_SCREEN) w.setWindowState(Qt::WindowFullScreen | Qt::WindowMaximized);
    dataCollector.setDataCollection(&registerData); // говорим модулю связи, откуда брать данные (модель)
    w.setCommunicator(&dataCollector); // говорим главному окну, кто будет добывать данные из контроллера
    hmi.setRegisterData(&registerData); // связываем модель и представление модели
    hmi.setValidator(dataCollector.makeValidator()); // представление должно уметь приводить имена регистров к валидному виду

    hmi.loadFromXml(Consts::xmlHMIPath);
    // сделать не надо было читать весь xml файл, для проверки на наличие копии, а только program title?
    // (реализовать метод check, который будет читать только название)

    // ??? Как послать сигнал уже открытому приложению на поднятие окна и перевод фокуса???
    /* С помощью API целевой системы придётся найти нужный процесс и открыть окно.
     * Но придётся писать платформозависимый код. В случае Windows нужно смотреть,
     * как с помощью WinAPI искать процессы и пытаться открыть их окна, если они есть.
     * В случае Linux там ещё сложнее, или через XLib, или через DBus, но я не уверен, как именно это делается. */

    if(!hmi.getTitle().isEmpty())
    {
        titleMem.setKey(hmi.getTitle()); // создаем общую память, ключем является caption
        titleMem.attach(); // в линуксе если не очистить шаред_память, то она останется в системе после закрытия программы.
        titleMem.detach(); // подкл-откл с этим ключем - это очистит память, если программа вылетела, а ключ остался.
        if (!titleMem.create(1)){
            qDebug()<<"hmi "<<hmi.getTitle()<<" is alredy started";
            return 0;
        }
        w.setWindowTitle(hmi.getTitle());
    }

    w.addHmi(hmi.getWidget(),hmi.getMenuList()); // а подгонку размеров окна оставляю на совести функции addHmi(), оттуда проще выкавыривать размеры содержимого
    if(!w.isVisible())w.show(); // show на совести w.addHmi, но если вдруг что-то не так, то надо же что-то показать.

    return a.exec();
}
