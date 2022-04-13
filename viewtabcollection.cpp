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

#include "viewtabcollection.h"

ViewTabCollection::ViewTabCollection(QObject *parent):QObject(parent)
{
    // вобщем-то нечего инициализировать.
}

/*ViewTabCollection::~ViewTabCollection()
{
    // вложенные вкладки автоматически удалятся, т.к. являются детьми this
    // ViewElement удалять не надо, т.к. это коллекция УКАЗАТЕЛЕЙ QPointer, а владелец.

}*/

void ViewTabCollection::connectTabWidget(QTabWidget *tabWidget)
{
    connect(tabWidget, SIGNAL(currentChanged(int)), SLOT(onChangeTab(int)));
}

void ViewTabCollection::addTab(ViewTabCollection *newTab)
{
    newTab->setParent(this);
    subTabs.append(newTab); // зачем плодить самодельную систему удаления вложенных объектов, когда есть уже готовая
    connect(newTab, SIGNAL(actualMapUpdated()), SLOT(subMapUpdated()));
}

void ViewTabCollection::addViewElement(const PlcRegister &key, ViewElement *element)
{
    if(key.isValid())currentTabElements.insertMulti(key, element); // неверные ключи надо отсекать на самом раннем этапе.
}

QMap<PlcRegister, QPointer<ViewElement>> ViewTabCollection::getActualMap()
{
    auto map = currentTabElements;
    if(subTabs.count() > activeSubTabIndex)
            map.unite(subTabs.at(activeSubTabIndex)->getActualMap());
    return map;
}

QMap<PlcRegister, QPointer<ViewElement> > ViewTabCollection::getAllMap()
{
    auto map = currentTabElements;
    foreach(ViewTabCollection *tab, subTabs)
            map.unite(tab->getAllMap());
    return map;
}

void ViewTabCollection::subMapUpdated()
{
    emit actualMapUpdated();
}

void ViewTabCollection::onChangeTab(int newIndex)
{
    activeSubTabIndex = newIndex;
    emit actualMapUpdated();
}

// решает проблему поддержки QPointer в QSet, но на самом деле можно и без QSet обойтись.
/*uint qHash(const QPointer<ViewElement>& client) {
    return qHash(client.operator->());
}*/
