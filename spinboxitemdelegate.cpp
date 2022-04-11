#include "spinboxitemdelegate.h"
#include <QPainter>
#include <QDebug>

SpinboxItemDelegate::SpinboxItemDelegate(double minValue, double maxValue, int decimalDigits, QObject *parent):QItemDelegate(parent)
{
    maxVal = maxValue;
    minVal = minValue;
    DecDigits = decimalDigits;
}

void SpinboxItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i = index.data().toInt();
    QString str=QString::number((double)i/pow10(DecDigits));
    painter->drawText(option.rect,str,QTextOption(option.decorationAlignment));
}

QWidget *SpinboxItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setMinimum(minVal);
    editor->setMaximum(maxVal);
    editor->setDecimals(DecDigits);
    return editor;
}

void SpinboxItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qDebug()<<"delegate set editor data"<<editor;
    double value =index.model()->data(index, Qt::EditRole).toInt();
    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->setValue((double)value/pow10(DecDigits));
}

void SpinboxItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    qDebug()<<"delegate set model data"<<editor;
    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->interpretText();
    int value = round(spinbox->value()*pow10(DecDigits));
    model->setData(index, value);
    qDebug()<<"model data set end";
}

void SpinboxItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

