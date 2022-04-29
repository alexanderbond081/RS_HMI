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

#include <QFileDialog>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include "datasaveload.h"
#include "resources.h"

RecipeStorage::RecipeStorage(const QString &name, const QList<PlcRegister> &registers, DataCollection *data, QObject *parent) : QObject(parent)
{
    caption=name;
    path=Consts::workDirPath+"/"+name;
    QDir dir;
    dir.mkpath(path);
    path+="/"+name+".dat";
    // спутями разобрались, а как сделать имя файла по умолчанию? и как его запоминать?

    foreach(PlcRegister reg, registers)
        if(reg.isValid())regs<<reg; // Проверку делать всё-таки надо, а то насохраняет битых регистров в файл.
    collection=data;
    connect(&savetimer,SIGNAL(timeout()),SLOT(onTimer()));
    // save load конектить будет hmi
}

void RecipeStorage::onTimer()
{
    // ??? это будет работать ???
    // ждём пока регистры прочитаются
    // !!! сделать бы чтобы учитывались только регистры только из данного рецепта !!! (сейчас смотрятся все регистры из списка одноразового запроса)
    if(collection->getReadOnceCount()>0){
        if(QTime::currentTime() > savetimeout){
            savetimer.stop();
            emit saveFinish();
            qDebug()<<"data saving timeout - " << savefilename;

            // и тут надо вывести объявление на экран, что сохранить не удалось!!!!!
            QMessageBox::warning(nullptr,Resources::translate("save failed"),
                                 Resources::translate("plc connection error")); // "Ошибка сохранения данных","Нет связи с ПЛК."
        }
        if(!DEBUG_TEST) return; // игнорируем ошибку, если это дебаг, и тупо сохраняем что есть.
    }

    // а теперь сохраняем
    savetimer.stop();
    //qDebug()<<savefilename;
    //QMap<PlcRegister, int> data;
    QFile *file = new QFile(savefilename);
    if(!file->open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug()<<"Can't open file";//"Ошибка открытия файла";
        QMessageBox::warning(nullptr,Resources::translate("save failed"),
                             Resources::translate("can't open file")); // "Ошибка сохранения данных","Невозможно открыть файл."
        return;
    }
    QTextStream textstream(file);
    foreach(PlcRegister reg, regs){
        textstream<<reg.toString()+"\t"+QString().number(collection->getValue(reg))+"\n";
    }
    file->close();
    delete file;

    Resources::log("save recipe "+savefilename);

    emit saveFinish();
}

void RecipeStorage::save()
{
    if(collection==nullptr) return;

    // для того чтобы записать регистры в файл, сначала нужно прочитать их из контроллера
    collection->addToReadOnceList(regs);
    // получаем имя файла
    QFileDialog savedialog;
    savedialog.setDefaultSuffix(".dat");
    savefilename = savedialog.getSaveFileName(nullptr, Resources::translate("save")+" "+caption, path, "*.dat");
    if(savefilename.isEmpty())return; // была нажата "отмена"
    path=savefilename;
    savetimeout=QTime::currentTime().addSecs(3);
    savetimer.start(100);
    emit saveStart();
}

void RecipeStorage::load()
{
    if(collection==nullptr) return;

    QString filename = QFileDialog::getOpenFileName(nullptr, Resources::translate("load")+" "+caption, path, "*.dat");
    if(filename.isEmpty())return;

    QMap<PlcRegister, int> data;
    QFile *file = new QFile(filename);
    if(!file->open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Can't open file"; //"Ошибка загрузки файла";
        return;
    }
    QTextStream textstream(file);
    QList<PlcRegister> expectedRegs=regs;
    while(!textstream.atEnd()){
        QStringList line=textstream.readLine().split("\t"); // QChar(9)
        PlcRegister reg(line.at(0));
        int val=0;
        if(line.count()>1) val=line.at(1).toInt();
        if(expectedRegs.contains(reg)){
            expectedRegs.removeOne(reg);
            data.insert(reg,val); // проверяем наличие регистра в списке. Чтобы не насувать лишнего.
        }
    }

    // Добавить анализ все ли регистры из ожидаемого списка были прочитаны
    if(expectedRegs.count()>0){
        // сообщаем о том что файл не полный.
        if(QMessageBox::question(nullptr,Resources::translate("warning"),
                                 Resources::translate("file corrupted warning"))!=QMessageBox::Yes) // "Файл содержит не полные данные или повреждён. Загрузить не полные данные?"
            return;
    }
    collection->submitRegisterValues(data);
    path=filename;

    Resources::log("load recipe "+filename);
}



/********************************/
/*   Реализация  TextfileLog    */
/********************************/

TextfileLog::TextfileLog(const QString &logName, int interval, const QList<PlcRegister> &logRegisters,
                         const QList<int> &regDecimals, const QList<QString> &regLabels,
                         QWidget *parent): ViewElement("",PlcRegister("none"),true,parent)
{
    // нет защиты от разной длины registers, decimals и т.п.
    name=logName;
    QString path=Consts::workDirPath+"/"+name;
    QDir dir;
    dir.mkpath(path);

    /*
    - TextfileLog (вообще нет проверок на соответствие кол-ва регистров и параметров отображения)
    сделать адо чтото типа:
        for(int c=0;c<regs.count();c++){
            if(labels.count()<=c)labels.append(regs.at(c).toString());
            if(regintervals.count()<=c) regintervals.append(1);
            else if(regintervals.at(c)<1)regintervals[c]=1;
        }
    */

    registers = logRegisters;
    decimals = regDecimals;
    for(int i=0;i<decimals.count();i++)
        dividers<<qPow(10,decimals.at(i));
    labels = regLabels;
    values.insert(0,registers.count(),0);

    // чтобы не надо было лишний контроль за длиной массивов осуществлять.
    for(int c=0;c<registers.count();c++){
        if(labels.count()<=c)labels.append("");
        if(decimals.count()<=c)decimals.append(0);
        if(dividers.count()<=c)dividers.append(1);
    }

    timer=new QTimer(this);
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    timer->start(interval);
}

void TextfileLog::onTimer()
{
    /*  если файл не существует, то создаём его и запихиваем туда шапку
        если же существует, то просто записываем туда */

    QString filename = Consts::workDirPath+"/"+name+"/"+QDate::currentDate().toString()+".txt";
    QFile *file = new QFile(filename);
    if(!file->exists()){
        if(!file->open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug()<<"Can't open log file";//"Ошибка открытия файла для логирования";
            return;
        }
        QTextStream textstream(file);
        textstream << "Text log file: "+name+"\n";
        textstream << "time";
        foreach (QString label, labels) {
            textstream << "\t"+label;
        }
        textstream << "\n";
    }
    else{
        if(!file->open(QIODevice::Append | QIODevice::Text)){
            qDebug()<<"Can't open log file";//"Ошибка открытия файла для логирования";
            return;
        }
    }
    QTextStream textstream(file);
    QString line=QTime::currentTime().toString("HH:mm:ss");
    for(int n=0;n<values.count();n++){
        int divider=dividers.at(n);
        int decimal=decimals.at(n);
        line+="\t"+QString::number((double)values.at(n)/divider,'f',decimal);
    } //file:///home/sashka1/testalarmlog.txt <- что это блин за хрень?! случайная строчка?
    textstream<<line+"\n";

    file->close();
    delete file;

}

void TextfileLog::view()
{
    int n=registers.indexOf(newValReg);
    if(n>=0) values.replace(n, value);
}
