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

#ifndef VIEWELEMENT_H
#define VIEWELEMENT_H

#include <QtCore>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QGraphicsView>
#include <QListView>
#include <QApplication>
#include "logmodel.h"
#include "plcregister.h"

const bool DEBUG_TEST = false;

/*  абстрактный класс, родитель нескольких классов для отображения различных регистров.
    1. выдумать порядок создания объекта...
        - сигналы (реакция на нажатие/редактирование) посылать должны только кнопки/окошки редактирования текста.
        - один метод установки новых значений
        - правила перевода значений в отображение и обратно (только для числовых...)
        - ещё один метод, получить лэйаут, для того чтобы вставить эту штуку на форму. Вот.

        объект запихивается в QMap сразу после (или во время) создания.
        надо всё же иметь возможность поменять правила отображения уже после создания, т.е.
        метод setViewStyle или хз надо предусмотреть. А чтобы иметь возможность запихнуть
        его в контейнер прямо во время создания, значит, надо перегрузить конструктор.
        т.к. настройки эти меняться будут только при создании, а тот кто будет создавать
        и запихивать в контейнер знает что создаёт, а не только общего предка, то конструкторы
        можно сотворить разных форматов, и методы установки параметров отображения тоже.
        В общем виде будут только:
            - вернуть лэйаут
            - установить значение
            - сигнал.
    2.  сигнал об изменении значения регистра.
        сам объект будет иметь приватный слот для конкретного элемента отображения
        и может послать свой сигнал в виде (имя регистра, значение регистра) при редактировании пользователем.

    3.  подумать над размещением элементов на Grid Layout. Можно ли на грид лэйаут запихнуть другие лэйауты?
        можно. Отлично работает. Значит не паримся делаем рандомные лэйауты для своих элементов
        типа подпись+кнопка. Потом размещаем их на Гриде при создании.

    4.  палитра элементов отображения:

        *1).  лампочка с подписью:    что с ней делать?!
              (LedView)             на самом деле сигнал об изменении регистра, который отображается
                                    нашей лампочкой, получает обёртка для этого элемента отображения,
                                    т.е. непосредственно ViewElement, а значит мы можем для отображения
                                    использовать что угодно.

                                    Вариант использования QLabel на которой вместо текста будет лампочка:

                                    QPixmap ledOffPix,LedRedPix,LedGreenPix,LedYellowPix;
                                    ledOffPix.load("led_off.jpg");
                                    LedRedPix.load("led_red.jpg");
                                    LedGreenPix.load("led_grn.jpg");
                                    LedYellowPix.load("led_ylw.jpg");

                                    QLabel label;
                                    label.setPixmap(ledOffPix);

                                    и т.п. в зависимости от состояния.

                                    Решилось применением стилей для обычных кнопок.

        ?2).  просто подпись:   QLabel со свойствами:
                                    QFrame: frameShape=panel
                                            frameShadow=sunken
                                            lineWidth=1
                                    QWidget:    autofillBackground=1
                                                palette: Window=цвет фона
                                                         WindowText=цвет текста
              это можно реализовать в hmi.cpp т.к. тут только элементы синхронизируемые с данными контроллера.

        *3).  кнопка с подписью:  QPushButton со свойствами:
              (LedButton)           text=подпись (или вариант через QLable подпись, а кнопка просто так.)
                                    icon=картинки для дополнительной индикации нажатости.
                                    checkable=1 (т.к. нам нужны кнопки с защёлкой, или не только?)
                                        в программе для контроллер+панель мицубиши куча кнопок с продолжительным
                                        нажатием и однокрантым нажатием. Но впринципе это можно обойтись и без
                                        них, если  сделать соответствующую логику.
                Решилось через применение стилей:
                pushButton.setStyleSheet("QPushButton#pushButton:checked {color: black; background-color: green};")


        4).  текстовое окошко для редактирования (с подписью):
                            QLineEdit со свойствами:
                                    QWidget: palette:   Text=цвет текста
                                                        Base=цвет фона
                        или QSpinBox с аналогичными св-вами. Поддерживает только целые числа.
                        или QDoubleSpinBox, который поддерживает дробные числа!!! думаю он нам и нужен
                            кроме всего у него настраиваются границы редактирования и кол-во разрядов.

        *5).  движок/крутилка с окошком отображающим число и подписью
              (NumericEdit)     тут 2 варианта:
                                1: QSlider
                                2: QDial
                            + QLabel для подписи
                            + QLabel для показаний в процентах или абсолютных величинах.

        *6).  таблица редактирования
              (TableDividedByColumns)   переделать вьюшку так, чтобы можно было хранить несколько регистров.

        *7). отображение состояний (мультилампочка? мультиподпись?) (например светофор)
             (StatusView)

        *8). отображение списка неполадок.
             (AlarmList)

        *9). простой график.
             (DiagView)

        *10). лог ошибок
              (AlarmLog)

        *11). лог графический
              (DiagramLogView)
        12.0). выпадающий список, с помощью которого можно выбрать один из нескольких вариантов значений регистра.
        12.1). набор кнопок с подписями, переключающий несколько значений регистра, аналогично выпадающему списку.
              (может быть вертикальным или горизонтальным, кнопки будут реализованы как QButton, с внешним видом и настройками LedButton)
        12.2). нужно ли лепить возможность реализации тумблера, в стиле олдскульного HMI? или вообще кастомно настраиваемый переключатель?

    5. Дополнительные свойства элементов.
    Есть такие свойства жлементов, которые не всегда нужно указывать. К ним относятся все косметические
    свойства:
        *-цвет
        ...-цвет подписи
        ?-цвет фона
        *-ширина
        *-высота
        -ширина подписи
        ...-секьюрити видимость?
        ...-секьюрити доступность?

    Эти параметры можно указать уже после создания виджета. Для их установки можно сделать отдельные методы
    к базовом классе ViewElement, а в классе HMI сделать приватный метод для чтения соответствующих тегов
    из XML. И получить вместо повторяемой в каждом методе XmlMake<Widget> пачки строк - одну строку вызова
    универсального метода XmlSetAdditionalProperties(const QWidget*).

*/

/*********************************/
/*  пробуем создать статический  */
/*  класс для хранения картинок  */
/*********************************/


  /****************************/
 /*  Объявление ViewElement  */
/****************************/

/* Абстрактный класс - предок коллекции визуальных элементов HMI
 *
   Для корректной работы с размещением подписи и размерами
   в классе-потомке надо реализовать метод adjustChildrenSize,
   а в конструкторе потомка следует сделать ряд рействий для размещения
   визуального элемента на виджете и определения дефолтных размеров.
   (например для стандартных однострочных элементов это:
    placeWidgetOnLayout(numView);
    setFixedHeight(getDefaultHeight());

    а для растягиваемых элементов:
    layout()->addWidget(&frame);
    canExpand=true;
    if(caption) caption->setWordWrap(true);
    adjustCaption();
    adjustChildrenSize();)
*/

class ViewElement : public QWidget
{
    Q_OBJECT
public:
    static const int freeExpand=0xFFFFFF;

private:
    PlcRegister plcregister;

    static int defaultHeight; // как вас адекватно инициализировать?
    static QFont defaultFont;

protected:
    QLabel *caption=nullptr;
    bool canExpand=false;
    int elementHeight=0;
    int elementWidth=0;
    bool captionFixedH=false;
    bool captionFixedW=false;
    bool captionFontSizeIsSet=false;
    bool captionWordWrapIsSet=false;

    PlcRegister newValReg; // хранит значени plcreg после вызова сигнала valueChanged для представлений отображающих несколько регистров.
    int value;
    QColor color;
    int valBase;
    bool vertic;
    virtual void view()=0; // каждый наследник будет отображать значение посвоему.
    PlcRegister getPLCregister(void);
    void placeWidgetOnLayout(QWidget *widget);
    void adjustCaption();
    virtual void adjustChildrenSize(){}

public:
    explicit ViewElement(const QString &cap, const PlcRegister &plcreg,
                         bool vertical=false, QWidget *parent = nullptr);
    virtual ~ViewElement();

    static void setDefaultHeight(int h){
        defaultHeight = h;
    }
    static void setDefaultFont(const QFont &fnt){
        defaultFont = fnt;
    }
    static const QFont getDefaultFont(){
        if(defaultFont.pointSize()<0)
        {
            // вот тут аккуратно, т.к. это обращение ДО инициализации Application вызовет ошибку.
            defaultFont=QApplication::font();
        }
        return defaultFont;
    }
    static QFont getDefaultCaptionFont(){
        QFont fnt=getDefaultFont();
        fnt.setPointSize(fnt.pointSize()*0.92);
        return fnt;
    }
    static int getDefaultHeight(){
        if(!defaultHeight){
            int h = QFontMetrics(getDefaultFont()).height();
            defaultHeight=h*1.6+8;//+QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        }
        return defaultHeight;
    }
    static int getDefaultSpacing(){
        return getDefaultHeight()/6;
    }

    int getHeight(){ return elementHeight; }
    int getWidth(){ return elementWidth; }

    void setValue(const QString &val, const PlcRegister &plcreg = PlcRegister(), int base=16); // а вот установка значения у всех одинаковая.
    void setValue(int val, const PlcRegister &plcreg = PlcRegister()); // даже не знаю понадобится ли его перегружать.

    virtual void setColor(QColor col);
    void setCaptionColor(QColor col);

    void setHeight(int h);
    void setFixedHeight(int h);
    void setWidth(int w);
    void setFixedWidth(int w);
    void setFontSize(int pointSize);
    void setCaptionFontSize(int pointSize);
    void setCaptionWordWrap(bool wwrap);
    void setCaptionHeight(int h);
    void setCaptionWidth(int w);
    void setCaptionAlighnment(Qt::Alignment al);

signals:
    void valueChanged(const PlcRegister &plcreg, int val); // если виджет способен изменять value, то при изменениях он будет посылать такой сигнал.

public slots:

};

/*  надо создать несколько потомков. для разных случаев разные. каждый из них значит будет
    таки layout с кишками. Надо таки ещё заложить возможность настройки вида этой бяки.
    вообще тип отображения не будет совпадать с типом регистра. Т.к. например битовый регистр
    может быть кнопкой или лампочкой. */

/*  может для редактирования и отображения надо разные объекты? там же по идее и функционал сильно разный
    и отображалку нафига делать полем редактирования с параметром рид онли?! думаю это надо будет сделать
    ради унификации внешнего вида... хотя хз.

    сейчас надо начать изучение инструментария Qt, потому что многих вещей я просто незнаю.
    *решено- Например как сделать касивую индикацию лампочек. (извращаться с пиксмапами в лэйблах видимо)
    *решено- Или как сделать кнопки-переключатели вкл/выкл (наподобие чекбокса, но не чекбокс)
        !!! добавить пиксмапы на кнопки и сделать кнопки засчёлкиваемыща о_0
    *решено- или хотябы как поменять цвета фона и текста на лэйблах и строкахРедактирования
        ну это хотябы можно нагуглить:
        QPalette pal_lineEdit(this->palette());
        pal_lineEdit.setColor(QPalette::Text, Qt::red); //цвет текста
        pal_lineEdit.setColor(QPalette::Base, Qt::yellow); //цвет фона
        lineEdit->setPalette(pal_lineEdit);
    *решено- чтбы метки имели непрозрачный фон, есть такая опция:
        label.setAutoFillBackground(true);
        (она есть и в гуёвом редакторе, если внимательно смотреть)
        в качестве фона можно устанавливать картинку - смотри учебник стр.99-100
    *решено - есть ещё вопрос по редактированию сетки... сетки? чтото типа екселя... таблиц..
        !!! короче QTableWidget - а тама разберёсся.
    - графики буду делать сильно потом...

    * пока не актуально, т.к. ввод цифр будет через такую штучку с плюсиком и минусиком, *
    * там только цыферы и максимальное-минимальное значение ограничивается по желанию *
    * numericEdit должен сообщать новое значение только ПОСЛЕ редактирования и не лезть с ограничениями mix-max ДО полного ввода значения

    такс!! надо ноутбук с сенсорным экраном!!! чтобы можно быть имитировать работу с панельным писи.
    */



  /****************************/
 /*  Объявление NumericView  */
/****************************/

class NumericView: public ViewElement
{
    Q_OBJECT
public:
    explicit NumericView(const QString &cap, const PlcRegister &plcreg, int decDigits = 0,
                         bool vertical=false, QWidget *parent = nullptr);

    void setColor(QColor col);

private slots:

private:
    virtual void view();
    virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)

    QLabel *numView;
    int decimalDigits; // кол-во знаков после десятичной точки. По сути степень в которую надо возвести 10 чтобы получить множитель.
    double divider;
};



  /****************************/
 /*  Объявление NumericEdit  */
/****************************/

class NumericEdit: public ViewElement
{
    Q_OBJECT
public:
    explicit NumericEdit(const QString &cap, const PlcRegister &plcreg, int min=0, int max=1000,
                         int decDigits = 0, bool vertical=false, QWidget *parent=nullptr);

    void setColor(QColor col);

    /*
        редактировать цифры в этом виджете просто невозможно, от каждое нажатие обновляет значение,
        а кроме этого каждое обновление данных с контроллера берет и меняет значение в ячейке.

        Что можно с этим сделать?
        1. не принимать обновленные значение с контроллера пока:
            а. ячейка в фокусе - как различить редактирование и нередактирование?
            б. какое-то время после нажатия кнопки? или изменения ячейки?
        2. не отправлять новые значения какое-то время после изменения
        3. отправлять новые значения сразу после нажатия [enter] или потере фокуса,
            но только если до этого было изменение?
    */
private slots:
    void changeValue(double newVal); // ловит изменение значений и запускает таймер
    void onChangeValueTimer(); // посылает сигнал valueChanged

private:
    virtual void view();
    virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)

    bool eventFilter(QObject * o, QEvent * e);

    QDoubleSpinBox numEdit;
    QTimer valChangeTimer;
    int editedValue;
    int decimalDigits; // кол-во знаков после десятичной точки. По сути степень в которую надо возвести 10 чтобы получился множитель.
    double divider;
};


/*   Объявление  customLed для LedView   */

class CustomLed: public QWidget
{
private:
    bool on=false;
    QColor cLedOn=QColor("#F10");
    QColor cLightOn=QColor("#FAA");
    QColor cLedOff=QColor("#776");
    QColor cLightOff=QColor("#776");
    const QColor light=QColor("#EED");
    const QColor shade=QColor("#665");
    const QColor darkborder=QColor("#332");
    const QColor gray=QColor("#776");
    const QColor border=QColor("#443");

    QString pixmapOn,pixmapOff;
    static QMap<QString,QPixmap> pixmaps;
public:
    void paintEvent(QPaintEvent *);
    void setStateOn(bool stateOn);
    void setColor(QColor color, bool inversed=false);
    void setExactColors(QColor color_on, QColor color_off);
    void setPixmaps(QString pixmap_on, QString pixmap_off);
    bool isOn();
};


/****************************/
/*   Объявление  LedView    */
/****************************/

class LedView: public ViewElement
{
  Q_OBJECT
public:
  explicit LedView(const QString &cap, const PlcRegister &plcreg,
                   bool vertical=false, QWidget *parent=nullptr);
    void setColor(QColor color);
    void setColor(QColor color, bool inversed);
    void setExactColors(QColor color_on, QColor color_off);
    void setImages(QString imageName_on, QString imageName_off);

private slots:

private:
  virtual void view();
  virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)

  //QPushButton ledView;
  CustomLed ledView;
};

/*  custon button for LedButton class  */
class CustomPushButton: public QPushButton
{
private:
    QString text_on="вкл";
    QString text_off="выкл";
public:
    void setTextOn(const QString texton);
    void setTextOff(const QString textoff);
    void paintEvent(QPaintEvent *);
};



/****************************/
/*  Объявление  LedButton   */
/****************************/

class LedButton: public ViewElement
{
  Q_OBJECT
public:
  explicit LedButton(const QString &cap, const PlcRegister &plcreg, bool vertical=false, QWidget *parent = nullptr);
  // !!! отображение цветов не доделано!!! сделать так же как в CustomLedView!!!

  //void setTextColor(QColor col){}; // понадобится только если использовать надписи на самих кнопках
  void setColor(QColor color); //, bool inversed=false); - инверсия не доделана и возможно оно и не надо?
// void setExactColors(QColor color_on, QColor color_off);
// void setImages(QString imageName_on, QString imageName_off);
  void setTextOn(const QString texton);
  void setTextOff(const QString textoff);
  void setConfirmOn(const QString text);
  void setConfirmOff(const QString text);

private slots:
  void changeValue(); // будет ловить изменение значений

private:
  virtual void view();
  virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)

  bool no_view=false;
  QString confirmOn;
  QString confirmOff;
  CustomPushButton button;
};

/****************************/
/*  Объявление  NumButton   */
/****************************/

class NumButton: public ViewElement
{
  Q_OBJECT
public:
  explicit NumButton(const QString &cap, const PlcRegister &plcreg, int value, bool vertical=false, QWidget *parent = nullptr);
  // !!! отображение цветов не доделано!!! сделать так же как в CustomLedView!!!

  // void setTextColor(QColor col){}; // понадобится только если использовать надписи на самих кнопках
  void setColor(QColor color); //, bool inversed=false); - инверсия не доделана и возможно оно и не надо?
  // void setExactColors(QColor color_on, QColor color_off);
  // void setImages(QString imageName_on, QString imageName_off);
  void setTextOn(const QString texton);
  void setTextOff(const QString textoff);
  void setConfirmOn(const QString text);
  void setConfirmOff(const QString text);

private slots:
  void changeValue(); // будет ловить изменение значений

private:
  virtual void view();
  virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)

  int theValue=1;
  bool no_view=false;
  QString confirmOn;
  QString confirmOff;
  CustomPushButton button;
};


/******************************/
/*   Объявление  StatusView   */
/******************************/

class StatusView: public ViewElement
{
  Q_OBJECT
public:
    explicit StatusView(const QString &cap, const PlcRegister &plcreg,
                      const QList<int> &regValues, const QList<QString> &valueLabels, const QList<QColor> labelColors,
                      bool vertical=false, QWidget *parent=0);

private slots:

private:
  virtual void view();
  virtual void adjustChildrenSize(); // для подгонки размеров содержимого под размер виджета (или под размер шрифта)
  QLabel statusView;
  QList<int> values;
  QList<QString> labels;
  QList<QColor> colors;

};


/******************************/
/*   Объявление  AlarmList    */
/******************************/

class AlarmList: public ViewElement
{
  Q_OBJECT
public:
  explicit AlarmList(const QString &cap, int rowsCount,
                     const QList<PlcRegister> &registers, const QList<QString> &regLabels,
                     const QList<QColor> &labelColors, bool vertical=true, QWidget *parent=0);

private slots:

private:
  virtual void view();
  virtual void adjustChildrenSize();
  void paintAlarmRow(int row);

  QFrame frame;
  QVector<int> vals;
  QList<PlcRegister> regs;
  QList<QLabel*> alarmViews;
  QList<PlcRegister> alarmRegs;
  QList<QString> labels;
  QList<QColor> colors;
  int rowscount;

};



/*****************************/
/*   Объявление  AlarmLog    */
/*****************************/

class AlarmLog: public ViewElement
{
  Q_OBJECT
public:
  explicit AlarmLog(const QString &cap, const QString &name, const QList<PlcRegister> &registers,
                    const QList<QString> &regLabels, const QList<QString> &regTooltips,
                    const QString dateformat, bool vertical=true, QWidget *parent=nullptr);

private slots:

private:
  virtual void view();
  virtual void adjustChildrenSize();

  AlarmLogStringModel alarmmodel;
  QListView alarmview;
  QList<PlcRegister> regs;
  int rowscount;

};


/*****************************/
/*    Объявление  Table      */
/*****************************/

class TableDividedByColumns: public ViewElement
{
  Q_OBJECT
public:
  explicit TableDividedByColumns(const QString &cap, int rowcount, const QList<QString> &columnlabels,
                                 const QList<PlcRegister> &columnregisters, const QList<int> &columnregintervals,
                                 bool vertical=true, QWidget *parent=nullptr);
  void addTextList(QList<QStringList> *textlist);
  void addTypeList(QList<QStringList> *typelist);
  void setColumnTypes(const QList<int> &columnMinValues, const QList<int> &columnMaxValues,const QList<QString> &columnTypes);

private slots:
  void changeValue(int row, int column); // будет ловить изменение значений
  void showContextMenu(const QPoint &p);

  void insertRowSlot();
  void deleteRowSlot();
  void copySelectedSlot();
  void pasteSelectedSlot();
  void cutSelectedSlot();

private:
  virtual void view();
  void adjustChildrenSize();
  bool eventFilter(QObject *target, QEvent *event);

  bool expandFlagBoxList=false;
  QMenu *contextMenu;
  QAction *insertRowAction,
          *deleteRowAction,
          *separatorAction,
          *copySelectedAction,
          *pasteSelectedAction,
          *cutSelectedAction;

  QList<QStringList> *textLists=nullptr; // как его хранить? в виде набора указателей? или копией?
  QList<QStringList> *typeLists=nullptr; // пока упростим себе жизнь с переводом индексов коллекций строк из HMI передачей указателя на ВЕСЬ набор. Да, это опасная избыточность.
  QList<int> regintervals;
  QList<QString> asd;
  QList<PlcRegister> regs;
  int cols;
  int rows;
  QTableWidget table;
};



/****************************************/
/*   Объявление  DiagViewDirectPaint    */
/****************************************/

/* класс делает что?
 * 1. по таймеру в буфер складываем данные
 * 2. заполняем набор массивов точек
 * 3. вызываем метод отрисовки, в котором на виджет рисуем фон,сетку и эти линии
 * мышь:
 * 1. по событию движение мыши
 * 2. определяем позицию, сохранием время и значения на графике
 * 3. вызываем метод отрисовки
 */


class DiagViewDirectPaint: public ViewElement
{
  Q_OBJECT
public:
  explicit DiagViewDirectPaint(const QString &cap, int interval,
                    const QList<PlcRegister> &lineRegisters, const QList<int> &lineDecimals, const QList<int> &lineMaxs,
                    const QList<QString> &lineLabels, const QList<QColor> lineColors, const QColor backgroundColor=Qt::white,
                    bool vertical=true,QWidget *parent=nullptr);
  // ~DiagView(); деструктор не нужен, т.к. все выделенные объекты являются дочерними к this и удаляться автоматически

private slots:
  void onTimer();

private:
  class Screen: public QWidget
  {
  public:
      explicit Screen(DiagViewDirectPaint* parent);
      void paintEvent(QPaintEvent *);
  private:
      DiagViewDirectPaint *parent;
  };

  virtual void view(); // получение новых данных
  void adjustChildrenSize();
  bool eventFilter(QObject *target, QEvent *event);
  void bufferResize(int num);
  void updateDiag();
  void updateCursor();

  Screen *screen; // пустой виджет для отрисовки графика
  QVector<int> values; // сюда будут складываться значения регистров в методе view()
  QVector<int> buffer; // все линии в одном массиве группами по linesCount точек
  int position=0; // указывает на самую старую по времения ячейку
  int linesCount=0;
  QTime lastPointTime;
  int cursorPos=0;
  QTime cursorTime;
  QVector<int> cursorVal;
  QVector<QPolygon> lines; // линии для отрисовки на сцене
  QTimer *timer; // а этот таймер будет раз в period складывать значения в buffer
  int labelsMaxCharCount=0;
  int valuesMaxCharCount=0;
  QList<PlcRegister> registers;
  QList<int> dividers;
  QList<int> decimals;
  QList<int> maxs;
  QList<QString> labels;
  QList<QColor> colors;
  QColor background;
};


#endif // VIEWELEMENT_H
