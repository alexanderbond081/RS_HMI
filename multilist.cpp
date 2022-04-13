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

#include "multilist.h"
#include <QStylePainter>
#include <QDebug>
#include <QStyleFactory>

ComboCheckBoxWidget::ComboCheckBoxWidget(QWidget *parent):QComboBox(parent)
{
    setStyle(QStyleFactory::create("Windows")); // это мега аццкий кастыль после очередного обновления Qt. Без него чекбоксы работать не будут. Последняя проверка 5.11.1 - не работает.
    //setStyleSheet("QComboBox { combobox-popup:12px }"); // а раньше этот баг лечился этой стройкой, теперь только кастыль с виндовым стилем.

    connect(model(), SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(slotModelRowsInserted(QModelIndex,int,int)));
    connect(model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(slotModelRowsRemoved(QModelIndex,int,int)));

    QStandardItemModel *standartModel = qobject_cast<QStandardItemModel*>(model());

    connect(standartModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotModelItemChanged(QStandardItem*)));
}

ComboCheckBoxWidget::~ComboCheckBoxWidget()
{

}

QStringList ComboCheckBoxWidget::checkedItems() const
{
	return mCheckedItems;
}

void ComboCheckBoxWidget::setCheckedItems(const QStringList &items)
{
	QStandardItemModel *standartModel = qobject_cast<QStandardItemModel*>(model());

    disconnect(standartModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotModelItemChanged(QStandardItem*)));

	for (int i = 0; i < items.count(); ++i)
	{
		int index = findText(items.at(i));

		if (index != -1)
		{
            standartModel->item(index)->setData(Qt::Checked, Qt::CheckStateRole);
		}
	}
    connect(standartModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotModelItemChanged(QStandardItem*)));
	collectCheckedItems();
}

void ComboCheckBoxWidget::paintEvent(QPaintEvent *event)
{
	(void)event;

	QStylePainter painter(this);

	painter.setPen(palette().color(QPalette::Text));

	QStyleOptionComboBox option;

	initStyleOption(&option);

	painter.drawComplexControl(QStyle::CC_ComboBox, option);

    QRect textRect = rect().adjusted(mDisplayRectDelta.left(), mDisplayRectDelta.top(), mDisplayRectDelta.right(), mDisplayRectDelta.bottom());

	painter.drawText(textRect, Qt::AlignVCenter, mDisplayText);
}

void ComboCheckBoxWidget::resizeEvent(QResizeEvent *event)
{
	(void)event;

	updateDisplayText();
}

void ComboCheckBoxWidget::collectCheckedItems()
{
	QStandardItemModel *standartModel = qobject_cast<QStandardItemModel*>(model());

	mCheckedItems.clear();

    for (int i = 0; i < count(); ++i)
	{
        QStandardItem *currentItem = standartModel->item(i);

		Qt::CheckState checkState = static_cast<Qt::CheckState>(currentItem->data(Qt::CheckStateRole).toInt());

		if (checkState == Qt::Checked)
		{
			mCheckedItems.push_back(currentItem->text());
        }
	}

	updateDisplayText();

	repaint();
}

void ComboCheckBoxWidget::updateDisplayText()
{
    QRect textRect = rect().adjusted(mDisplayRectDelta.left(), mDisplayRectDelta.top(), mDisplayRectDelta.right(), mDisplayRectDelta.bottom());

	QFontMetrics fontMetrics(font());

	mDisplayText = mCheckedItems.join(", ");

	if (fontMetrics.size(Qt::TextSingleLine, mDisplayText).width() > textRect.width())
	{
		while (mDisplayText != "" && fontMetrics.size(Qt::TextSingleLine, mDisplayText + "...").width() > textRect.width())
		{
			mDisplayText.remove(mDisplayText.length() - 1, 1);
		}

		mDisplayText += "...";
	}
}

void ComboCheckBoxWidget::slotModelRowsInserted(const QModelIndex &parent, int start, int end)
{
	(void)parent;

    QStandardItemModel *standartModel = qobject_cast<QStandardItemModel*>(model());
    disconnect(standartModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotModelItemChanged(QStandardItem*)));

	for (int i = start; i <= end; ++i)
	{
        standartModel->item(i)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        standartModel->item(i)->setCheckState(Qt::Unchecked);
	}

    connect(standartModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotModelItemChanged(QStandardItem*)));
}

void ComboCheckBoxWidget::slotModelRowsRemoved(const QModelIndex &parent, int start, int end)
{
	(void)parent;
	(void)start;
	(void)end;

	collectCheckedItems();
}

void ComboCheckBoxWidget::slotModelItemChanged(QStandardItem *item)
{
	(void)item;

	collectCheckedItems();
}


  //================================//
 //  ComboCheckBox v2 for Qt5 lib  //
//================================//

ComboCheckBox::ComboCheckBox(QWidget *parent):QComboBox(parent)
{
    listWidget = new QListWidget;
    this->setModel(listWidget->model());
    this->setView(listWidget);

    connect(model(), SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(slotModelRowsInserted(QModelIndex,int,int)));
    connect(model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(slotModelRowsRemoved(QModelIndex,int,int)));

    connect(listWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slotModelItemChanged(QListWidgetItem*)));
}


ComboCheckBox::~ComboCheckBox()
{
    delete listWidget;
}

QStringList ComboCheckBox::checkedItems() const
{
    return mCheckedItems;
}

void ComboCheckBox::setCheckedItems(const QStringList &items)
{
    blockSignals(true);
    for (int i = 0; i < items.count(); ++i)
    {
        int index = findText(items.at(i));

        if (index != -1)
        {
            listWidget->item(index)->setCheckState(Qt::Checked);
        }
    }
    blockSignals(false);
    collectCheckedItems();
}

void ComboCheckBox::paintEvent(QPaintEvent *event)
{
    (void)event;

    QStylePainter painter(this);

    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox option;

    initStyleOption(&option);

    painter.drawComplexControl(QStyle::CC_ComboBox, option);

    QRect textRect = rect().adjusted(mDisplayRectDelta.left(), mDisplayRectDelta.top(), mDisplayRectDelta.right(), mDisplayRectDelta.bottom());

    painter.drawText(textRect, Qt::AlignVCenter, mDisplayText);
}

void ComboCheckBox::resizeEvent(QResizeEvent *event)
{
    (void)event;

    updateDisplayText();
}

void ComboCheckBox::collectCheckedItems()
{
    mCheckedItems.clear();

    for (int i = 0; i < count(); ++i)
    {
        if (listWidget->item(i)->checkState() == Qt::Checked)
        {
            mCheckedItems.push_back(listWidget->item(i)->text());
        }
    }

    updateDisplayText();

    repaint();
}

void ComboCheckBox::updateDisplayText()
{
    QRect textRect = rect().adjusted(mDisplayRectDelta.left(), mDisplayRectDelta.top(), mDisplayRectDelta.right(), mDisplayRectDelta.bottom());

    QFontMetrics fontMetrics(font());

    mDisplayText = mCheckedItems.join(", ");

    if (fontMetrics.size(Qt::TextSingleLine, mDisplayText).width() > textRect.width())
    {
        while (mDisplayText != "" && fontMetrics.size(Qt::TextSingleLine, mDisplayText + "...").width() > textRect.width())
        {
            mDisplayText.remove(mDisplayText.length() - 1, 1);
        }

        mDisplayText += "...";
    }
}

void ComboCheckBox::slotModelRowsInserted(const QModelIndex &parent, int start, int end)
{
    (void)parent;

    blockSignals(true);

    for (int i = start; i <= end; ++i)
    {
        listWidget->item(i)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        listWidget->item(i)->setCheckState(Qt::Unchecked);
    }

    blockSignals(false);
}

void ComboCheckBox::slotModelRowsRemoved(const QModelIndex &parent, int start, int end)
{
    (void)parent;
    (void)start;
    (void)end;

    collectCheckedItems();
}

void ComboCheckBox::slotModelItemChanged(QListWidgetItem *item)
{
    (void)item;

    collectCheckedItems();
}

