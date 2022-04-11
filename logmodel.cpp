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

#include "logmodel.h"
#include <QTextStream>
#include <QFile>
#include <QDebug>

//#include <QColor>

AlarmLogStringModel::AlarmLogStringModel(const QStringList &alarmList, const QStringList &descriptionsList, QObject *parent):QAbstractListModel(parent)
{
    alarms=alarmList;
    descriptions=descriptionsList;
}

AlarmLogStringModel::AlarmLogStringModel(QObject *parent):QAbstractListModel(parent)
{

}

void AlarmLogStringModel::setAlarm(int alarmCode)
{
    if(!codeStatus.contains(alarmCode)){
        int row=log.count();
        codeStatus.insert(alarmCode, row); // добавляем индикацию того что сообщение активно
        beginInsertRows( QModelIndex(), 0, 0);
        log.append({alarmCode, QDateTime::currentDateTime()}); // добавляем сообщение
        endInsertRows();

        // А теперь сохранение в файл!
        // проверяем наличие файла, создаём если надо...

        QFile *file = new QFile(logfilefullpath);
        if(!file->exists()){
            if(!file->open(QIODevice::WriteOnly | QIODevice::Text)){
                qDebug()<<"Can't open log file "<<logfilefullpath; // "Ошибка открытия файла для логирования "
                delete file;
                return;
            }
            // сохраняем шапку
            QTextStream textstream(file);
            textstream << "Alarm log\n";
            textstream << "time \t code \t text\n";
        }
        else{
            if(!file->open(QIODevice::Append | QIODevice::Text)){
                qDebug()<<"Can't open log file ";
                delete file;
                return;
            }
        }

        // сохраняем сообщение
        QTextStream textstream(file);
        QString line;
        if(!dateformat.isEmpty()) line = QDateTime::currentDateTime().toString(dateformat);
        else line = QDate::currentDate().toString(Qt::SystemLocaleShortDate);
        line+="\t"+QString().number(alarmCode)+"\t";
        if(alarmCode<alarms.count())line+=alarms.at(alarmCode);

        textstream<<line+"\n";

        file->close();
        delete file;
    }
    else{
        // ничего в этом случае делать не надо т.к. сообщение всё ещё активно.
    }
}

void AlarmLogStringModel::resetAlarm(int alarmCode)
{
    if(codeStatus.contains(alarmCode)){
        beginResetModel();
        codeStatus.remove(alarmCode);
        endResetModel();
    }
    else{
        // ничего в этом случае делать не надо т.к. никаких активных сообщений просто нет
    }

}

void AlarmLogStringModel::setLogFile(const QString &filename)
{
    logfilefullpath = filename;

    // дальше надо загрузить содержимое файла в log (не забыть отправиль сигналы о начале вставки строк и конце)
    // ...

    QFile *file = new QFile(logfilefullpath);
    if(file->exists()){
        if(!file->open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug()<<"Can't open log file "<<logfilefullpath;
            delete file;
            return;
        }

        QTextStream textstream(file);
        while(!textstream.atEnd()){
            QStringList columns = textstream.readLine().split("\t");
            // анализируем строку и заполняем log. Как-то надо ещё и сигналы представлению посылать...
            if(columns.count() >= 2){
                QDateTime datetime;
                if(dateformat.isEmpty()) datetime = QDateTime::fromString(columns.at(0));
                else datetime = QDateTime::fromString(columns.at(0), dateformat);
                int code;
                bool codeisvalid;
                code = columns.at(1).toInt(&codeisvalid);

                if(codeisvalid && datetime.isValid()){
                    beginInsertRows( QModelIndex(), 0, 0);
                    log.append({code, datetime});
                    endInsertRows();
                    }
            }
        }
        file->close();
        delete file;
    }
    else qDebug()<<"file not found "<<logfilefullpath;
}

int AlarmLogStringModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED( parent );
    return log.count();
}

QVariant AlarmLogStringModel::data(const QModelIndex &index, int role) const
{
    int i = log.count()-index.row()-1;

    if((i>=0)&&(i<log.count())) // по идее эта проверка не обязательна, т.к. этот метод вызывает вьюшка это контролит? но надо чтобы было.
        if(role == Qt::DisplayRole){
            QString str;
            LogAlarm alarm = log.at(i);
            QString date;
            if(dateformat.count()) date = alarm.time.toString(dateformat);
            else date = alarm.time.toString(Qt::SystemLocaleShortDate);
            QString alarmText;
            if(alarm.code < alarms.count()) alarmText = alarms.at(alarm.code);
            str = date + "  -  "  + alarmText; // определить приемлемый формат даты-времени
            return str;
        }
        else if(role == Qt::ToolTipRole){
            QString str;
            if(log.at(i).code < descriptions.count())
                str = descriptions.at(log.at(i).code);
            return str;
        }
        else if(role == Qt::TextColorRole){
            // по индексу найти код, проверить совпадает значение в codeStatus с индексом,
            // вернуть цвет(ной шрифт), если да.
            // можно было бы и так: if(log.at(i).code == codeStatus.Key(i)) ...
            if(i == codeStatus.value(log.at(i).code,-1)) return activeColor;//QColor(Qt::red);
        }

    return QVariant();
}

void AlarmLogStringModel::setAlarmsText(const QStringList &alarmsText)
{
    alarms = alarmsText;
}

void AlarmLogStringModel::setAlarmsDescription(const QStringList &alarmsDescription)
{
    descriptions = alarmsDescription;
}

void AlarmLogStringModel::setDateFormat(const QString &format)
{
    dateformat = format;
}

void AlarmLogStringModel::setActiveAlarmColor(QColor color)
{
    activeColor = color;
}
