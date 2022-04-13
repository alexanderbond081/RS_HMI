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

#ifndef CUSTOMDELEGATES_H
#define CUSTOMDELEGATES_H

#include <QObject>
#include <QStyledItemDelegate>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QStringList>

class DoubleSpinboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    double minVal, maxVal;
    int DecDigits;
    static QDoubleSpinBox* doubleSpinboxSizehintStaticInstance;

public:
    explicit DoubleSpinboxDelegate(double minValue, double maxValue, int decimalDigits, QObject *parent=nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


class SpinboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    int minVal, maxVal;
    int DecDigits;
    int base;
    bool buttons;

public:
    explicit SpinboxDelegate(int integerBase, int minValue, int maxValue, bool showButtons, QObject *parent=nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class CheckBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:

public:
    explicit CheckBoxDelegate(QObject *parent=nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    QStringList list;

public:
    explicit ComboBoxDelegate(QStringList stringList, QObject *parent=nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class FlagBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    QStringList list;
    QString text4paint(int flags) const;

public:
    explicit FlagBoxDelegate(QStringList stringList, QObject *parent=nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

enum TimeUnits{
    hour,
    minute,
    sec,
    msec
};

class TimeEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    int min,max;
    TimeUnits value_units;
    QString format;

public:

    explicit TimeEditDelegate(int minTime, int maxTime, QObject *parent=nullptr, TimeUnits units=sec);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class VaribleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    QList<QStyledItemDelegate*> delegates;
    int *delegateIndex;
    int columnIndex;

public:
    explicit VaribleDelegate(int definingColumn, const QStringList &cellTypes, QList<QStringList> *textLists, QObject *parent=nullptr);
    virtual ~VaribleDelegate();

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // CUSTOMDELEGATES_H
