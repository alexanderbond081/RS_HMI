#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <qgraphicsscene.h>
#include <QTimer>
#include <QResizeEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    bool eventFilter(QObject *target, QEvent *event);
    ~Dialog();

private slots:
    void on_pushButton_clicked();

private:
    Ui::Dialog *ui;

    QGraphicsScene *scene;
    int actualScene=0;
    QTimer *paintTimer;
    int *buffer;
    QGraphicsLineItem **lines;
    int count=0;
    int position=0;
    int xPos=0;
    int bufShift=0;
    QGraphicsLineItem *mycursor=0;

    void addCursor();
    void addValue(int value);
    void repaintBuffer();
};

#endif // DIALOG_H
