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

#include "customdelegates.h"
#include "resources.h"
#include "multilist.h"
#include <QPainter>
#include <QDebug>
#include <QTime>
#include <QDateTimeEdit>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QApplication>
#include <QtMath>
#include <QListWidget>

////////////////////////////////////////////////////////////////////////////
/// для того чтобы в таблице место под ячейки резервировалось адекватно, ///
/// необходимо учесть не только размер текста, но и размер декораций     ///
/// различных редакторов. Следующая напоминалка должна помочь.           ///
///                                                                      ///
/// пока я разленился и сделал всё через option.decorationSize,          ///
/// но это не достаточно точно будет работать, т.к. хз какие темы с      ///
/// какими масштабами какие соотношения отступов и прочего будут давать  ///
////////////////////////////////////////////////////////////////////////////
/*
    // dpi = 96   font = Noto Sans   size = 11   KDE theme = Breeze
    qDebug()<<"text size "<<ss; // xx, 22
    qDebug()<<"decoration size "<<option.decorationSize; // 16,16
    qDebug()<<"check box label "<<QApplication::style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing); // 6
    qDebug()<<"small icon "<<QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize); // 16
    qDebug()<<"SliderThick="<<QApplication::style()->pixelMetric(QStyle::PM_SliderThickness); //15
    qDebug()<<"indicatorWidth="<<QApplication::style()->pixelMetric(QStyle::PM_IndicatorWidth); // 14
    qDebug()<<"SpinBoxFrameWitdh="<<QApplication::style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth); // 3
    qDebug()<<"ComboBoxFrameWitdh="<<QApplication::style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth); // 1
    qDebug()<<"splitterWidth="<<QApplication::style()->pixelMetric(QStyle::PM_SplitterWidth); // 4
    qDebug()<<"ScrollBarExtent="<<QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent); // 14
    qDebug()<<"ListViewIconSize="<<QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize); // 24
    qDebug()<<"CheckBoxLabelSpacing="<<QApplication::style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing); // 6
    qDebug()<<"RadioButtonLabelSpacing="<<QApplication::style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing); // 6
*/

///////////////////////////////
///  DoubleSpinboxDelegate   //
///////////////////////////////

DoubleSpinboxDelegate::DoubleSpinboxDelegate(double minValue, double maxValue, int decimalDigits, QObject *parent):QStyledItemDelegate(parent)
{
    maxVal = maxValue;
    minVal = minValue;
    DecDigits = decimalDigits;
}

QDoubleSpinBox* DoubleSpinboxDelegate::doubleSpinboxSizehintStaticInstance=nullptr; // пока оставлю, может пригодиться

QSize DoubleSpinboxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ss;
    QString str=QString::number(maxVal*qPow(10,DecDigits))+"-."; // знакоместо под минус,точку и разряды и кнопочки виджета редактирования и отступы рамочки
    ss=option.fontMetrics.size(0,str);
    ss.operator+=(QSize(option.decorationSize.width()*1.2,2)); // вполне работает. Немного ползает в зависимости от масштаба экрана, но не сильно.
    return ss;
}

void DoubleSpinboxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i = index.data().toInt();
    QString str=QString::number((double)i/qPow(10,DecDigits));
    painter->drawText(option.rect,str,QTextOption(option.decorationAlignment));
}

QWidget *DoubleSpinboxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    qDebug()<<"-> create DoubleSpinBox editor...";

    (void)option;
    (void)index;
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setMinimum(minVal);
    editor->setMaximum(maxVal);
    editor->setDecimals(DecDigits);

    return editor;
}

void DoubleSpinboxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    //qDebug()<<"delegate set editor data"<<editor;

    double value =index.model()->data(index, Qt::EditRole).toInt();
    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->setValue((double)value/qPow(10,DecDigits));
}

void DoubleSpinboxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    //qDebug()<<"delegate set model data"<<editor;

    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->interpretText();
    int value = round(spinbox->value()*qPow(10,DecDigits));
    model->setData(index, value);
    //qDebug()<<"model data set end";
}

void DoubleSpinboxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
    (void)index;
}


///////////////////////////
///   SpinboxDelegate    //
///////////////////////////

SpinboxDelegate::SpinboxDelegate(int integerBase, int minValue, int maxValue, bool showButtons, QObject *parent):QStyledItemDelegate(parent)
{
    maxVal = maxValue;
    minVal = minValue;
    base = integerBase;
    buttons = showButtons;
}

QSize SpinboxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ss;
    QString str=QString::number(maxVal)+"-"; // знакоместо под минус
    ss=option.fontMetrics.size(0,str);
    ss.operator+=(QSize(option.decorationSize.width()*1.2,2)); // вполне работает. Немного ползает в зависимости от масштаба экрана, но не сильно.
    return ss;
}

QWidget *SpinboxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    (void)option;
    (void)index;
    QSpinBox *editor = new QSpinBox(parent);
    editor->setMinimum(minVal);
    editor->setMaximum(maxVal);
    editor->setDisplayIntegerBase(base);

    if(!buttons){
        editor->setButtonSymbols(QAbstractSpinBox::NoButtons);
        editor->setSingleStep(0);
    }
    return editor;
}

void SpinboxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i = index.data().toInt();
/*
//это пока не надо удалять т.к. оно может потом понадобиться в качестве примера
    QString str;
    int dat=0;
    if(index.model()->hasIndex(index.row(),index.column()-1,index.parent()))
        dat=index.model()->data(index.model()->index(index.row(),index.column()-1,index.parent())).toInt();
    switch(dat){
        case 1:
        case 2:
        case 3: str=QString::number((double)i/10);break;
        case 4:
        case 5: str=QString::number((double)i/100);break;
        case 6: str=QTime(0,0,0).addSecs(i).toString("hh:mm:ss"); break;
        case 0:
        default: str=QString::number(i,base);break;
        //str=QString::number(i);
    }
*/
    QString str=QString::number(i,base);
    painter->drawText(option.rect,str,QTextOption(option.decorationAlignment));
}

void SpinboxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value =index.data().toInt(0);
    QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
    spinbox->setValue(value);
}

void SpinboxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
    spinbox->interpretText();
    int value = spinbox->value();
    model->setData(index, value);
}

void SpinboxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
    (void)index;
}


//////////////////////////
///  CheckBoxDelegate   //
//////////////////////////

CheckBoxDelegate::CheckBoxDelegate(QObject *parent):QStyledItemDelegate(parent)
{

}

QSize CheckBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ss;
    QString str = index.data(Qt::DisplayRole).toString();
    ss=option.fontMetrics.size(0,str);
    ss.operator+=(QSize(option.decorationSize.width()*1.2,2)); // вполне работает. Немного ползает в зависимости от масштаба экрана, но не сильно.
    return ss;
}

void CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //Get item data
    bool value = index.data(Qt::UserRole).toBool();
    QString text = index.data(Qt::DisplayRole).toString();

    // fill style options with item data
    const QStyle *style = QApplication::style();
    QStyleOptionButton opt;
    opt.state |= value ? QStyle::State_On : QStyle::State_Off;
    opt.state |= QStyle::State_Enabled;
    opt.text = text;
    opt.rect = option.rect;

    // draw item data as CheckBox
    style->drawControl(QStyle::CE_CheckBox,&opt,painter);
}

QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    (void)option;
    (void)index;

    QCheckBox *checkbox = new QCheckBox(parent);
    return checkbox;
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
    myEditor->setText(index.data(Qt::DisplayRole).toString());
    myEditor->setChecked(index.data(Qt::UserRole).toBool());
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
    bool value = myEditor->isChecked();

    //set model data
    QMap<int,QVariant> data;
    data.insert(Qt::DisplayRole,myEditor->text());
    data.insert(Qt::UserRole,value);
    model->setItemData(index,data);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

    (void)index;
}


//////////////////////////
///  ComboBoxDelegate   //
//////////////////////////


ComboBoxDelegate::ComboBoxDelegate(QStringList stringList, QObject *parent):QStyledItemDelegate(parent)
{
    list=stringList;
}

QSize ComboBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // тут надо сделать актуальную ширину текущего выбора? кажется не логичным, т.к. есть редактор выбора и будет не красиво, если он будет обрезан.
    // значит надо выбрать самую длинную из строк (и взять запас в 20пикселов как всегда).
    QSize ss;
    foreach(QString str, list){
        QSize s=option.fontMetrics.size(0,str);
        if(ss.width() < s.width()) ss=s;
    }
    ss.operator+=(QSize(option.decorationSize.width()*1.2,2)); // вполне работает. Немного ползает в зависимости от масштаба экрана, но не сильно.
    return ss;
}

void ComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i = index.data().toInt();
    QString str;
    if((i<list.count())&&(i>=0)) str=list.at(i);

    painter->drawText(option.rect,str,QTextOption(option.decorationAlignment));
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    (void)option;
    (void)index;
    QComboBox *editor = new QComboBox(parent);
    editor->addItems(list);
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int i = index.data().toInt();
    QComboBox *combobox = static_cast<QComboBox*>(editor);
    combobox->setCurrentIndex(i);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combobox = static_cast<QComboBox*>(editor);
    int i=0;
    if(combobox->count())i=combobox->currentIndex(); // если список пуст (count=0), вылетает ASSERT failure in QList<T>::at: "index out of range"
    model->setData(index, i);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

    (void)index;
}


//////////////////////////
///  FlagBoxDelegate   //
//////////////////////////


FlagBoxDelegate::FlagBoxDelegate(QStringList stringList, QObject *parent):QStyledItemDelegate(parent)
{
    list=stringList;
}

QString FlagBoxDelegate::text4paint(int flags) const
{
    QString str;
    int mask=1;
    for(int i=0;i<list.count();i++){
        if(flags&mask){
            if(str.count()>0)str+=", ";
            str+=list.at(i);
        }
        mask<<=1;
    }
    return str;
}

QSize FlagBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // ммм есть вариант сравнивать актуальную ширину содержимого с переносом строк (как сосчитать кол-во строк для переноса?)
    // и самой длинной отдельной строки аналогично ComboBox, чтобы редактор не обрезало по краям...
    QSize editorSize, textSize;

    foreach(QString str, list){
        QSize s=option.fontMetrics.size(0,str);
        if(editorSize.width() < s.width()) editorSize=s;
    }
    editorSize.operator+=(QSize(option.decorationSize.width()*3,2));

    if(Resources::EXPAND_FLAGBOXLIST){
        // определяем размер надписи, пробуем вписать его в двойную ширину подписи?
        QString str = text4paint(index.data().toInt());
        textSize=option.fontMetrics.boundingRect(QRect(0,0,editorSize.width()*1.6,editorSize.height()),Qt::TextWordWrap,str).size();
        editorSize = editorSize.expandedTo(textSize);
    }
    return editorSize;
}

void FlagBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString str = text4paint(index.data().toInt());
    painter->drawText(option.rect,str,QTextOption(Qt::AlignHCenter));//option.decorationAlignment
}

QWidget *FlagBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    (void)option;
    (void)index;

    ComboCheckBoxWidget *editor = new ComboCheckBoxWidget(parent);
    editor->addItems(list);

    return editor;
}

void FlagBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int flags = index.data().toInt();
    ComboCheckBoxWidget *comboflagbox = static_cast<ComboCheckBoxWidget*>(editor);
    // каждый бит flags соответствует установленному состоянию checked списка в comboflagbox
    // изза особенностей ComboCheckBoxWidget придётся немного странно устанавливать флаги
    int mask=1;
    QStringList checkedList;
    for(int i=0;i<comboflagbox->count();i++){
        bool checked = mask&flags;
        if(checked) checkedList<<list.at(i);
        mask=mask<<1;
    }
    comboflagbox->setCheckedItems(checkedList);
}

void FlagBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ComboCheckBoxWidget *comboflagbox = static_cast<ComboCheckBoxWidget*>(editor);
    int flags=0;
    int mask=1;
    for(int i=0;i<comboflagbox->count();i++){
        if(comboflagbox->itemData(i,Qt::CheckStateRole).toBool()) flags|=mask;
        mask<<=1;
    }
    //if(combobox->count())i=combobox->currentIndex(); // если список пуст (count=0), вылетает ASSERT failure in QList<T>::at: "index out of range"
    model->setData(index, flags);
}

void FlagBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

    (void)index;
}


//////////////////////////
//   EditTimeDelegate   //
//////////////////////////


TimeEditDelegate::TimeEditDelegate(int minTime, int maxTime, QObject *parent, TimeUnits units):QStyledItemDelegate(parent)
{
    min=minTime;
    if(min<0)min=0;
    max=maxTime;
    value_units=units;
    switch(value_units){
    case hour:format="hh";
        if(max>23)max=23;break;
    case minute:format="hh:mm";
        if(max>1439)max=1439;break;
    case msec:format="ss:zzz";
        if(max>59999)max=59999;break;
    case sec:
    default: format="hh:mm:ss";
        if(max>86399)max=86399;
    }
}

QSize TimeEditDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ss;
    QString str=QTime(0,0,0).toString(format);
    ss=option.fontMetrics.size(0,str);
    ss.operator+=(QSize(option.decorationSize.width()*1.6,2)); // вполне работает. Немного ползает в зависимости от масштаба экрана, но не сильно.
    return ss;
}

QWidget *TimeEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{ // , const QString &format="hh:mm:ss"
    (void)option;
    (void)index;
    QDateTimeEdit *editor=new QDateTimeEdit(parent);    
    editor->setDisplayFormat(format);
    editor->setMinimumDate(QDate(0,1,1));
    editor->setMaximumDate(QDate(0,1,1));
    switch(value_units){
    case hour:
        editor->setMinimumTime(QTime(min,0,0)); // очень сомнительно как то выглядит
        editor->setMaximumTime(QTime(max,0,0));
        break;
    case minute:
        editor->setMinimumTime(QTime(0,0,0).addSecs(min*60)); // а это просто не удобно...
        editor->setMaximumTime(QTime(0,0,0).addSecs(max*60));
        //qDebug()<<" max time "<<max<<"   "<<editor->maximumTime();
        break;
    case msec:
        editor->setMinimumTime(QTime(0,0,0).addMSecs(min));
        editor->setMaximumTime(QTime(0,0,0).addMSecs(max));
        break;
    case sec:
    default:
        editor->setMinimumTime(QTime(0,0,0).addSecs(min));
        editor->setMaximumTime(QTime(0,0,0).addSecs(max));
        break;
    }
    // ??? зачем предыдущий switch-case, если потом всё равно устанавливается
    //editor->setMinimumTime(QTime(0,0,0).addSecs(min));
    //editor->setMaximumTime(QTime(0,0,0).addSecs(max));
    //editor->set Интервал скролирования?
    return editor;
}

void TimeEditDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int value = index.data().toInt();
    QTime time(0,0,0);
    switch(value_units){
    case hour:time.setHMS(value,0,0);break; // очень сомнительно как то выглядит
    case minute:time=time.addSecs(value*60);break; // а это просто не удобно...
    case sec:time=time.addSecs(value);break;
    case msec:time=time.addMSecs(value);break;
    }
    QString str=time.toString(format);
    painter->drawText(option.rect,str,QTextOption(option.decorationAlignment));
}

void TimeEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value = index.data().toInt();

    QTime time(0,0,0);
    switch(value_units){
    case hour:time.setHMS(value,0,0);break; // очень сомнительно как то выглядит
    case minute:time=time.addSecs(value*60);break; // а это просто не удобно...
    case sec:time=time.addSecs(value);break;
    case msec:time=time.addMSecs(value);break;
    }
    QTimeEdit *timeedit = static_cast<QTimeEdit*>(editor);
    timeedit->setTime(time);
}

void TimeEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTimeEdit *timeedit = static_cast<QTimeEdit*>(editor);
    QTime time=timeedit->time();
    int value;
    switch(value_units){
    case hour:value=time.hour();break; // очень сомнительно как то выглядит
    case minute:value=QTime(0,0,0).secsTo(time)/60;break; // а это просто не удобно...
    case sec:value=QTime(0,0,0).secsTo(time);break;
    case msec:value=QTime(0,0,0).msecsTo(time);break;
    }
    model->setData(index, value);
}

void TimeEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

    (void)index;
}



//////////////////////////
//   VaribleDelegate   //
//////////////////////////



VaribleDelegate::VaribleDelegate(int definingColumn, const QStringList &cellTypes,
                                 QList<QStringList> *textLists, QObject *parent):QStyledItemDelegate(parent)
{
    delegateIndex=new int;
    columnIndex=definingColumn;

    int minval=0;
    int maxval=32000;

    //qDebug()<<cellTypes;
    foreach(QString type, cellTypes){
        // я пока не понимаю что надо делать с мин-макс. Возможно придётся как-то вносить эти величины в строку типа.
        QStringList typesplit=type.split(":");
        //qDebug()<<type;
        QStyledItemDelegate *delegat=nullptr;

        if(typesplit.at(0)==Consts::intType){
            int decimals=0;
            if(typesplit.count()>1)decimals=typesplit.at(1).toInt();
            delegat = new DoubleSpinboxDelegate(minval/qPow(10,decimals),maxval/qPow(10,decimals), decimals,parent);
        }
        else if(typesplit.at(0)==Consts::binType){
            delegat = new SpinboxDelegate(2,minval,maxval,false,parent);
        }
        else if(typesplit.at(0)==Consts::octType){
            delegat = new SpinboxDelegate(8,minval,maxval,true,parent);
        }
        else if(typesplit.at(0)==Consts::hexType){
            delegat = new SpinboxDelegate(16,minval,maxval,true,parent);
        }
        else if(typesplit.at(0)==Consts::timeType){
            TimeUnits units=sec;
            if(typesplit.count()>1){
                if(typesplit.at(1)=="hour") units=hour;
                else if(typesplit.at(1)=="minute") units=minute;
                if(typesplit.at(1)=="msec") units=msec;
                // а секунды по умолчанию
            }
            delegat = new TimeEditDelegate(minval,maxval,parent,units);
        }
        else if(typesplit.at(0)==Consts::textListType){
            int listnum=0;
            QStringList list;
            if(typesplit.count()>1)listnum=typesplit.at(1).toInt();
            if(textLists)
            if(textLists->count()>listnum)list<<textLists->at(listnum);
            delegat = new ComboBoxDelegate(list,parent);
        }
        else if(typesplit.at(0)==Consts::floatType){
            // пока не реализовал делегата.
        }
        else if(typesplit.at(0)==Consts::stringType){
            // пока не реализовал делегата.
        }
        else  delegat = new SpinboxDelegate(10,minval,maxval,true,parent);

        delegates<<delegat;
    }
    // дефолтный делегат, чтобы не возникло ошибок в работе.
    if(delegates.count()<=0) delegates<< new SpinboxDelegate(10,minval,maxval,true,parent);
}

VaribleDelegate::~VaribleDelegate()
{
    delete delegateIndex;
}

QSize VaribleDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // я хз как правильно. Видимо надо перебрать все варианты делегатов и выбрать самый большой.
    QSize ss;
    foreach(QStyledItemDelegate *it, delegates){
        ss=ss.expandedTo(it->sizeHint(option,index));
    }
    return ss;
    //delegates.at(delegateIndex[0])->sizeHint(option,index);
}

QWidget *VaribleDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // в зависимости от значения в соседней(или указанной) ячейке выбираем тип делегата для редактирования
    // и отображения в данной ячейке. Если значение выходит за пределы, то выбираем базовый(первый) делегат.
    int i=0;
    if(index.model()->hasIndex(index.row(),columnIndex,index.parent()))
        i=index.model()->data(index.model()->index(index.row(),columnIndex,index.parent())).toInt();
    if(i>=delegates.count())i=0;

    delegateIndex[0]=i;
    return delegates.at(i)->createEditor(parent,option,index);
}

void VaribleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i=0;
    if(index.model()->hasIndex(index.row(),columnIndex,index.parent()))
        i=index.model()->data(index.model()->index(index.row(),columnIndex,index.parent())).toInt();

    //qDebug()<<"paint "<<index.column()<<"  "<< columnIndex<< "  "<<i;
    if(i>=delegates.count())i=0;
    delegates.at(i)->paint(painter,option,index);
}

void VaribleDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int i=delegateIndex[0];
    //qDebug()<<"setEditor "<<i;

    if(delegates.count()>i)
        delegates.at(i)->setEditorData(editor,index);
}

void VaribleDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int i=delegateIndex[0];
    //qDebug()<<"setModel "<<i;

    if(delegates.count()>i)
        delegates.at(i)->setModelData(editor,model,index);
}

void VaribleDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int i=delegateIndex[0];

    if(delegates.count()>i)
        delegates.at(i)->updateEditorGeometry(editor,option,index);
}
