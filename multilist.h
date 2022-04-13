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

#ifndef MULTILIST_H
#define MULTILIST_H

#include <QtGui>
#include <QComboBox>
#include <QListWidget>

class ComboCheckBoxWidget: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(QStringList checkedItems READ checkedItems WRITE setCheckedItems)

public:
    ComboCheckBoxWidget(QWidget *parent);
    virtual ~ComboCheckBoxWidget();

    QStringList checkedItems() const;
    void setCheckedItems(const QStringList &items);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QStringList mCheckedItems;

    void collectCheckedItems();

    QString mDisplayText;
    const QRect mDisplayRectDelta = QRect(4, 1, -25, 0);

    void updateDisplayText();

private slots:
    void slotModelRowsInserted(const QModelIndex &parent, int start, int end);
    void slotModelRowsRemoved(const QModelIndex &parent, int start, int end);
    void slotModelItemChanged(QStandardItem *item);

};

//==================================

class ComboCheckBox: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(QStringList checkedItems READ checkedItems WRITE setCheckedItems)

public:
    ComboCheckBox(QWidget *parent);
    virtual ~ComboCheckBox();

    QStringList checkedItems() const;
    void setCheckedItems(const QStringList &items);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QStringList mCheckedItems;
    QListWidget *listWidget;

    void collectCheckedItems();

    QString mDisplayText;
    const QRect mDisplayRectDelta = QRect(4, 1, -25, 0);

    void updateDisplayText();

private slots:
    void slotModelRowsInserted(const QModelIndex &parent, int start, int end);
    void slotModelRowsRemoved(const QModelIndex &parent, int start, int end);
    void slotModelItemChanged(QListWidgetItem *item);

};

#endif // MULTILIST_H
