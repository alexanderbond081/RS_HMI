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

#ifndef VIEWTABCOLLECTION_H
#define VIEWTABCOLLECTION_H

#include <QObject>
#include "viewelement.h"
#include "plcregister.h"

/* вместилище пар <регистр, визуализация> для формирования наборов регистров для запросов контроллеру.
 * выполнено в иерархическом виде для уменьшения количества запрашиваемых регистров и ускорения
 * поиска соответствующих регистрам визуализаций.
 * Один набор вложенных вкладок, один индекс, один слот переключателя.
*/

class ViewTabCollection: public QObject
{
    Q_OBJECT

private:
    QMap<PlcRegister, QPointer<ViewElement>> currentTabElements;
    QList<ViewTabCollection*> subTabs; // можно было бы воспользоваться встроенной в QObject коллекцией children. Но тогда придётся проверять тип chield, чтобы не случилось ошибки.
    int activeSubTabIndex=0;

public:
    ViewTabCollection(QObject *parent=0);
   // ~ViewTabCollection();

    void connectTabWidget(QTabWidget* tabWidget);
    void addTab(ViewTabCollection *newTab);
    void addViewElement(const PlcRegister &key, ViewElement *element);

    QMap<PlcRegister, QPointer<ViewElement> > getActualMap();
    QMap<PlcRegister, QPointer<ViewElement> > getAllMap();

signals:
    void actualMapUpdated(void); // этот сигнал проходит сквозьняком от нижних уровней вкладок к пользователю коллекции, чтобы тот обновил у себя список текущих объектов на экране

private slots:
    void subMapUpdated(void); // слот-приёмник для сигнала actualMapUpdated от своих вложений.
    void onChangeTab(int newIndex); // слот-инициатор сигнала actualMapUpdated
};

#endif // VIEWTABCOLLECTION_H
