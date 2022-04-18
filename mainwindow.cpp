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

#include <QTextBlock>
#include <QItemDelegate>
#include <QTime>
#include <qserialport.h>
#include <qfiledialog.h>
#include <qinputdialog.h>

#include <qcryptographichash.h>
#include <qmessagebox.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "fatekcommrs.h"
#include "resources.h"
#include "plcregister.h"

#include <QStandardItemModel>

#include <qlistwidget.h>
#include <qlistview.h>
#include <QStandardItemModel>
#include <QWhatsThis>
#include <QToolTip>
#include <QAbstractScrollArea>

#include "logmodel.h"

#include <QtGui>
#include <QComboBox>
#include <QStyleFactory>
#include <QDesktopWidget>
#include <QScrollBar>

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        // убираем обработку скроллинга на окне. Виджеты и скроллбары при этом будут его обрабатывать.
        event->accept();
        return true;
    }
    return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // если этого не сделать, то эти окна могут остаться висеть не в тему.
    msgBox.close();
    //commwindow.close(); // чтобы убрать этот кастыль - при создани окна делаем setAttribute(Qt::WA_QuitOnClose, false);
    //about.close();
    // дальше не мешаем программе отрабатывать закрытие.
    QMainWindow::closeEvent(event);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->actionOpenCommSettings, &QAction::triggered, this, &MainWindow::showCommWindow);
    QObject::connect(ui->actionFullScreen, &QAction::triggered, this, &MainWindow::toggleFullscreen);
    QObject::connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutWindow);

    // внедряем перевод в интерфейс:
    ui->menuSettings->setTitle(Resources::translate("settings"));

    ui->menuSettings->actions().at(0)->setText(Resources::translate("connection"));
    ui->menuSettings->actions().at(2)->setText(Resources::translate("fullscreen"));
    ui->menuAbout->setTitle(Resources::translate("info"));
    ui->menuAbout->actions().at(0)->setText(Resources::translate("about menu"));

    ui->statusBar->addWidget(new QLabel(Resources::translate("device")+":", this));
    ui->statusBar->addWidget(&portName);
    ui->statusBar->addWidget(new QLabel(":",this));
    ui->statusBar->addWidget(&portStatus);
    ui->statusBar->addWidget(new QLabel("  ",this));
    ui->statusBar->addWidget(new QLabel(Resources::translate("plc")+":", this));
    ui->statusBar->addWidget(&plcStatus);
    ui->statusBar->addWidget(new QLabel("  ",this));
    ui->statusBar->addWidget(new QLabel(Resources::translate("sending")+":", this));
    ui->statusBar->addWidget(&commStatus);
    QObject::connect(&statusUpdateTimer,&QTimer::timeout,this,&MainWindow::statusUpdate);
    statusUpdateTimer.start(500);

    // пробуем раскрасить фон - работает криво шо ппц
    //noconnect_color_effect = new QGraphicsColorizeEffect();
    //noconnect_color_effect->setColor(Qt::gray);
    //noconnect_color_effect->setStrength(0.3);
    //ui->hmiScrollArea->setGraphicsEffect(noconnect_color_effect);

    // месага должна подниматься на верх экрана, убираться при потере фокуса и не дублироваться.
    // всё это надо чтобы при нестабильной связи данное сообщение не блокировало работу с программой.
    msgBox.setModal(false);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowFlags(Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint|Qt::Popup);

    QObject::connect(ui->actionTest,&QAction::triggered, this, &MainWindow::testmenu);
    ui->hmiScrollArea->viewport()->installEventFilter(this); // фильтр скроллинга на окне (чтобы избежать конфликтов например с QTabWidget)

    //about.setParent(this);
    //about.setWindowFlags(about.windowFlags() | Qt::Window);
}

MainWindow::~MainWindow()
{
    //dataCollector = 0;
    if(hmiWidget)hmiWidget->setParent(nullptr);
    delete ui;
}

void MainWindow::setCommunicator(PlcCommRS485 *newcomm)
{
    commwindow.setSettingsFilename(Consts::workDirPath+"/commsettings.cfg");
    commwindow.setCommunicator(newcomm);
    comm = newcomm;
}

void MainWindow::addHmi(QWidget *wid, QList<QMenu*> menus)
{
    if(!wid) return; // нельзя быть уверенным, что нам не подсунут пустышку.

    hmiWidget = wid;
    hmiWidget->show(); // это необходимо, т.к.
    hmiWidget->adjustSize(); // после этого виджет становится своего естественного размера
    QSize hmiSize=hmiWidget->size();
    ui->hmiScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // чтобы узнать реальные размеры скроллбаров
    ui->hmiScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    show(); // это небходимо, т.к. иначе scrollArea будет дефолтного размера 100x30
    adjustSize(); // на всякий случай...

    int vsw=ui->hmiScrollArea->verticalScrollBar()->width();
    int hsw=ui->hmiScrollArea->horizontalScrollBar()->width();

    // есть ли способ ПРОСТО узнать размер содержимого QScrollArea, без вычислений и добавлений фиктивных виджетов?
    // (ui->hmiScrollArea->contentsRect().size() всего на пару пикселов меньше чем ui->hmiScrollArea->size())
    QSize addSize = size()-ui->hmiScrollArea->contentsRect().size()+QSize(vsw,hsw); // получаем размер без учета области, где будет размещен HMI

    ui->hmiScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->hmiScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->hmiScrollArea->setWidget(hmiWidget); // собственно размещаем HMI на окне скроллирования
    resize(hmiSize+addSize); // и наконец подгоняем размер главного окна вручную, т.к. само оно этого сделать не способно

    foreach(QMenu* menu, menus){
        if(menu!=nullptr) ui->menuBar->addMenu(menu);
    }

    // когда DEBUG не нужен:
    if(!DEBUG_TEST)
        ui->menuBar->actions().at(2)->setVisible(false);
}

void MainWindow::showCommWindow()
{
    if(commwindow.isVisible()){
        if(commwindow.isMinimized())commwindow.showNormal();
        commwindow.raise();
    }
    else commwindow.show();
}

void MainWindow::toggleFullscreen()
{
    this->setWindowState(this->windowState()^Qt::WindowFullScreen);
}

void MainWindow::showAboutWindow()
{
    if(about.isVisible()){
        if(about.isMinimized())about.showNormal();
        about.raise();
    }
    else about.show();
}

void MainWindow::statusUpdate()
{
    if(!comm)return;

    portName.setText(comm->getPortName());
    QPalette pal=portStatus.palette();
    if(comm->portIsOpen()){
        pal.setColor(QPalette::WindowText,Qt::darkGreen);
        portStatus.setText(Resources::translate("opened"));
        portStatus.setPalette(pal);
    }
    else{
        pal.setColor(QPalette::WindowText,Qt::darkRed);
        portStatus.setText(Resources::translate("closed"));
        portStatus.setPalette(pal);
    }

    pal=commStatus.palette();
    if(comm->communicationIsOn()){
        pal.setColor(QPalette::WindowText,Qt::darkGreen);
        commStatus.setText(Resources::translate("sending on"));
        commStatus.setPalette(pal);
    }
    else{
        pal.setColor(QPalette::WindowText,Qt::darkYellow);
        commStatus.setText(Resources::translate("sending off"));
        commStatus.setPalette(pal);
    }

    pal=plcStatus.palette();
    switch(comm->plcConnectionStatus()){
    case Ok:
        pal.setColor(QPalette::WindowText,Qt::darkGreen);
        plcStatus.setText(Resources::translate("connected"));
        plcStatus.setPalette(pal);
        //noconnect_color_effect->setStrength(0);
        noconnect_msg_may_show=true;
        break;

    case NoConnection:
        pal.setColor(QPalette::WindowText,Qt::darkRed);
        plcStatus.setText(Resources::translate("no connection"));
        plcStatus.setPalette(pal);
        //noconnect_color_effect->setStrength(0.35);
        // надо чтобы это сообщение появлялось только после ПРОПАДАНИЯ связи, т.е. если до этого связь была.
        if(noconnect_msg_may_show)
        {
            msgBox.setText(Resources::translate("no connection"));
            if(msgBox.isHidden())msgBox.show();
            else msgBox.raise();
            noconnect_msg_may_show=false;
        }
        break;

    case UnknownError:
        pal.setColor(QPalette::WindowText,Qt::darkYellow);
        plcStatus.setText(Resources::translate("unknown"));
        plcStatus.setPalette(pal);
        //noconnect_color_effect->setStrength(0.35);
        break;

    default: pal.setColor(QPalette::WindowText,Qt::darkYellow);
        plcStatus.setText(Resources::translate("plc error"));
        plcStatus.setPalette(pal);
        break;
    }
}


void MainWindow::testmenu()
{

    qDebug()<<quint8(-1);
    qDebug()<<quint8(-127);

    return;

    QFile file("/home/sashka1/test.hex");
    //file.setFileName(Consts::workDirPath+"\test.dat");
    if(!file.open(QIODevice::WriteOnly)){
        qDebug()<<"Can't open diagram file for saving"; //"Ошибка открытия файла для сохранения графика";
        return;
    }
    QDataStream fileStream(&file);

    int value=254;
    QByteArray val_ar;
    qint8 val8=122;
    short val16=255;
    signed char val_char=121;
    fileStream<<value;
    fileStream<<val16;
    fileStream<<val8;
    fileStream<<val_char;
    file.close();

    QFile file4read("/home/sashka1/test.hex");
    //file.setFileName(Consts::workDirPath+"\test.dat");
    if(!file4read.open(QIODevice::ReadOnly)){
        qDebug()<<"Can't open diagram file for saving"; //"Ошибка открытия файла для сохранения графика";
        return;
    }
    QDataStream readStream(&file4read);
    QByteArray byteArr;
    QVector<short> shortArr;
    QVector<int> intArr;

    readStream>>value;
    readStream>>val16;
    readStream>>val8;
    readStream>>val_char;

    intArr.append(value);
    byteArr.append(val_char);

    file4read.close();
    qDebug()<<value<<" "<<val16<<" "<<val8<<" ";
    qDebug()<<intArr<<"  "<<shortArr<<"  "<<byteArr;

    return;

    QString str="123";
    QByteArray utf8=str.toUtf8().append(char(0));
    qDebug()<<str;
    qDebug()<<utf8.count();
    qDebug()<<utf8;


    return;

    //QMessageBox::information(nullptr, "local language", QLocale().name() + " - " + QString::number(QLocale().language()));

    qDebug()<<QLocale().name();
    qDebug()<<QLocale().language();

    qDebug()<<Resources::dictionary.keys();

    return;

    ui->menuBar->actions().at(2)->setVisible(false);

    return;
    /*
    QWidget *w=new QWidget;
    QDoubleSpinBox *spb = new QDoubleSpinBox;
    QDoubleSpinBox *spb2 = new QDoubleSpinBox;
    w->setLayout(new QVBoxLayout);
    w->layout()->addWidget(spb);
    w->layout()->addWidget(spb2);
    w->show();

    return;
    */

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Test", "Quit?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      qDebug() << "Yes was clicked";
      QApplication::quit();
    } else {
      qDebug() << "Yes was *not* clicked";
    }

    return;

    QStringList lst;
    QStandardItemModel *model=new QStandardItemModel(3, 1); // 3 rows, 1 col
    for (int r = 0; r < 3; ++r)
    {
        QStandardItem* item = new QStandardItem(QString("Item %0").arg(r));

        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);

        model->setItem(r, 0, item);
        lst<<QString("Item %0").arg(r);
    }

    QComboBox* combo = new QComboBox();
    combo->setModel(model);
    combo->setStyle(QStyleFactory::create("Windows"));

    qDebug()<<QStyleFactory::keys();

    QListView* list = new QListView();
    list->setModel(model);

    QTableView* table = new QTableView();
    table->setModel(model);

    QWidget *container=new QWidget();
    QVBoxLayout* containerLayout = new QVBoxLayout();
    container->setLayout(containerLayout);
    containerLayout->addWidget(combo);
    containerLayout->addWidget(list);
    containerLayout->addWidget(table);

    QComboBox *editor = new QComboBox;
    QListWidget *rudiment = new QListWidget;
    foreach(QString ls,lst){
        QListWidgetItem *li = new QListWidgetItem(ls);
        li->setFlags(Qt::ItemIsUserCheckable| Qt::ItemIsEnabled);
        li->setCheckState(Qt::Unchecked);
        rudiment->addItem(li);
    }
    editor->setModel(rudiment->model());
    editor->setView(rudiment);
    containerLayout->addWidget(editor);
    editor->addItem("123asd");
    rudiment->item(3)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    rudiment->item(3)->setCheckState(Qt::Unchecked);

    container->show();

    return;

    qDebug()<<QDate::currentDate().toString();
    qDebug()<<QDate::currentDate().toString("dd.MM.yyyy");

    return;

    if(!file.open(QIODevice::WriteOnly)){
        qDebug()<<"Can't open file "<<file.fileName(); //"Ошибка открытия файла для сохранения графика";
        return;
    }
    //QDataStream stream(&file);
    QByteArray data;
    int dat=-666;

    data.append((dat>>24)&0xFF);
    data.append((dat>>16)&0xFF);
    data.append((dat>>8)&0xFF);
    data.append((dat)&0xFF);
    uchar* bytes=reinterpret_cast<uchar *>(&dat);
    data.append(bytes[3]);
    data.append(bytes[2]);
    data.append(bytes[1]);
    data.append(bytes[0]);

    file.write(data);
    //stream<<dat;
    //stream.writeBytes(bytes, sizeof(dat));

    file.close();

    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"Can't open file "<<file.fileName();//"Ошибка открытия файла для сохранения графика";
        return;
    }
    data.clear();
    data=file.readAll();

    dat=(static_cast<uchar>(data.at(0))<<24)|(static_cast<uchar>(data.at(1))<<16)
            |(static_cast<uchar>(data.at(2))<<8)|(static_cast<uchar>(data.at(3)));
    qDebug()<<dat<<" - "<<bytes[0]<<bytes[1]<<bytes[2]<<bytes[3];

    bytes[3]=data.at(0);
    bytes[2]=data.at(1);
    bytes[1]=data.at(2);
    bytes[0]=data.at(3);
    qDebug()<<dat<<" - "<<bytes[0]<<bytes[1]<<bytes[2]<<bytes[3];

    uchar num1=201;
    char num2=num1;
    uchar num3=num2;
    qDebug()<<num1<<" "<<(int)num2<<" "<<num3;

    int num4=(uchar)num2 - 128;
    qDebug()<<num4;

    int v2=static_cast<uchar>(char(0xFF));
    qDebug()<<v2;
    int v3=char(0xFF);
    qDebug()<<v3;

    // end of cast test
    return;

    QVector<int> vect;
    int *arr;
    QTime start,finish,time_vect,time_arr;
    int avr;

    int aSize=99999999;

    start=QTime::currentTime();
    vect.reserve(aSize);
    //vect.resize(aSize);
    for(int i=0;i<aSize;i++){
        vect.append(i);
        //vect.replace(i,i);
    }
    qDebug()<<"init vector = "<<start.msecsTo(QTime::currentTime());

    start=QTime::currentTime();
    arr = new int[aSize];
    for(int i=0;i<aSize;i++){
        arr[i]=i;
    }
    qDebug()<<"init C array = "<<start.msecsTo(QTime::currentTime());


    start=QTime::currentTime();
    for(int i=0;i<aSize;i++){
        avr=(avr+vect.at(0))/2;
    }
    qDebug()<<avr<<" -- read vector = "<<start.msecsTo(QTime::currentTime());

    start=QTime::currentTime();
    for(int i=0;i<aSize;i++){
        //arr[i]=1245;
        avr=(avr+arr[i])/2;
    }
    qDebug()<<avr<<" -- read C array = "<<start.msecsTo(QTime::currentTime());

    delete arr;

    QWidget *somewindow = new QWidget();
    QLayout *somelayout = new QVBoxLayout();

    QListView *somelistView = new QListView();
    QTableView *sometableView = new QTableView();
    QStandardItemModel *somelistModel = new QStandardItemModel();
    AlarmLogStringModel *alarmModel = new AlarmLogStringModel();

    somelistView->setStyleSheet("QToolTip { color: #000; background-color: #ff0; border-width: 1px; border-style: solid; border-color: #000;}");
    alarmModel->setDateFormat("dd/MM/yy HH:mm:ss");
    somewindow->setLayout(somelayout);

    sometableView->setModel(somelistModel);
    somelistView->setModel(alarmModel);
    somelistView->setSelectionMode(QAbstractItemView::NoSelection);
    somelayout->addWidget(somelistView);
    somelayout->addWidget(sometableView);


    alarmModel->setAlarmsText({"123","234","345"});
    alarmModel->setAlarmsDescription({"asd","sdf","dg"});
    alarmModel->setLogFile("/home/sashka1/testalarmlog.txt");
    alarmModel->setAlarm(0);
    alarmModel->setAlarm(0);
    alarmModel->setAlarm(1);
    alarmModel->setAlarm(2);
    alarmModel->resetAlarm(2);

    QModelIndex indx;
    somelistModel->insertColumns(0,2);
    somelistModel->insertRows(0,30);
    for(int i=0;i<30;i++){
        int somecolornum=rand()/(RAND_MAX/5);
        indx = somelistModel->index(i,0);

        QString order = QTime::currentTime().toString()+" - ("+ QString("%1").arg(i,2,10,QChar('0')) + ") ";//QString().number(i,'g',2);

        somelistModel->setData(somelistModel->index(i,1),i);

        switch(somecolornum){
        case 0:
            somelistModel->setData(indx,order+"\"red\"",Qt::DisplayRole);
            somelistModel->setData(indx,QColor(Qt::darkRed),Qt::TextColorRole);
            somelistModel->setData(indx,
                                   "<b>Не работает насос охлаждения РК при температуре садки более 350 градусов.</b><p>"
                                   "1. Убедиться, что включен автомат QF7 \"ОХЛАЖДЕНИЕ РК\". Убедиться, что включено реле LS6 и пускатель KM5.<p>"
                                   "2. Проверить исправность насоса, электроконтактного манометра, завоздушенности системы.<p>"
                                   "3. Низкое давление воды в подводящей магистрали.",
                                   Qt::ToolTipRole);
            break;
        case 1:
            somelistModel->setData(indx,order+"\"green\"",Qt::DisplayRole);
            somelistModel->setData(indx,QColor(Qt::darkGreen),Qt::TextColorRole);
            somelistModel->setData(indx,
                                   "<h2>Сбой в цепи питания или управления ДВН.</h2><p>"
                                   "1.убедиться, что включен автомат QF13 \"ДВН\".<p>"
                                   "2.На индикаторной панели частотного преобразователя должна отображаться текущая частота в герцах.<p>"
                                   "В противном случае, если это сбой по питанию или управлению, то необходимо выключить QF13, подождать<p>"
                                   "примерно 2 минуты пока погаснет индикация и снова включить.",
                                   Qt::ToolTipRole);
            break;
        case 2:
            somelistModel->setData(indx,order+"\"yellow\"",Qt::DisplayRole);
            somelistModel->setData(indx,QColor(Qt::darkYellow),Qt::TextColorRole);
            somelistModel->setData(indx,
                                   "<h3>Нет питания РРГ, не исправен или нет питания клапана.</h3><p>"
                                   "1.Проверить свечение индикатора на блоках питания G5,G5A, мигающее или отсутствуещее свечение сигнализирует о неисправности источника питания."
                                   "2.Проверить исправность клапана, РРГ10.",
                                   Qt::ToolTipRole);
            break;
        case 3:
            somelistModel->setData(indx,order+"\"blue\"",Qt::DisplayRole);
            somelistModel->setData(indx,QColor(Qt::darkBlue),Qt::TextColorRole);
            somelistModel->setData(indx,
                                   "<h3>Закончился газ в баллоне или не отрегулирована подача газа.</h3><p>"
                                   "1.Заменить баллон.\n"
                                   "2.Установить редуктором баллона давление на выходе в пределах 0.5-1 Атм.\n"
                                   "3.При значительном превышении давления(выше 2атм) стравить излишки газа открытием клапана и подачей расхода ~20л/ч.",
                                   Qt::ToolTipRole);
            break;
        default:
            somelistModel->setData(indx,order+"\"пусто\"",Qt::DisplayRole);
            somelistModel->setData(indx,QColor(Qt::darkGray),Qt::TextColorRole);
        }
    }

    somewindow->show();

    return;

    // это было что-то тестовое. Зачем не помню.

    QString pass = QInputDialog::getText(nullptr, "Расширенный доступ", "Введите пароль:", QLineEdit::Password, "");
    qDebug()<<pass;
    QString hash = QString(QCryptographicHash::hash(pass.toUtf8(),QCryptographicHash::Md5).toHex());
    qDebug()<<hash;
    if(pass.isEmpty()) QMessageBox::warning(nullptr,"Расширенный доступ","Пароль не подходит");

    return;

    for(int i=0;i<1000;i++){
    QWidget *widget=new QWidget();
    QPointer<QMenu> pMenu=new QMenu("тестовое миню",widget);
    QPointer<QAction> pAct1=pMenu->addAction("пип 1");
    QPointer<QAction> pAct2=pMenu->addAction("пип 2");

    //QAction::setVisible();

    //qDebug()<<pMenu->parent();
    //qDebug()<<pMenu->menuAction()->parent();

    QPointer<QAction> pMenuAct=ui->menuBar->addMenu(pMenu);
    //pMenu->menuAction()->setEnabled(false);

    //qDebug()<<pMenu->parent();
    //qDebug()<<pMenu->menuAction()->parent();
    //qDebug()<<ui->menuBar->actions().last()->parent();

    QPointer<QMenu> pMenuSub = pMenu->addMenu("аха");
    //qDebug()<<pMenuSub->parent();
    //qDebug()<<pAct1.isNull();

    //delete pMenu;
    //ui->menuBar->removeAction(ui->menuBar->actions().last());
    //ui->menuBar->actions().last()
    //qDebug()<<pMenuAct->iconText();
    //qDebug()<<pMenu->title();
    //qDebug()<<pMenu->actions().count();
    //pAct1->setVisible(false);
    //pAct2->setDisabled(true);
    //qDebug()<<pAct2.isNull();
    //delete pMenu;
    delete widget;
    }
}

int MainWindow::setAppTitle(QString cap)
{
    // пробуем проверить наличие cap в sharedMem
    setWindowTitle(cap);
    return 1;
}

inline int hash(QString &reg){
    int move=5;
    const QChar* str=reg.data();
    return (((int(str[0].toLatin1())<<move)^int(str[1].toLatin1()))<<move)^int(str[2].toLatin1());
}
