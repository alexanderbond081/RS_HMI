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

#ifndef DIAGRAMLOG_H
#define DIAGRAMLOG_H

#include "viewelement.h"

class PointOfDiagram
{
    // для передачи точки в контейнер и обратно, не для хранения.
public:
    qint64 time_msec=0; // мсек от начала дня, т.е. в данном контексте от начала графика. хватило бы и простого int, т.к. это 5965 часов.
    QVector<int> lines;
    bool isValid(){if(lines.count()>0) return true; else return false;}

    // надо тут методы вообще в этом классе?
};

class FileOfDiagram: public QObject
{
    Q_OBJECT

private:
    friend class DiagramLog;

    // хвосты для системы поиска соответсвующих файлов графика в заданной директории
    // но я думаю что не буду реализовывать это пока
    // static QStringList pathCash; // список файлов в рабочем каталоге
    // static QDateTime pathCashTime; // последнее обновление списка файлов (для периодического перечитывания)
    // static QMap<QDate, QString> fileCheckCash; // сюда скидываем все проверенные файлы в директории path
    // чтобы второй раз не лазить. Битые файлы тоже сюда скидываем со значением =0(QDate)
    // ?стоит ли туда же скидывать проверенные значения даты с пустым именем?

    static int SAVE_BUFFER_SIZE;
    
    bool alloc_mem();
    bool add_mem();
    bool trim_mem();
    bool free_mem();

    QString filename; // имя файла включая путь
    QDate date;

    qint64 *times; // массив времени точек в милисекундах от начала суток
    int *values; // массивы значений сваленые в кучу
    int lines_count; // кол-во значений соотв. одной точке
    int interval_msec; // ожидаемый интервал между точками (для ужатия)
    int points_count; // кол-во точек в памяти
    int points_saved; // кол-во точек сохраненных в файл.
    int total_size; // кол-во точек, под которые выделена память.
    bool allocated; // переменная показывает выделена ли память под массив. Для очистки
    QTime lastsave_time;
    QTimer *saveTimer=nullptr; // создается/продляется в addPoint() в режиме singleshot.

    QMap<qint64, QString> notes; // пометки на графике. Могут использоваться для многих целей
    int notes_saved;
    const QByteArray ionV09Format=QByteArray("ion.v09");
    const QByteArray ionV10Format=QByteArray("ion.v10");

    int last_average_index;
    bool loadIonV09File(const QString &filename); // это то что называется legacy
    bool saveIonV09File(const QString &filename); // запись в старый формат, который наверное не нужен, но пусть будет...
    bool loadIonV10File(const QString &filename);
    bool loadFile(const QString &filename);
    bool saveFile(const QString &filename);

    bool checkFileHeader(const QString &filename){return 0;} // это делается в функции загрузки
    bool appendFile(const QString &filename){return 0;} // это делается в функции сохранения

    bool startTimer();
    // поиск файлов оставим до лучших времен
    // QString findFile(){return "";} // ищет файл с соотв. датой

    // meta data
    QList<int> _decimals;
    QList<int> _maxs;
    QList<QColor> _colors;
    QColor _backgroundColor;
    QString _description;
    QList<QString> _labels;

private slots:
    void onSaveFileTimer();

public:
    // static void setPath(const QString &newPath){} // установка общей директории для поиска и создания файлов

    explicit FileOfDiagram(QDate date, const QString &filename, int linesCount, int interval, int fileSize=1000);
    virtual ~FileOfDiagram();

    int linesCount();
    int pointsCount();
    int sizeInPoints();
    int isAllocated();

    PointOfDiagram getPoint(int num); // возвращаем значения в точке
    int findPointIndex(qint64 msec);
    int findPointNextTo(qint64 msec); // поиск номера точки, соответсвующей указанному времени. Если не найден, то возвращаем -1
    PointOfDiagram findPoint(qint64 msec); // поиск значения точки, соответсвующей указанному времени. Если не найден, то...
    PointOfDiagram findPoint(QDateTime time); // поиск значения точки, соответсвующей указанному времени. Если не найден, то...
    //int findPointIndex(QDateTime time);

    void startAverage(qint64 left);
    PointOfDiagram getAverage(qint64 left, qint64 right); // устанавливаем начало и интервал для построения серии усредненных значений
    PointOfDiagram nextAverage(qint64 right); // возвращаем среднее значение, соотв. следующему интервалу.
    QString findNote(qint64 time); // возвращает последнюю запись к указанному моменту времени
    QString findNote(QDateTime time); // возвращает последнюю запись к указанному моменту времени
    QMap<qint64, QString> &getNotes();

    bool addPoint(const PointOfDiagram &point); // если дата не совпадает, то посылать?
    bool addNote(qint64 time, QString note);

    void setMetaData(const QList<int> &decimals,
                     const QList<int> &maxs,
                     const QList<QColor> &colors,
                     const QColor backgroundColor,
                     const QString &description,
                     const QList<QString> &labels);
    QList<int> &getDecimals();
    QList<int> &getMaxs();
    QList<QColor> &getColors();
    QColor &getBackgroundColor();
    QString &getDescription();
    QList<QString> &getLabels();
};


class DiagramLog : public ViewElement
{
    Q_OBJECT

public:
    explicit DiagramLog(const QString &cap, const QString &name, const QString description, int interval,
                      const QList<PlcRegister> &lineRegisters, const QList<int> &lineDecimals, const QList<int> &lineMaxs,
                      const QList<QString> &lineLabels, const QList<QColor> lineColors, QColor backColor,
                      QWidget *parent=nullptr);
    virtual ~DiagramLog(); // требуется очистка памяти т.к. используются указатели

private slots:
    void onTimer();
    void moveToDate(QDate date); // для перемотки экрана

private:

    class Screen: public QWidget
    {
    public:
      explicit Screen(DiagramLog *parent);
      void paintEvent(QPaintEvent *);
    private:
      DiagramLog *parent;
    };

    void view(); // получение новых данных
    void adjustChildrenSize();

    bool eventFilter(QObject *target, QEvent *event);
    QTime last_repaint_time=QTime::currentTime();
    QTime last_update_time=QTime::currentTime();
    QDateTime last_screen_position;
    double screen_pos_correction=0;

    QString generateFilename(QDate date);
    FileOfDiagram* openFileOfDiagram(QDate fileDate);
    void screenBufferResize(int width);
    void updateCursor();
    void updateDiag(bool fast=false);

    QString path; // путь определяемый name
    QString name; // название, часть path и filename.
    //QScrollBar *scrollbar; // прокрутка скроллингом не актуальна, если не известны границы.

    Screen *screen; // пустой виджет для отрисовки графика
    QVector<int> screenBuffer; // все линии в одном массиве группами по linesCount точек
    const int EMPTY_POINT = -100500;
    const int INTERPOL_POINT = -100600;
    const qint64 MSEC_PER_DAY = 86400000;
    int CHARSIZE=11;

    QRect scrollLeftRect,scrollRightRect,rewindToTheEndRect,scaleInRect,ScaleOutRect,inputDateRect;

    //QVector<PointOfDiagram> screenBuffer; // или все же так?
    int screenWidthPix=0;

    int linesCount=0;
    int *values; // сюда будут складываться значения регистров в методе view()
    QHash<QDate, FileOfDiagram*> dataFiles;
    QDateTime screenPositionTime; // позиция отображения в конструкторе будет задаваться now - 24 часа
    int screenWidthSec; // проще будет сразу задать ширину экрана в секундах. По умолчанию 24*60*60

    int cursorPos=0;
    //QTime cursorTime;
    //QVector<int> cursorVal;
    PointOfDiagram cursorPoint;
    QString cursorNote;

    QTimer *timer; // а этот таймер будет раз в period складывать значения в buffer

    bool gotValues=false;
    //int labelsMaxCharCount=0; // удалить
    //int valuesMaxCharCount=0; // удалить
    QString labelMax; // чтобы не делать повторный поиск    
    QList<PlcRegister> registers;
    QList<int> dividers;
    QList<int> decimals;
    QList<int> maxs;
    QList<QColor> colors;
    QColor backgroundColor;
    QString description;
    QList<QString> labels;
};


#endif // DIAGRAMLOG_H
