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

#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QDateTime>
#include <QColor>

struct LogAlarm{
    int code;
    QDateTime time;
};

class AlarmLogStringModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AlarmLogStringModel(const QStringList &alarmList, const QStringList &descriptionsList, QObject *parent=0);
    explicit AlarmLogStringModel(QObject *parent=0);

    void setAlarm(int alarmCode); // с установкой новой алармы вроде бы всё просто
    void resetAlarm(int alarmCode); // но вот какдолжно работать отпускание... не понимаю. Всё подряд передавать, а там пусть само разбирается?

    void setLogFile(const QString &filename); // загружаем то что есть, пишем лог туда же.

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setAlarmsText(const QStringList &alarmsText);
    void setAlarmsDescription(const QStringList &alarmsDescription);
    void setDateFormat(const QString &format);
    void setActiveAlarmColor(QColor color);

private:
    QString dateformat;//="dd.MM.yy H:m:s";
    QString logfilefullpath;
    QStringList alarms;
    QStringList descriptions;
    QHash<int,int> codeStatus;
    QVector<LogAlarm> log;

    QColor activeColor=QColor(Qt::darkRed);

};

#endif // LOGMODEL_H
