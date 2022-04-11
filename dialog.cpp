#include "dialog.h"
#include "ui_dialog.h"
#include <QtMath>
#include <qdebug.h>
#include <QMetaEnum>
#include <qscrollbar.h>
#include <qopenglwidget.h>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    //ui->graphicsView->setAlignment(Qt::AlignRight|Qt::AlignBottom);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene=new QGraphicsScene;
    scene->installEventFilter(this);

    ui->graphicsView->setScene(scene);
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->viewport()->installEventFilter(this);

    //ui->graphicsView->horizontalScrollBar()->setInvertedControls(true);
    ui->graphicsView->horizontalScrollBar()->installEventFilter(this);


//    ui->graphicsView->horizontalScrollBar()->installEventFilter(this);
//    ui->graphicsView->verticalScrollBar()->installEventFilter(this);
    ui->graphicsView->installEventFilter(this);

    count=200;
    buffer=new int[count];
    for(int i=0;i<count;i++)buffer[i]=0;
    lines=new QGraphicsLineItem*[count];
    for(int i=0;i<count;i++)lines[i]=0;

    paintTimer=new QTimer(this);
    QObject::connect(paintTimer,SIGNAL(timeout()),this,SLOT(on_pushButton_clicked()));
    //paintTimer->start(200);
}

bool Dialog::eventFilter(QObject *target, QEvent *event)
{
    //qDebug()<<target->metaObject()->className()<<" - "<<event->type();

    if(target==scene){
        if(event->type()==QEvent::GraphicsSceneMouseMove){
            QGraphicsSceneMouseEvent *sceneEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
            xPos=sceneEvent->scenePos().x();
            addCursor();
            return true;
        }
    }
    else if(target==ui->graphicsView->viewport()){
        if(event->type()==QEvent::Resize){
            QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
            int h=resizeEvent->size().height();
            ui->graphicsView->setSceneRect(0,0,count,h);
            ui->graphicsView->horizontalScrollBar()->setSliderPosition(ui->graphicsView->horizontalScrollBar()->maximum());
            bufShift=0;
            repaintBuffer();
            return true;
        }
        else if(event->type()==QEvent::Wheel){
            ui->graphicsView->horizontalScrollBar()->event(event);
            return true;
        }
    }
   // else if(target==ui->graphicsView->horizontalScrollBar()){
        //qDebug()<<"horizScroll event "<<event->type();
     //   if(event->type()==QEvent::Resize){
            //qDebug()<<"horizScroll resize event";
       //     QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
            //resizeEvent->size().
            //ui->graphicsView->horizontalScrollBar()->setSliderPosition(ui->graphicsView->horizontalScrollBar()->maximum());
         //   return true;
       // }
    //}
    return false;
}

Dialog::~Dialog()
{
    delete ui;
    delete buffer;
}

void Dialog::on_pushButton_clicked()
{
//    qDebug()<<"-------- new point ----------";
    static int dY;
    dY+=qrand()%40-20;
    int value=buffer[position]+dY;
    if(value<0){value=0;dY=0;}
    else if(value>400){value=400;dY=0;}
    addValue(value);
    repaintBuffer();
}

void Dialog::addValue(int value)
{
    int h=ui->graphicsView->viewport()->height();
    int y1;
    if(position<=0)y1=buffer[count-1];
    else y1=buffer[position-1];

    buffer[position]=value;
//    if(lines[position])scene->removeItem(lines[position]);
//    lines[position]=scene->addLine(count+bufShift,h-y1*h/300,count+bufShift+1,h-buffer[position]*h/300);
    position++;
    if(position>=count) position=0;
//    bufShift++;

//    ui->graphicsView->setSceneRect(bufShift,0,count,h);
//    ui->graphicsView->horizontalScrollBar()->setSliderPosition(ui->graphicsView->horizontalScrollBar()->sliderPosition()+1);

}

void Dialog::repaintBuffer()
{    
    scene->clear();
    mycursor=0;

    int pos=position;
    int h=ui->graphicsView->viewport()->height();
    //int nexty=h-buffer[pos]*h/300;
    QPolygon polyg;
    for(int i=0;i<count;i++){
        polyg.append(QPoint(i,h-buffer[pos]*h/300));
        //int y1=nexty;
        if(++pos>=count) pos=0;
        //nexty=h-buffer[pos]*h/300;
        //lines[pos]=scene->addLine(i,y1,i+1,nexty);
    }
    scene->addPolygon(polyg,QPen(Qt::red),QBrush(Qt::transparent));
    addCursor();

    scene->addLine(50,10,51,90,QPen());
    scene->addLine(60,10,61,60,QPen());
    scene->addLine(70,10,71,30,QPen());
}

/////////////////////////////////////

void Dialog::addCursor()
{
    if(mycursor) scene->removeItem(mycursor);
    mycursor=scene->addLine(xPos,0,xPos,ui->graphicsView->viewport()->height(),QPen(Qt::gray));
}
