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

#include <QHeaderView>
#include <QtMath>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QResizeEvent>
#include <QPalette>
#include <QFile>
#include <QSizePolicy>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include "viewelement.h"
#include "customdelegates.h"
#include "resources.h"


  /*****************************/
 /*  Реализация  ViewElement  */
/*****************************/

int ViewElement::defaultHeight=0;
QFont ViewElement::defaultFont=QFont();

void ViewElement::view()
{

}

PlcRegister ViewElement::getPLCregister()
{
    return plcregister;
}

ViewElement::ViewElement(const QString &cap, const PlcRegister &plcreg, bool vertical, QWidget *parent): QWidget(parent)
{
    setToolTip(plcreg.toString()); // хинт, показывающий адрес регистра при наведении мышки. ??? Возможно нужен только для отладки и надо сделать вкл/выкл в настройках.

    if(cap.isEmpty()) vertic = false; // пробуем убрать возможные перекосы и смещения врезультате разной обработки вертикальной и горизонтальной раскладок при отсутствии подписи
    else vertic = vertical;

    if(vertic){
        this->setLayout(new QVBoxLayout);
    }
    else{
        this->setLayout(new QHBoxLayout);
    }

    if(cap.length()>0){
        caption=new QLabel(cap);
        caption->setFont(getDefaultCaptionFont());
        //caption->setWordWrap(true); // ??? надо ли так? вот я пока сомневаюсь.
        caption->setAlignment(Qt::AlignCenter); // центровка текста на виджете caption
        layout()->addWidget(caption);
        layout()->setAlignment(caption,Qt::AlignCenter); // центровка виджета caption. В ряде случаев требуется подправить в конструкторе потомка?
    }
    if(vertical) layout()->setSpacing(getDefaultSpacing()*0.5); //0.2?
    else layout()->setSpacing(getDefaultSpacing());
    layout()->setMargin(getDefaultSpacing()*0.5);

    setFont(getDefaultFont());

    plcregister = plcreg;
    color = Qt::black;
    value = 0;
}

ViewElement::~ViewElement()
{
    if(caption) delete caption; // если caption не будет помещён на layout, то он не сможет удалиться.
}

void ViewElement::placeWidgetOnLayout(QWidget* widget)
{
    layout()->addWidget(widget);

    if(QString(layout()->metaObject()->className())=="QVBoxLayout"){
        layout()->setAlignment(widget, Qt::AlignBottom|Qt::AlignCenter);
    }
    else {
        if(caption)
            layout()->setAlignment(widget, Qt::AlignRight|Qt::AlignVCenter);
        else
            layout()->setAlignment(widget, Qt::AlignCenter);
    }
}

void ViewElement::adjustCaption()
{
    // не всё размещается как хотелось бы, но в целом работает.

    if(caption){
        // определяем шрифт подписи и шрифт для wordwrap
        if(!captionFontSizeIsSet) caption->setFont(getDefaultCaptionFont());
        QFont wwfont=caption->font();
        if(!captionFontSizeIsSet) wwfont.setPointSize(wwfont.pointSize()*0.9);
        Qt::Alignment align;

        if(!canExpand){ // обычный однострочный элемент
            // задаем размеры
            if(vertic){ // подпись сверху
                if(!captionFixedH){
                    // устанавливаем мин-макс высоту при вертикальном размещении
                    caption->setMinimumHeight(0);//QFontMetrics(wwfont).height());
                    caption->setMaximumHeight(freeExpand);//QFontMetrics(wwfont).height()*2); // << перенос по какой-то причине хуй кладет на эти размеры и переносит на 3 строчки обрезая при этом буквы.
                }
                else align|=Qt::AlignVCenter;
                if(!captionFixedW){
                    // устанавливаем мин-макс ширину при вертикальном размещении
                    caption->setMinimumWidth(0);
                    if(elementWidth>0)caption->setMaximumWidth(elementWidth*2);
                    else if(elementHeight>0)caption->setMaximumWidth(elementHeight*4); // << перенос по какой-то причине хуй кладет на эти размеры и переносит на 3 строчки обрезая при этом буквы.
                        else caption->setMaximumWidth(getDefaultHeight()*4);
                }
                else align|=Qt::AlignHCenter;
            }
            else{ // подпись слева
                if(!captionFixedH){
                    // устанавливаем мин-макс высоту при горизонтальном размещении
                    caption->setMinimumHeight(0);//QFontMetrics(wwfont).height());
                    if(elementHeight>0)caption->setMaximumHeight(elementHeight);
                    else caption->setMaximumHeight(getDefaultHeight());
                }
                else align|=Qt::AlignVCenter;
                if(!captionFixedW){
                    // устанавливаем мин-макс ширину при горизонтальном размещении
                    caption->setMinimumWidth(0);
                    if(elementWidth>0)caption->setMaximumWidth(elementWidth*2);
                    else if(elementHeight>0)caption->setMaximumWidth(elementHeight*5);
                        else caption->setMaximumWidth(getDefaultHeight()*5);
                }
                else align|=Qt::AlignHCenter;
            }

            // проверяем вмещается ли текст по ширине, и если нет, включаем перенос и задаем уменьшеный шрифт
            QRect rect=caption->fontMetrics().boundingRect(caption->text());
            if(rect.width()>caption->maximumWidth()){
                caption->setFont(wwfont);
                //caption->setMinimumSize(caption->maximumSize()/2);
                if(!captionWordWrapIsSet) caption->setWordWrap(true);
            }
            else{
                if(!captionFixedW) caption->setMinimumWidth(rect.width()+2);
                if(!captionWordWrapIsSet) caption->setWordWrap(false);
            }
        }
        else { /// if(canExpand)
            // т.к. подпись растягиваемая, то надо определить размещение текста на подписи
            caption->setAlignment(Qt::AlignTop|Qt::AlignLeft);

            // определяем размеры подписи
            // (пока подпись будет растягиваться вне зависимости от размеров элемента,
            // но может надо было бы сделать ограничение по ширине/высоте при elementH/W>0?)
            if(!captionFixedH){
                caption->setMinimumHeight(0);
                caption->setMaximumHeight(freeExpand);
            }
            if(!captionFixedW){
                caption->setMinimumWidth(0);
                caption->setMaximumWidth(freeExpand);
            }

            // определяем размещение подписи
            if(vertic){
                if(captionFixedH) align|=Qt::AlignVCenter;
                if(captionFixedW) align|=Qt::AlignLeft;
            }
            else{
                if(captionFixedH) align|=Qt::AlignTop;
                if(captionFixedW) align|=Qt::AlignHCenter;
            }
        }
        // задаем размещение
        layout()->setAlignment(caption, align);
    }/// if(caption)
}

void ViewElement::setValue(const QString &val, const PlcRegister &plcreg, int base)
{
    bool ok;
    newValReg=plcreg;
    int v = val.toInt(&ok,base);
    if(ok){
        value = v;
        view();
    }
}

void ViewElement::setValue(int val, const PlcRegister &plcreg)
{
    newValReg=plcreg;
    value = val;
    view();
}

void ViewElement::setColor(QColor col)
{
    color=col;
}

void ViewElement::setCaptionColor(QColor col)
{
    QPalette pal(caption->palette());
    pal.setColor(QPalette::WindowText, col);
    caption->setPalette(pal);
}

void ViewElement::setFixedHeight(int h)
{
    if(h>0) elementHeight=h;
    else elementHeight=0;

    // т.к. надо сделать чтобы стандартные элементы собраные в 2 столбика смотрелись рядом,
    // устанавливаем фиксированную высоту не только элемента, но и всего виджета: (только для горизонтальной раскладки)
    if(vertic && caption){ // !!! обрабатываем отсутствие подписи как горизонтальную раскладку, это в данном случае избыточно, т.к. уже есть в конструкторе viewelement
        QWidget::setMinimumHeight(0);
        QWidget::setMaximumHeight(freeExpand);
    }
    else
        if(elementHeight>0){ // фиксированная высота
            QWidget::setFixedHeight(elementHeight);
        }
        else  // растягиваемающаяся высота
            if(canExpand){
                QWidget::setMinimumHeight(0);
                QWidget::setMaximumHeight(freeExpand);
            }
            else QWidget::setFixedHeight(getDefaultHeight());

    adjustCaption();
    adjustChildrenSize();
}

void ViewElement::setHeight(int h)
{
    setFixedHeight(h);
}

void ViewElement::setFixedWidth(int w)
{
    if(w>0) elementWidth=w;
    else elementWidth=0;

    adjustCaption(); // подпись к стандартным элементам ограчичивается в размерах исходя из elementW/H...
    adjustChildrenSize();
}

void ViewElement::setWidth(int w)
{
    setFixedWidth(w);
}

void ViewElement::setFontSize(int pointSize)
{
    if(pointSize>0){
        QFont fnt = font();
        fnt.setPointSize(pointSize);
        setFont(fnt);
        adjustChildrenSize();
    }
}

void ViewElement::setCaptionFontSize(int pointSize)
{
    if(caption){
        if(pointSize>0){
            QFont fnt = caption->font();
            fnt.setPointSize(pointSize);
            caption->setFont(fnt);
            captionFontSizeIsSet=true;
        }
        else {
            // caption->setFont(font()); // надо ли это здесь? или это будет в adjustCaption?
            captionFontSizeIsSet=false;
        }
        adjustCaption(); // подгоняем размер подписи
    }
}

void ViewElement::setCaptionWordWrap(bool wwrap)
{
    if(caption){
        caption->setWordWrap(wwrap);
        captionWordWrapIsSet=true;
        adjustCaption(); // подгоняем размер подписи
    }
}

void ViewElement::setCaptionHeight(int h)
{
    if(caption){
        if(h>0){
            captionFixedH=true;
            caption->setFixedHeight(h);
        }
        else {
            captionFixedH=false;
            // надо ли тут устанавливать min-max размер или всё будет в adjustCap.?
            adjustCaption();
        }
    }
}

void ViewElement::setCaptionWidth(int w)
{
    if(caption){
        if(w>0){
            captionFixedW=true;
            caption->setFixedWidth(w);
        }
        else {
            captionFixedW=false;
            // надо ли тут устанавливать min-max размер или всё будет в adjustCap.?
            adjustCaption();
        }
    }
}

void ViewElement::setCaptionAlighnment(Qt::Alignment alignment)
{
    if(caption) layout()->setAlignment(caption, alignment);
}

  /*****************************/
 /*  Реализация  NumericView  */
/*****************************/

NumericView::NumericView(const QString &cap, const PlcRegister &plcreg, int decDigits,
                         bool vertical, QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    decimalDigits = decDigits; // остальное инициируется в конструкторе родителя.
    divider = qPow(10, decimalDigits);

    numView=new QLabel();
    QPalette pal(numView->palette());
    pal.setColor(QPalette::WindowText, color);
    pal.setColor(QPalette::Window, pal.color(QPalette::Base));

    numView->setAlignment(Qt::AlignCenter);

    numView->setPalette(pal);
    numView->setAutoFillBackground(true);
    numView->setFrameShape(QFrame::Panel);
    numView->setFrameShadow(QFrame::Sunken);
    numView->setLineWidth(1);
    /* в последствии необходимо настроить отступы и прочие тонкости размещения */

    placeWidgetOnLayout(numView);
    setFixedHeight(getDefaultHeight());

    this->setValue(value); // отобразить хоть что-то.
}

void NumericView::setColor(QColor col)
{
    color=col;
    QPalette pal(numView->palette());
    pal.setColor(QPalette::WindowText, color);
    numView->setPalette(pal);
}

void NumericView::view()
{
    //int div = (int) qPow(10, decimalDigits);
    numView->setText(QString::number(value/divider,'f',decimalDigits));
}

void NumericView::adjustChildrenSize()
{
    ViewElement::adjustSize();

    QFont fnt=this->font();
    fnt.setPointSize(fnt.pointSize()*1.15);
    numView->setFont(fnt);
    int h = elementHeight;
    h=h-getDefaultSpacing()*1.5;
    int w=h*3;
    if(elementWidth) w=elementWidth;

    numView->setFixedHeight(h);
    numView->setFixedWidth(w);
}


  /************************/
 /* Описание NumericEdit */
/************************/

NumericEdit::NumericEdit(const QString &cap, const PlcRegister &plcreg, int min, int max, int decDigits,
                         bool vertical, QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    decimalDigits = decDigits; // остальное инициируется в конструкторе родителя.
    divider = qPow(10, decimalDigits);

    numEdit.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    numEdit.setMinimum(min/divider);
    numEdit.setMaximum(max/divider);
    numEdit.setDecimals(decDigits);

    numEdit.installEventFilter(this);
    QObject::connect(&numEdit, SIGNAL(valueChanged(double)), this, SLOT(changeValue(double)));
    QObject::connect(&valChangeTimer,&QTimer::timeout,this,&NumericEdit::onChangeValueTimer);
    valChangeTimer.setSingleShot(true);

    // надо ли заменить это одной строкой setColor(color)?
    // С точки зрения количества выполняемых бессмысленных операций - нет.
    // С точки зрения унификации кода - да.
    QPalette pal(numEdit.palette());
    pal.setColor(QPalette::Text, color);
    numEdit.setPalette(pal);

    placeWidgetOnLayout(&numEdit);
    setFixedHeight(getDefaultHeight());

    this->setValue(value); // отобразить хоть что-то.
}

void NumericEdit::setColor(QColor col)
{
    color=col; // данная строка в нынешнем виде бессмысленна. Но если её убрать, то в результате поле color будет содержать ничему не соответствующую информацию, что ещё более бессмысленно.
    QPalette pal(numEdit.palette());
    pal.setColor(QPalette::Text, color);
    numEdit.setPalette(pal);
}

void NumericEdit::changeValue(double newVal)
{
    // тут запустить/перезапустить таймер (и вместе с ним блокировку обновления на 5 сек)
    // к сожалению придется запускать таймер еще и по нажатию клавишь, т.к. не все введенные символы
    // запускают сигнал valueChanged. А еще редактирование может быть не только с клавиатуры,
    // поэтому ЭТУ обработку тоже не пересунешь в eventFilter. Так что будет по уродски 2 раза таймер перезапускаться...

    editedValue = newVal*divider;
    valChangeTimer.start(5000);
}

void NumericEdit::onChangeValueTimer()
{
    if(value!=editedValue){
        value=editedValue;
        emit valueChanged(getPLCregister(), value);
    }
}

void NumericEdit::view()
{
    if(valChangeTimer.isActive()) return; // это необходимо, чтобы обмен с контроллером не мешал редактировать цифру

    numEdit.blockSignals(true);
    numEdit.setValue((double)value/divider);
    editedValue=value; // чтобы избежать рассинхронизации и как следствие ненужных сообщений.
    numEdit.blockSignals(false);
}

void NumericEdit::adjustChildrenSize()
{
    ViewElement::adjustSize();

    QFont fnt=this->font();
    fnt.setPointSize(fnt.pointSize()*1.15); // ?? почему я его сделал увеличенным? как то не смотрится местами
    numEdit.setFont(fnt);

    int h=elementHeight;
    h=h-getDefaultSpacing();

    int w=h*3;
    if(elementWidth) w=elementWidth;

    numEdit.setFixedHeight(h);
    numEdit.setFixedWidth(w);
}

bool NumericEdit::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::KeyPress){
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
        if(keyEvent->key()==Qt::Key_Escape){
            valChangeTimer.stop();
            this->view(); // отменяет последнее редактирование и отменяет посылку нового значения контроллеру
            numEdit.clearFocus();
            return true;
        }
        if(keyEvent->key()==Qt::Key_Return){
            if(valChangeTimer.isActive()){
                valChangeTimer.stop();
                onChangeValueTimer(); // завершает редактирование и посылает сигнал контроллеру сразу
                numEdit.setValue(numEdit.value()); // имитируем стандартные действия при нажатии enter
                numEdit.selectAll();
            }
            else{
                numEdit.selectAll();
            }
            return true; // блокируем станартные действия, т.к. они посылают сигнал valueChanged зачем-то
        }

        valChangeTimer.start(5000); // не все введенные символы вызывают сигнал valueChanged, пришлось прикрутить этот костыль.
        return false;
    }
    if(e->type() == QEvent::FocusOut){
        if(valChangeTimer.isActive()){
            valChangeTimer.stop();
            onChangeValueTimer(); // считаем что переход к другой ячейке - это сигнал о том что введенное значение верно.
        }
        return false; // родную обработку не надо блокировать
    }
    // как сделать чтобы при появлении фокуса число выделялось? как при редактировании таблиц...
    // (selectAll не таботает, т.к. обработка этого события продолжается в QlineEdit, а туда можно залезть только изнутри QDoubleSpinBox)

    return false;
}


  /***************************************/
 /*  Реализация custom led для LedView  */
/***************************************/


inline int lightestColorLine(QColor color){
    int lightest=1;
    if(color.red()>lightest)lightest=color.red();
    if(color.green()>lightest)lightest=color.green();
    if(color.blue()>lightest)lightest=color.blue();
    return lightest;
}

QMap<QString,QPixmap> CustomLed::pixmaps;

void CustomLed::paintEvent(QPaintEvent *)
{
    QPixmap pix;
    double w=width(),h=height();
    QPainter *painter = new QPainter(this);

    if(isOn()){
        if(pixmaps.contains(pixmapOn))
            pix=pixmaps.value(pixmapOn).scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }
    else{
        if(pixmaps.contains(pixmapOff))
            pix=pixmaps.value(pixmapOff).scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }

    if(!pix.isNull()){
        painter->drawPixmap(0,0,pix);
        painter->end();
        delete painter;
        return;
    }

    painter->setRenderHint(QPainter::Qt4CompatiblePainting,true);
    painter->setRenderHint(QPainter::Antialiasing,true);

// рисуем внешнюю окантовку

    double asp=(w+h)/48;
    //double asp=h/24;
    QLinearGradient gradient = QLinearGradient(qRound(w/2-3*h/w), qRound(h/2-3*w/h), qRound(w/2+3*h/w), qRound(h/2+3*w/h));
    gradient.setColorAt(0, light);
    gradient.setColorAt(1, shade);
    painter->setBrush(gradient);
    painter->setPen(QPen(darkborder));
    painter->drawEllipse(QRectF(1,1,width()-2,height()-2));

// рисуем внутреннюю часть окантовки

    gradient.setColorAt(0, shade);
    gradient.setColorAt(1, light);
    painter->setBrush(gradient);
    painter->setPen(QPen(Qt::NoPen));
    painter->drawEllipse(QRectF(2*asp,2*asp,width()-4*asp,height()-4*asp));

// рисуем светилку

    QRadialGradient gradient2 = QRadialGradient(w/2.7,h/2.7,(h+w)/6);
    int colorLight;
    if(on){
        gradient2.setColorAt(0, cLightOn);
        gradient2.setColorAt(1, cLedOn);
        colorLight=lightestColorLine(cLedOn);
    }
    else{
        gradient2.setColorAt(0, cLightOff);
        gradient2.setColorAt(1, cLedOff);
        colorLight=lightestColorLine(cLedOff);
    }

    if(colorLight<150) painter->setPen(QPen(border));
    else painter->setPen(QPen(gray));//setPen(QPen(Qt::NoPen));
    painter->setBrush(gradient2);
    painter->drawEllipse(QRectF(3*asp,3*asp,width()-6*asp,height()-6*asp));

    painter->end();
    delete painter;
}

void CustomLed::setStateOn(bool stateOn)
{
    on = stateOn;
    this->repaint();
}

void CustomLed::setColor(QColor color, bool inversed)
{
    double k=230/(double)lightestColorLine(color);
    QColor c1=QColor(color.red()*k,color.green()*k,color.blue()*k);
    QColor c2=gray;//QColor("#777");
    //QColor(c1.red()/100+80,c1.green()/100+80,c1.blue()/100+80); // вариант с темно-цветными выключенными лампочками субъективно воспринимается как более сложный для восприятия.
    if(inversed){
        cLedOff=c1;
        cLedOn=c2;
    }
    else{
        cLedOn=c1;
        cLedOff=c2;
    }
    // выключенные лампочки не должны бликовать, чтобы не привлекать внимание.
    k=lightestColorLine(cLedOn);
    if(k>150) cLightOn=QColor(cLedOn.red()/3+170,cLedOn.green()/3+170,cLedOn.blue()/3+170);
    else cLightOn=cLedOn;
    k=lightestColorLine(cLedOff);
    if(k>150) cLightOff=QColor(cLedOff.red()/3+170,cLedOff.green()/3+170,cLedOff.blue()/3+170);
    else cLightOff=cLedOff;

    // cLightOn=QColor((cLedOn.red()+k*2)/3,(cLedOn.green()+k*2)/3,(cLedOn.blue()+k*2)/3); // другой вариант рассчёта блика. При таком рассчёте блик от серого не будет виден.
}

void CustomLed::setExactColors(QColor color_on, QColor color_off)
{
    cLedOn=color_on;
    cLedOff=color_off;

    int k=lightestColorLine(cLedOn);
    if(k>150) cLightOn=QColor(cLedOn.red()/3+170,cLedOn.green()/3+170,cLedOn.blue()/3+170);
    else cLightOn=cLedOn;
    k=lightestColorLine(cLedOff);
    if(k>150) cLightOff=QColor(cLedOff.red()/3+170,cLedOff.green()/3+170,cLedOff.blue()/3+170);
    else cLightOff=cLedOff;
}

void CustomLed::setPixmaps(QString pixmap_on, QString pixmap_off)
{
    QFile file(pixmap_on);
    if(file.exists()){
        pixmapOn=file.fileName();
        pixmaps.insert(pixmapOn, QPixmap(pixmapOn));
    }
    file.setFileName(pixmap_off);
    if(file.exists()){
        pixmapOff=file.fileName();
        pixmaps.insert(pixmapOff, QPixmap(pixmapOff));
    }
}

bool CustomLed::isOn()
{
    return on;
}


  /*************************/
 /*  Реализация  LedView  */
/*************************/

LedView::LedView(const QString &cap, const PlcRegister &plcreg, bool vertical,
                 QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    // точно больше ничего не надо?
    placeWidgetOnLayout(&ledView);
    setFixedHeight(getDefaultHeight());
}

void LedView::adjustChildrenSize()
{
    ViewElement::adjustSize();

    int h = elementHeight;
    h=h-getDefaultSpacing();
    // я не думаю что стоит менять пропорции лампочки. Фиксированная ширина в данном случае будет игнорится
    int w=h;
    ledView.setFixedSize(QSize(h,w));
}

void LedView::setColor(QColor color)
{
    ledView.setColor(color, false);
}

void LedView::setColor(QColor color, bool inversed)
{
    ledView.setColor(color,inversed);
}

void LedView::setExactColors(QColor color_on, QColor color_off)
{
    ledView.setExactColors(color_on,color_off);
}

void LedView::setImages(QString imageName_on, QString imageName_off)
{
    ledView.setPixmaps(imageName_on,imageName_off);
}

void LedView::view()
{
    ledView.setStateOn(value);
}


/***************************************************/
/*  Реализация custom button для класса LedButton  */
/***************************************************/

void CustomPushButton::setTextOn(const QString texton)
{
    text_on=texton;
}

void CustomPushButton::setTextOff(const QString textoff)
{
    text_off=textoff;
}

void CustomPushButton::paintEvent(QPaintEvent *)
{
    QColor but(this->palette().color(QPalette::ColorRole::Button));

    double k=230/(double)lightestColorLine(but);
    QColor cButOn=QColor(but.red()*k,but.green()*k,but.blue()*k);
    QColor cLightOn=QColor(cButOn.red()/2+127,cButOn.green()/2+127,cButOn.blue()/2+127);
    QColor cShadeOn=QColor(cButOn.red()/1.7,cButOn.green()/1.7,cButOn.blue()/1.7);

    QColor cButOff=QColor(cButOn.red()/3+40,cButOn.green()/3+40,cButOn.blue()/3+40);
    QColor cLightOff=QColor(cButOff.red()/3+127,cButOff.green()/3+127,cButOff.blue()/3+127);
    QColor cShadeOff=QColor(cButOff.red()/2,cButOff.green()/2,cButOff.blue()/2);

    QPainter *painter = new QPainter(this);
    painter->setRenderHint(QPainter::Qt4CompatiblePainting,true);
    //painter->setRenderHint(QPainter::Antialiasing,true);

// рисуем внешнюю окантовку кнопки
    double w=width(),h=height();
    QLinearGradient gradient = QLinearGradient(qRound(w/2-3*h/w), qRound(h/2-3*w/h), qRound(w/2+3*h/w), qRound(h/2+3*w/h));
    gradient.setColorAt(0, QColor("#EED"));
    gradient.setColorAt(1, QColor("#665"));
    painter->setBrush(gradient);
    painter->setPen(QPen(QColor("#333")));
    painter->drawRoundedRect(QRect(0,0,width()-1,height()-1),5,5);

// рисуем внутреннюю часть окантовки

    gradient.setColorAt(0, QColor("#665"));
    gradient.setColorAt(1, QColor("#EED"));
    painter->setBrush(gradient);
    painter->setPen(QPen(Qt::NoPen));
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->drawRoundedRect(QRect(2,2,width()-4,height()-4),5,5);

// рисуем наклоны кнопки

    if(this->isChecked()){
    //if(isDown()){
        gradient.setColorAt(0, cShadeOn);//
        gradient.setColorAt(1, cLightOn);
    }
    else{
        gradient.setColorAt(0, cLightOff);
        gradient.setColorAt(1, cShadeOff);
    }
    painter->setPen(QPen(QColor("#333")));
    painter->setBrush(gradient);
    painter->setRenderHint(QPainter::Antialiasing,false);
    painter->drawRoundedRect(3,3,width()-7,height()-7,4,4);

// рисуем плоскость кнопки

    QBrush brush = QBrush(Qt::SolidPattern);
    //brush.setStyle(Qt::SolidPattern);
    if(this->isChecked()){
    //if(isDown()){
        painter->setPen(QPen(cShadeOn));
        brush.setColor(cButOn);
        painter->setBrush(brush);
        painter->drawRoundedRect(5,5,width()-11,height()-11,3,3);
    }
    else{
        painter->setPen(QPen(cShadeOff));
        brush.setColor(cButOff);
        painter->setBrush(brush);
        painter->drawRoundedRect(5,5,width()-11,height()-12,3,3);
    }

// рисуем текст
    // я не уверен, что эти кнопки будут реализованы как потомок QPushButton, т.к. для нас это избыточно
    // плюс надо будет реализовать режим для однократного срабатывания... хз как его синхронизировать с контроллером...

    // нажатые кнопки имеют подпись смещенную чуть вниз. Чем больше кнопка, тем больше смещение.
    painter->setFont(ViewElement::getDefaultFont());
    if(this->isChecked()){
        painter->setPen(Qt::white);
        painter->drawText(0,h/24,w,h,Qt::AlignCenter,text_on);
        //painter->drawText((w-painter->fontMetrics().width(text_on))/2+1, (h+painter->font().pointSizeF())/2+1, text_on);
    }
    else if(this->isDown()){
        painter->setPen(Qt::lightGray);
        painter->drawText(0,h/24,w,h,Qt::AlignCenter,text_off);
        //painter->drawText((w-painter->fontMetrics().width(text_off))/2+1, (h+painter->font().pointSizeF())/2+1, text_off);
    }
    else{
        painter->setPen(Qt::lightGray);
        painter->drawText(0,0,w,h,Qt::AlignCenter,text_off);
        //painter->drawText((w-painter->fontMetrics().width(text_off))/2, (h+painter->font().pointSizeF())/2, text_off);
    }

    painter->end();
    delete painter;
}

  /***************************/
 /*  Реализация  LedButton  */
/***************************/

LedButton::LedButton(const QString &cap, const PlcRegister &plcreg, bool vertical,
                     QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    button.setText("");
    button.setCheckable(true);
    setColor(QColor("#4AF"));
    button.setFocusPolicy(Qt::NoFocus);

    // ЧОТКА РАЗБЕРИСЬ КАКОЙ СИГНАЛ ЗАВОДИТЬ! ОНИ РАБОТАЮТ ПОРАЗНОМУ.
    QObject::connect(&button, SIGNAL(toggled(bool)), this, SLOT(changeValue()));

    placeWidgetOnLayout(&button);
    setFixedHeight(getDefaultHeight());
}

void LedButton::adjustChildrenSize()
{
    int h = elementHeight;
    h=h-getDefaultSpacing()*0.5;
    int w=h*2;
    if(elementWidth) w=elementWidth;

    button.setFixedHeight(h);
    button.setFixedWidth(w);
}

void LedButton::setColor(QColor color) //, bool inversed) - инверсия не реализована
{
    QPalette palet = button.palette();
    palet.setColor(QPalette::ColorRole::Button,color);
    button.setPalette(palet);
}

void LedButton::setTextOn(const QString texton)
{
    button.setTextOn(texton);
}

void LedButton::setTextOff(const QString textoff)
{
    button.setTextOff(textoff);
}

void LedButton::setConfirmOn(const QString text)
{
    confirmOn=text;
}

void LedButton::setConfirmOff(const QString text)
{
    confirmOff=text;
}

void LedButton::view()
{
    if(no_view)return; // пока висит окно с вопросом, обновление с контроллера не должно поменять состояние кнопки.
    button.blockSignals(true);
    button.setChecked(value);
    button.blockSignals(false);
}

void LedButton::changeValue()
{
    no_view=true; // пока висит окно с вопросом, обновление с контроллера не должно поменять состояние кнопки.
    if(button.isChecked() && !confirmOn.isEmpty()){
        QMessageBox::StandardButton ask;
        ask = QMessageBox::question(this,Resources::translate("confirm action"),confirmOn, QMessageBox::Yes|QMessageBox::No);
        if (ask == QMessageBox::Yes) {}
        else {
            no_view=false;
            this->view();
            return;
        }
    }
    if(!button.isChecked() && !confirmOff.isEmpty()){
        QMessageBox::StandardButton ask;
        ask = QMessageBox::question(this,Resources::translate("confirm action"),confirmOff, QMessageBox::Yes|QMessageBox::No);
        if (ask == QMessageBox::Yes) {}
        else {
            no_view=false;
            this->view();
            return;
        }
    }

    value = button.isChecked();
    no_view=false;
    emit valueChanged(getPLCregister(), value);
}


/***************************/
/*  Реализация  NumButton  */
/***************************/


NumButton::NumButton(const QString &cap, const PlcRegister &plcreg, int value,
                     bool vertical, QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    theValue=value;
    button.setText("");
    button.setCheckable(true);
    setColor(QColor("#4AF"));
    button.setFocusPolicy(Qt::NoFocus);

    QObject::connect(&button, SIGNAL(toggled(bool)), this, SLOT(changeValue()));

    placeWidgetOnLayout(&button);
    setFixedHeight(getDefaultHeight());
}

void NumButton::adjustChildrenSize()
{
    int h = elementHeight;
    h=h-getDefaultSpacing()*0.5;
    int w=h*2;
    if(elementWidth) w=elementWidth;

    button.setFixedHeight(h);
    button.setFixedWidth(w);
}

void NumButton::setColor(QColor color) //, bool inversed) - инверсия не реализована
{
    QPalette palet = button.palette();
    palet.setColor(QPalette::ColorRole::Button,color);
    button.setPalette(palet);
}

void NumButton::setTextOn(const QString texton)
{
    button.setTextOn(texton);
}

void NumButton::setTextOff(const QString textoff)
{
    button.setTextOff(textoff);
}

void NumButton::setConfirmOn(const QString text)
{
    confirmOn=text;
}

void NumButton::setConfirmOff(const QString text)
{
    confirmOff=text;
}

void NumButton::view()
{
    if(no_view)return; // пока висит окно с вопросом, обновление с контроллера не должно поменять состояние кнопки.
    button.blockSignals(true);
    button.setChecked(value==theValue);
    button.blockSignals(false);
}

void NumButton::changeValue()
{
    no_view=true; // пока висит окно с вопросом, обновление с контроллера не должно поменять состояние кнопки.
    if(button.isChecked() && !confirmOn.isEmpty()){
        QMessageBox::StandardButton ask;
        ask = QMessageBox::question(this,Resources::translate("confirm action"),confirmOn, QMessageBox::Yes|QMessageBox::No);
        if (ask == QMessageBox::Yes) {}
        else {
            no_view=false;
            this->view();
            return;
        }
    }
    if(!button.isChecked() && !confirmOff.isEmpty()){
        QMessageBox::StandardButton ask;
        ask = QMessageBox::question(this,Resources::translate("confirm action"),confirmOff, QMessageBox::Yes|QMessageBox::No);
        if (ask == QMessageBox::Yes) {}
        else {
            no_view=false;
            this->view();
            return;
        }
    }

    if(button.isChecked()) value = theValue;
    no_view=false;
    emit valueChanged(getPLCregister(), value);
}



  /*******************************/
 /*   Реализация  StatusView    */
/*******************************/


StatusView::StatusView(const QString &cap, const PlcRegister &plcreg, const QList<int> &regValues,
                       const QList<QString> &valueLabels, const QList<QColor> labelColors,
                       bool vertical, QWidget *parent): ViewElement(cap,plcreg,vertical,parent)
{
    values=regValues;
    labels=valueLabels;
    colors=labelColors;

    QPalette pal(statusView.palette());
    pal.setColor(QPalette::WindowText, color);
    pal.setColor(QPalette::Window, pal.color(QPalette::Base));

    statusView.setAlignment(Qt::AlignCenter);
    statusView.setPalette(pal);
    statusView.setAutoFillBackground(true);
    statusView.setFrameShape(QFrame::Panel);
    statusView.setFrameShadow(QFrame::Sunken);
    statusView.setLineWidth(1);
    /* в последствии необходимо настроить отступы и прочие тонкости размещения */

    placeWidgetOnLayout(&statusView);

    setFixedHeight(getDefaultHeight());

    this->setValue(value); // отобразить хоть что-то.
}

void StatusView::adjustChildrenSize()
{
    QFont fnt=this->font();
    fnt.setPointSize(fnt.pointSize()+1);
    statusView.setFont(fnt);

    int h = elementHeight;
    h=h-getDefaultSpacing()*1.2;
    int w = h*4;
    if(elementWidth) w=elementWidth;

    statusView.setFixedHeight(h);
    statusView.setFixedWidth(w);
}

void StatusView::view()
{
    int n=values.indexOf(value); // если не найдено, то возвращает значение -1

    QString str; // по умолчанию строка пустая. можно было бы в последствии сделать дефолтное значение
    if((n>=0)&&(n<labels.count())) str=labels.at(n);
    statusView.setText(str);

    QColor col=Qt::black; // по умолчанию чёрный цвет. можно было бы в последствии сделать дефолтное значение
    QPalette pal(statusView.palette());
    if ((n>=0)&&(n<colors.count())) col=colors.at(n);
    pal.setColor(QPalette::WindowText, col);
    statusView.setPalette(pal);
}


  /*****************************/
 /*   Реализация  AlarmList   */
/*****************************/


AlarmList::AlarmList(const QString &cap, int rowsCount,
                     const QList<PlcRegister> &registers, const QList<QString> &regLabels,
                     const QList<QColor> &labelColors, bool vertical,
                     QWidget *parent): ViewElement(cap,PlcRegister("none"),vertical,parent)
{
    // сохраняем список регистров и соответсвующие текст и цвета.
    regs=registers;
    rowscount=rowsCount;
    labels=regLabels;
    colors=labelColors;
    //while(colors.count()<registers.count()) colors.append(Qt::black); // я хз вроде и без этого работает

    // создаём рамку со светлым фоном и слегка утопленную(, фиксированной ширины?).
    QVBoxLayout* frameLayout=new QVBoxLayout(&frame);
    frameLayout->setMargin(2);
    frameLayout->setSpacing(0);
    frame.setLayout(frameLayout);

    QPalette pal(frame.palette());
    pal.setColor(QPalette::Window, pal.color(QPalette::Base));
    frame.setPalette(pal);
    frame.setFrameStyle(QFrame::Panel);
    frame.setFrameShadow(QFrame::Sunken);
    frame.setAutoFillBackground(true);

    // добавляем элемент отображения на виджет и настраиваем размеры и т.п. по умолчанию
    layout()->addWidget(&frame);
    if(caption) caption->setWordWrap(true);
    canExpand=true;
    adjustCaption();
    adjustChildrenSize();
}

void AlarmList::adjustChildrenSize()
{
    // этот элемент растягиваемый только по горизонтали
    // высота определяется количеством строк.
    // Если указана фиксированная высота, то она переводится в кол-во целых строк

    Qt::Alignment screen_align=Qt::AlignVCenter;
    int rows=rowscount;
    int h=elementHeight;
    int w=elementWidth;
    int label_h = fontMetrics().lineSpacing()+1; // такое же вычисление делается в xmlCommonAttributes()

    if(h>0){ // высота указана, из нее считаем кол-во строк
        rows = h / label_h;
    }
    else { // иначе берем указанное/дефолтное кол-во рядов
        if(rows<=0) rows=5; // высота по умолчанию
    }
    if(w>0){ // нег горизонт.растяжки
        frame.setFixedWidth(w-getDefaultSpacing());
        if(vertic)screen_align|=Qt::AlignHCenter; else screen_align|=Qt::AlignRight;
    }
    else { // горизон. растяжка
        frame.setMinimumWidth(getDefaultHeight()*10);
        frame.setMaximumWidth(freeExpand); // такая величина у Qt по умолчанию.
    }
    layout()->setAlignment(&frame, screen_align);
    frame.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);//Expanding); //Preferred);

    int addlines=rows-alarmViews.count();
    // добавляем недостающие строки
    for(int i=0;i<addlines;i++){
        QLabel* labl=new QLabel("",&frame);
        labl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        alarmViews<<labl;
        frame.layout()->addWidget(labl);
    }
    // удаляем лишние
    for(int i=0;i>addlines;i--){
        delete alarmViews.last();
        alarmViews.removeLast();
    }
    // устанавливаем новое значение высоты строк
    foreach(QLabel *labl, alarmViews){
        labl->setFixedHeight(label_h);
    }
}

void AlarmList::paintAlarmRow(int row)
{
    // обновляем запись в строке с индексом row.
    // для этого сначала определяем что за регистр там должен отображаться.
    // потом находим соответствующую строку
    // и цвет.4к

    QString alarmText;
    QColor alarmColor=Qt::black;
    if(row<alarmRegs.count()){
        int al_num=regs.indexOf(alarmRegs.at(row));
        if(al_num>=0){
            if(al_num<labels.count())alarmText=labels.at(al_num);
            if(al_num<colors.count())alarmColor=colors.at(al_num);
        }
    }
    alarmViews.at(row)->setText(alarmText);
    QPalette pal=alarmViews.at(row)->palette();
    pal.setColor(QPalette::WindowText, alarmColor);
    alarmViews.at(row)->setPalette(pal);
}

void AlarmList::view()
{
    if(regs.indexOf(newValReg)>=0){ // проверяем подходит ли нам этот регистр (а то вдруг он ошибочный)

        int n=alarmRegs.indexOf(newValReg); // позиция регистра в списке ошибок

        if((n>=0)&&(value<=0)){ // если значение регистра сменилось на 0, но регистр находится в списке алармов, то
            alarmRegs.removeAt(n); // убираем регистр из списка
            if(n<alarmViews.count()) // если аларм отображался, то сдвигаем отображение
                for(int i=n;i<alarmViews.count();i++) // перебираем строки вьюшки
                    paintAlarmRow(i);
        }

        if((n<0)&&(value>0)){ // если же регитра нет в списке алармов, а значение больше 0, то
            int i=alarmRegs.count(); // определяем индекс новой записи
            alarmRegs.append(newValReg); // добавляем регистр в список
            if(i<alarmViews.count())
                paintAlarmRow(i); // отрисовываем новую запись в случае надобности.
        }
        // в остальных случаях ничего не меняем.
    }
}



/*****************************/
/*   Реализация  AlarmLog    */
/*****************************/


AlarmLog::AlarmLog(const QString &cap, const QString &name, const QList<PlcRegister> &registers,
                   const QList<QString> &regLabels, const QList<QString> &regTooltips, const QString dateformat,
                   bool vertical, QWidget *parent): ViewElement(cap,PlcRegister("none"),vertical,parent)
{
    regs = registers;

    alarmmodel.setActiveAlarmColor(QColor(Qt::darkRed));
    alarmmodel.setAlarmsText(regLabels);
    alarmmodel.setAlarmsDescription(regTooltips);
    if(!dateformat.isEmpty()) alarmmodel.setDateFormat(dateformat);

    alarmview.setModel(&alarmmodel);
    alarmview.setSelectionMode(QAbstractItemView::NoSelection);

    // добавляем элемент отображения на виджет и настраиваем размеры и т.п. по умолчанию
    layout()->addWidget(&alarmview);
    if(caption) caption->setWordWrap(true);
    canExpand=true;
    adjustCaption();
    adjustChildrenSize();

    QDir dir;
    dir.mkpath(Consts::workDirPath+"/"+name); // тут я не уверен, может надо сделать проверку наличия директории?
    alarmmodel.setLogFile(Consts::workDirPath+"/"+name+"/"+name+".txt");
}

void AlarmLog::view()
{
    if(regs.contains(newValReg)){
        int i = regs.indexOf(newValReg);
        if(value>0) alarmmodel.setAlarm(i);
        else alarmmodel.resetAlarm(i);
    }
}

void AlarmLog::adjustChildrenSize()
{
    // копипаста с DiagViewDirectPaint...

    int h=elementHeight;
    int w=elementWidth;
    Qt::Alignment screen_align;

    if(h>0){ // нет верт. растяжки
        alarmview.setFixedHeight(h-getDefaultSpacing()*2);
        screen_align|=Qt::AlignVCenter;
    }
    else { // верт.растяжка
        alarmview.setMinimumHeight(getDefaultHeight()*5);
        alarmview.setMaximumHeight(freeExpand); // такая величина у Qt по умолчанию.
    }
    if(w>0){ // нег горизонт.растяжки
        alarmview.setFixedWidth(w-getDefaultSpacing());
        if(vertic)screen_align|=Qt::AlignHCenter; else screen_align|=Qt::AlignRight;
    }
    else { // горизон. растяжка
        alarmview.setMinimumWidth(getDefaultHeight()*10);
        alarmview.setMaximumWidth(freeExpand); // такая величина у Qt по умолчанию.
    }
    layout()->setAlignment(&alarmview, screen_align);
    alarmview.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    alarmview.setFont(this->font());
    alarmview.setStyleSheet("QToolTip { color: #000; back ground-color: #ff0; border-width: 1px; border-style: solid; border-color: #000;}");
}


  /*************************/
 /*   Реализация  Table   */
/*************************/


TableDividedByColumns::TableDividedByColumns(const QString &cap, int rowcount, const QList<QString> &columnlabels,
                                             const QList<PlcRegister> &columnregisters, const QList<int> &columnregintervals,
                                             bool vertical, QWidget *parent): ViewElement(cap,PlcRegister("none"),vertical,parent)
{
    regs=columnregisters;
    regintervals=columnregintervals;
    QList<QString> labels = columnlabels;
    for(int c=0;c<regs.count();c++){
        if(labels.count()<=c)labels.append(regs.at(c).toString());
        if(regintervals.count()<=c) regintervals.append(1);
        else if(regintervals.at(c)<1)regintervals[c]=1;
    }
    table.setSelectionMode(QAbstractItemView::ContiguousSelection);
    table.setRowCount(rowcount);
    table.setColumnCount(regs.count());
    table.setWordWrap(0);
    table.setHorizontalHeaderLabels(labels);

    table.installEventFilter(this);

    // создаем контекстное меню и экшны в для него
    contextMenu = new QMenu(this);
    table.verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(table.verticalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));
    table.setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(&table, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));
    insertRowAction=new QAction(Resources::translate("insert empty lines"),this); // "вставить пустые строки"
    QObject::connect(insertRowAction,SIGNAL(triggered(bool)),this,SLOT(insertRowSlot()));
    deleteRowAction=new QAction(Resources::translate("delete lines"),this); // "удалить строки со сдвигом"
    QObject::connect(deleteRowAction,SIGNAL(triggered(bool)),this,SLOT(deleteRowSlot()));
    separatorAction=new QAction(this);
    separatorAction->setSeparator(true);
    copySelectedAction=new QAction(Resources::translate("copy selected"),this); // "копировать"
    QObject::connect(copySelectedAction,SIGNAL(triggered(bool)),this,SLOT(copySelectedSlot()));
    pasteSelectedAction=new QAction(Resources::translate("paste selected"),this); // "вставить"
    QObject::connect(pasteSelectedAction,SIGNAL(triggered(bool)),this,SLOT(pasteSelectedSlot()));
    cutSelectedAction=new QAction(Resources::translate("cut selected"),this); // "вырезать"
    QObject::connect(cutSelectedAction,SIGNAL(triggered(bool)),this,SLOT(cutSelectedSlot()));

    QObject::connect(&table, SIGNAL(cellChanged(int,int)), this, SLOT(changeValue(int,int)));

    // добавляем элемент отображения на виджет и настраиваем размеры и т.п. по умолчанию
    layout()->addWidget(&table);
    if(caption) caption->setWordWrap(true);
    canExpand=true;
    adjustCaption();
    adjustChildrenSize();
}

void TableDividedByColumns::adjustChildrenSize()
{
    // копипаста с DiagViewDirectPaint...

    int h=elementHeight;
    int w=elementWidth;
    Qt::Alignment screen_align;

    if(h>0){ // нет верт. растяжки
        table.setFixedHeight(h-getDefaultSpacing()*2);
        screen_align|=Qt::AlignVCenter;
    }
    else { // верт.растяжка
        table.setMinimumHeight(getDefaultHeight()*5);
        table.setMaximumHeight(freeExpand); // такая величина у Qt по умолчанию.
    }
    if(w>0){ // нег горизонт.растяжки
        table.setFixedWidth(w-getDefaultSpacing());
        if(vertic)screen_align|=Qt::AlignHCenter; else screen_align|=Qt::AlignRight;
    }
    else { // горизон. растяжка
        table.setMinimumWidth(getDefaultHeight()*10);
        table.setMaximumWidth(freeExpand); // такая величина у Qt по умолчанию.
    }
    layout()->setAlignment(&table, screen_align);
    table.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    table.setFont(this->font());
    table.resizeColumnsToContents();
    table.resizeRowsToContents();
}

void TableDividedByColumns::addTextList(QList<QStringList> *textlist)
{
    textLists=textlist;
}

void TableDividedByColumns::addTypeList(QList<QStringList> *typelist)
{
    typeLists=typelist;
}

void TableDividedByColumns::setColumnTypes(const QList<int> &columnMinValues, const QList<int> &columnMaxValues,const QList<QString> &columnTypes)
{
    for(int c=0;c<regs.count();c++)
    {
        int minval=0;
        int maxval=1000;
        QString coltype="int:0";
        if(columnMinValues.count()>c) minval=columnMinValues.at(c);
        if(columnMaxValues.count()>c) maxval=columnMaxValues.at(c);
        if(columnTypes.count()>c) coltype=columnTypes.at(c);
        QStringList coltypesplit=coltype.split(":");

        // устанавливаем вид и тип редактирования столбцов;
        // --> тут надо вставить выбор типа данных, точнее типа ячейки.
        QAbstractItemDelegate *delegat=nullptr;

        if(coltypesplit.at(0)==Consts::intType){
            int decimals=0;
            if(coltypesplit.count()>1)decimals=coltypesplit.at(1).toInt();
            delegat = new DoubleSpinboxDelegate(minval/qPow(10, decimals),maxval/qPow(10, decimals), decimals,this);
        }
        else if(coltypesplit.at(0)==Consts::binType){
            delegat = new SpinboxDelegate(2,minval,maxval,false,this);
        }
        else if(coltypesplit.at(0)==Consts::octType){
            delegat = new SpinboxDelegate(8,minval,maxval,true,this);
        }
        else if(coltypesplit.at(0)==Consts::hexType){
            delegat = new SpinboxDelegate(16,minval,maxval,true,this);
        }
        else if(coltypesplit.at(0)==Consts::timeType){
            delegat = new TimeEditDelegate(minval,maxval,this);
        }
        else if(coltypesplit.at(0)==Consts::textListType){
            int listnum=0;
            QStringList list;
            if(coltypesplit.count()>1)listnum=coltypesplit.at(1).toInt();
            if(textLists)
            if(textLists->count()>listnum)list<<textLists->at(listnum);
            delegat = new ComboBoxDelegate(list,this);
        }
        else if(coltypesplit.at(0)==Consts::flagListType){
            int listnum=0;
            QStringList list;
            if(coltypesplit.count()>1)listnum=coltypesplit.at(1).toInt();
            if(textLists)
            if(textLists->count()>listnum)list<<textLists->at(listnum);
            delegat = new FlagBoxDelegate(list,this);
        }
        else if(coltypesplit.at(0)==Consts::floatType){
            // пока не реализовал делегата.
        }
        else if(coltypesplit.at(0)==Consts::stringType){
            // пока не реализовал делегата.
        }
        else if(coltypesplit.at(0)==Consts::typeListType){
            int listnum=0;
            int definingColumn=0;
            QStringList typelist;
            if(coltypesplit.count()>1)listnum=coltypesplit.at(1).toInt();
            if(coltypesplit.count()>2)definingColumn=coltypesplit.at(2).toInt();
            if(typeLists)
            if(typeLists->count()>listnum)typelist<<typeLists->at(listnum);
            delegat = new VaribleDelegate(definingColumn,typelist,textLists,this);
        }

        if(delegat) table.setItemDelegateForColumn(c, delegat);


        for(int r=0;r<table.rowCount();r++)
        {
            QTableWidgetItem *it = new QTableWidgetItem("0");
            it->setTextAlignment(Qt::AlignCenter);
            table.setItem(r, c, it);
        }
    }
}

void TableDividedByColumns::changeValue(int row, int column)
{
    // слот приватный. Значения column и row контролируются на стороне вызова...
    // надо ли дополнительно проверять? пока не вижу где может возникнуть проблема.

    // вычисляем регистр, соответствующий изменённой ячейке, посылаем сигнал изменения регистра с новым значением.
    PlcRegister reg = regs.at(column)+row*regintervals.at(column);

    value = table.model()->index(row,column).data().toInt();
    //qDebug()<<reg.toString()<<" "<<QString::number(value);

    emit valueChanged(reg, value);
}

void TableDividedByColumns::showContextMenu(const QPoint &p)
{
    //qDebug()<<"show context menu";
    QList<QAction *> actList;
    if(table.selectionModel()->selectedRows().count()){
        //qDebug()<<"has row selected";
        actList.append(insertRowAction);
        actList.append(deleteRowAction);
        actList.append(separatorAction);
    }
    if(table.selectionModel()->selection().count()){
        //qDebug()<<"has selection";
        actList.append(copySelectedAction);
        actList.append(pasteSelectedAction);
        actList.append(cutSelectedAction);
        contextMenu->exec(actList,table.mapToGlobal(p));
    }
}

void TableDividedByColumns::insertRowSlot()
{
    // тут надо осуществить сдвиг всех строк начиная с предпоследней вниз
    // и заполнение нулями выделенной(верхней если несколько) строки.
    // !!!
    // т.к. выделять строки можно не подряд, это несколько усложняет процесс:
    // 1. сдвигаем строки с последней минус кол-во строк выделения до последней выделенной
    // 2. далее заполняем строку нулями
    // 3. далее повторяем первые два пункта с оставшимися строками.

    int startRow=table.selectionModel()->selectedRows().at(0).row();
    int rowSelectCount=table.selectionModel()->selectedRows().count();
    for(int r=table.rowCount()-1-rowSelectCount;r>=startRow;r--){
        for(int c=0;c<table.columnCount();c++){
            QVariant val=table.model()->data(table.model()->index(r,c));
            table.model()->setData(table.model()->index(r+rowSelectCount,c), val);
        }
    }
    for(int r=startRow;r<startRow+rowSelectCount;r++){
        for(int c=0;c<table.columnCount();c++){
            table.model()->setData(table.model()->index(r,c), 0);
        }
    }

}

void TableDividedByColumns::deleteRowSlot()
{
    // тут надо осуществить сдвиг всех строк начиная со следующей после выделенной вверх
    // и заполнение нулями нижней строки
    // !!!
    // такая же беда как и со строками, только еще хуже, т.к. выделять можно хоть шашечками
    // как вариант, найти настройку где убрать выделение не прямоугольной области

    int startRow=table.selectionModel()->selectedRows().at(0).row();
    int rowSelectCount=table.selectionModel()->selectedRows().count();
    for(int r=startRow+rowSelectCount;r<table.rowCount();r++){
        for(int c=0;c<table.columnCount();c++){
            QVariant val=table.model()->data(table.model()->index(r,c));
            table.model()->setData(table.model()->index(r-rowSelectCount,c), val);
        }
    }
    for(int r=table.rowCount()-rowSelectCount;r<table.rowCount();r++){
        for(int c=0;c<table.columnCount();c++){
            table.model()->setData(table.model()->index(r,c), 0);
        }
    }
    table.clearSelection();
}

void TableDividedByColumns::copySelectedSlot()
{
    QString copytext;
    int row=-1; // на всякий случай, хотя оно тут и не надо, т.к. вставка разделителей для 1го элемента отсекается условием if(copytext.count())
    foreach(QTableWidgetItem *item, table.selectedItems()){
        if(copytext.count()){
            if(item->row() == row) copytext.append("\t");
            else copytext.append(13); // ??? достаточно ли этого разделителя для виндовс?
        }
        row = item->row();
        copytext.append(item->text());
    }
    if(copytext.count()) QApplication::clipboard()->setText(copytext);
}

void TableDividedByColumns::pasteSelectedSlot()
{
    int startColumn=0;
    int startRow=0;
    if(table.selectedItems().count()){
        startColumn=table.selectedItems().first()->column();
        startRow=table.selectedItems().first()->row();
    }
    QStringList rowList = QApplication::clipboard()->text().split(13);
    for(int r=0;r<rowList.count();r++){
        QStringList colList = rowList.at(r).split("\t");
        for(int c=0;c<colList.count();c++){
            bool ok;
            int val=colList.at(c).toInt(&ok);
            if(ok){
                table.model()->setData(table.model()->index(startRow+r,startColumn+c),val);
            }
        }
    }
}

void TableDividedByColumns::cutSelectedSlot()
{
    copySelectedSlot();
    foreach(QTableWidgetItem *item, table.selectedItems())
        item->setData(Qt::EditRole, 0);
}

bool TableDividedByColumns::eventFilter(QObject *target, QEvent *event)
{
    if((target==&table)&&(event->type()==QEvent::KeyPress)){
        QKeyEvent *keyevent = static_cast<QKeyEvent*>(event);

        if ((keyevent->key() == Qt::Key_C) && (keyevent->modifiers() & Qt::ControlModifier)){
            copySelectedSlot();
            return true;
        }
        else
        if ((keyevent->key() == Qt::Key_X) && (keyevent->modifiers() & Qt::ControlModifier)){
            cutSelectedSlot();
            return true;
        }
        else
        if(keyevent->key() == Qt::Key_V && (keyevent->modifiers() & Qt::ControlModifier)){
            pasteSelectedSlot();
            return true;
        }
        else
        if(keyevent->key() == Qt::Key_W && (keyevent->modifiers() & Qt::ControlModifier)){
            // как бы тут сделать так чтобы переключалась подгонка размера ячеек
            // по размеру содержимого / по размеру редакторов?
            // пока остановимся на FlagBoxList, остальное вообще не принципиально.
            // где-то надо сделать глобальную переменную... наверное в resources.
            // да, это костыль. да, не красиво. Эта фича исключительно для моего родного отца.
            expandFlagBoxList=!expandFlagBoxList;
            Resources::EXPAND_FLAGBOXLIST=expandFlagBoxList;
            table.resizeColumnsToContents();
            table.resizeRowsToContents();
            Resources::EXPAND_FLAGBOXLIST=false;
            return true;
        }
    }
    return false;
}

void TableDividedByColumns::view()
{
    table.blockSignals(true);

    // вычисляем ячейку соответствующую регистру newValReg, вносим значение.
    for(int col=0;col<table.columnCount();col++){
        if(regs.at(col).getType()==newValReg.getType()){
            int row = (newValReg.getIndex() - regs.at(col).getIndex())/regintervals.at(col);
            if((row>=0)&&(row<table.rowCount())){
                int regindex=regs.at(col).getIndex()+row*regintervals.at(col);
                if(newValReg.getIndex()==regindex) // на случай если регистр попадает между строк.
                {
                    //table.item(row,col)->setText(QString::number(value/qPow(10,decimals.at(col))));
                    table.model()->setData(table.model()->index(row,col),value);
                }
            }
        }
    }

    table.blockSignals(false);
}



/****************************************/
/*   Реализация  DiagViewDirectPaint    */
/****************************************/

/*   Реализация  Screen  */

DiagViewDirectPaint::Screen::Screen(DiagViewDirectPaint *parent):QWidget(parent)
{
    this->parent = parent;
    this->setMouseTracking(true);
}

void DiagViewDirectPaint::Screen::paintEvent(QPaintEvent *)
{
    QPainter *painter = new QPainter(this);

    QBrush brush = painter->brush();
    brush.setColor(parent->background);//Qt::white
    brush.setStyle(Qt::SolidPattern);
    painter->setBrush(brush);
    painter->setRenderHint(QPainter::Qt4CompatiblePainting,true);
    painter->drawRect(0,0,width()-1,height()-1);
    for(int i=0;i<parent->linesCount;i++){
        QPen pen=painter->pen();
        if(i<parent->colors.count())pen.setColor(parent->colors.at(i));
        else pen.setColor(Qt::black);
        painter->setPen(pen);
        painter->drawPolyline(parent->lines.at(i));
    }

    // рисуем курсор и табличку со значениями на которые он указывает
    // рассчитываем ширину текста подписей и цифр, смещения, межстрочных интервалов
    // исходя из размеров текста. Пока выбираем размер текста 8.

    int charSize=8;
    int totalWidth=(parent->labelsMaxCharCount+parent->valuesMaxCharCount+2)*charSize;
    int valuesShift=(parent->labelsMaxCharCount+2)*charSize;
    int totalShift=0;
    if(parent->cursorPos<this->width()/2)totalShift=this->width()-10-totalWidth;

    if(parent->cursorPos>0){
        painter->setPen(QPen(QColor(150,150,150,150)));
        painter->drawLine(parent->cursorPos,1,parent->cursorPos,height()-2);

        // нарисовать квадратик полупрозрачный
        painter->setPen(QPen(Qt::transparent));
        painter->setBrush(QBrush(QColor(230,230,230,230)));
        painter->drawRect(5+totalShift,5,totalWidth,this->height()-10);

        // нарисовать подписи
        QFont font=QFont("fixed",charSize);
        painter->setFont(font);
        for(int i=0;i<parent->linesCount;i++){
            painter->setPen(QPen(parent->colors.at(i)));
            int y=5+(i+1)*(charSize*1.2+2);
            painter->drawText(7+totalShift,y,parent->labels.at(i));
            QString str=QString::number((double)parent->cursorVal.at(i)/parent->dividers.at(i),'f',parent->decimals.at(i));
            painter->drawText(valuesShift+totalShift,y,str);
        }
        painter->setPen(QPen(Qt::black));
        painter->drawText(10+totalShift,12+(parent->linesCount+1)*(charSize*1.2+2),parent->cursorTime.toString("HH:mm:ss"));
    }
    painter->end();
    delete painter;
}

/*  Screen  */

DiagViewDirectPaint::DiagViewDirectPaint(const QString &cap, int interval, const QList<PlcRegister> &lineRegisters,
                   const QList<int> &lineDecimals, const QList<int> &lineMaxs, const QList<QString> &lineLabels,
                   const QList<QColor> lineColors, const QColor backgroundColor, bool vertical, QWidget *parent):
    ViewElement(cap,PlcRegister("none"),vertical,parent)
{
    linesCount=lineRegisters.count();
    registers=lineRegisters;

    decimals=lineDecimals;
    while(decimals.count()<linesCount)
        decimals.append(0);

    for(int i=0;i<decimals.count();i++)
        dividers<<qPow(10,decimals.at(i));

    maxs=lineMaxs;
    while(maxs.count()<linesCount)
        maxs.append(1000);

    labels=lineLabels;
    while(labels.count()<linesCount)
        labels.append("");

    colors=lineColors;
    while(colors.count()<linesCount)
        colors.append(Qt::gray);

    background=backgroundColor;

    values.insert(0,linesCount,0); // сразу заполняем массив текущих значений по количеству линий
    cursorVal.insert(0,linesCount,0);
    lines.insert(0,linesCount,QPolygon());

    for(int i=0;i<linesCount;i++){
        if(labelsMaxCharCount<labels.at(i).count())labelsMaxCharCount=labels.at(i).count();
        int n=log10(maxs.at(i))+decimals.at(i);
        if(valuesMaxCharCount<n)valuesMaxCharCount=n;
    }

    screen = new Screen(this);
    screen->installEventFilter(this);
    buffer.insert(0,100*linesCount,0);

    // добавляем элемент отображения на виджет и настраиваем размеры и т.п. по умолчанию
    layout()->addWidget(screen);
    if(caption) caption->setWordWrap(true);
    canExpand=true;
    // делать фиксированные размеры по умолчанию для таких объектов?
    adjustCaption();
    adjustChildrenSize();

    // запускаем таймер, заполняющий диаграмму
    timer=new QTimer(this);
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    timer->start(interval);
}


void DiagViewDirectPaint::adjustChildrenSize()
{

    int h=elementHeight;
    int w=elementWidth;
    Qt::Alignment screen_align;

    if(h>0){ // нет верт. растяжки
        screen->setFixedHeight(h-getDefaultSpacing()*2);
        screen_align|=Qt::AlignVCenter;
    }
    else { // верт.растяжка
        screen->setMinimumHeight(getDefaultHeight()*5);
        screen->setMaximumHeight(freeExpand); // такая величина у Qt по умолчанию.
    }
    if(w>0){ // нег горизонт.растяжки
        screen->setFixedWidth(w-getDefaultSpacing());
        if(vertic)screen_align|=Qt::AlignHCenter; else screen_align|=Qt::AlignRight;
    }
    else { // горизон. растяжка
        screen->setMinimumWidth(getDefaultHeight()*10);
        screen->setMaximumWidth(freeExpand); // такая величина у Qt по умолчанию.
    }
    layout()->setAlignment(screen, screen_align);
    screen->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void DiagViewDirectPaint::onTimer()
{
    // по таймеру: - перезаполняем полилайны
    //             - вызываем отрисовку

    if(linesCount<=0)return;
    // добавляем текущие значения в буфер и смещаем position

    for(int i=0;i<linesCount;i++){
        buffer.replace(position,values.at(i));
        if(++position>=buffer.count())position=0;
    }

    lastPointTime=QTime::currentTime();

    // и обновляем диаграмму.
    if(screen->isVisible()){
        updateDiag();
        updateCursor();
        screen->repaint();
    }
}

void DiagViewDirectPaint::view()
{
    // определяем позицию регистра в листе и записываем значение регистра в соответствующую ячейку массива
    int n=registers.indexOf(newValReg);
    if(n>=0) values.replace(n, value);
}

void DiagViewDirectPaint::bufferResize(int num)
{
    if(linesCount<=0)return; // защита от нулевой длины буфера
    if(num<100)num=100; // на всякий случай защита от слишком мелких размеров

    int newCount=num*linesCount;
    if(newCount>buffer.count()){
        //вставляем пустые значения в позицию position
        buffer.insert(position,newCount-buffer.count(),0);
    }
    else if(newCount<buffer.count()){
        //удаляем сколько есть строчек с конца, потом сколько осталось сначала.
        if(position<newCount){
            buffer.remove(position,buffer.count()-newCount);
        }

        else {
            buffer.remove(position,buffer.count()-position);
            buffer.remove(0,position-newCount);
            position=0;
        }
    }
}

bool DiagViewDirectPaint::eventFilter(QObject *target, QEvent *event)
{
    if((target==screen)&&(event->type()==QEvent::MouseMove)){
        QMouseEvent *moveEvent = static_cast<QMouseEvent *>(event);
        cursorPos=moveEvent->pos().x(); // выход курсора за пределы сцены будет обрабатываться в updateCursor()
        updateCursor();
        screen->repaint();
        return true;
    }
    else if((target==screen)&&(event->type()==QEvent::Resize)){
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w=resizeEvent->size().width()-2;
        bufferResize(w);
        updateDiag();
        updateCursor(); // может потребоваться сместить textGroup
        return true;
    }
    else if((target==screen)&&(event->type()==QEvent::Leave)){
        cursorPos=-1;
        updateCursor();
        screen->repaint();
        return true;
    }

    return false;
}

void DiagViewDirectPaint::updateDiag()
{
    int pos=position;
    int h0=screen->height()-2;
    for(int i=0;i<linesCount;i++){
        lines[i].clear();
    }
    for(int x=buffer.count()/linesCount-1;x>0;x--){
        for(int i=0;i<linesCount;i++){
            if(--pos<0)pos=buffer.count()-1;
            int y=h0-(float)buffer.at(pos)/maxs.at(i)/dividers.at(i)*h0;
            lines[i].append(QPoint((float)x+1,y+1));
        }
    }
}

void DiagViewDirectPaint::updateCursor()
{
    if(cursorPos<1)cursorPos=-1;
    else{
        if(cursorPos>screen->width()-2)cursorPos=screen->width()-2;
        int pos=(cursorPos-1)*linesCount+position;
        if(pos>=buffer.count())pos-=buffer.count();
        if(pos<0)pos+=buffer.count(); // хз
        for(int i=0;i<linesCount;i++){
            cursorVal[i]=buffer.at(pos++);
        }
        int msec=(cursorPos-screen->width()-2)*timer->interval();
        cursorTime=lastPointTime.addMSecs(msec);
    }
}
