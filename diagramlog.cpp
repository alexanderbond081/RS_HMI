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

#include "diagramlog.h"
#include "resources.h"
#include <QMouseEvent>
#include <QDateEdit>
#include <qdebug.h>
#include <QDesktopWidget>



// ---------------------------------
//  реализация класса FileOfDiagram
// ---------------------------------

int FileOfDiagram::SAVE_BUFFER_SIZE = 2048; // 1024 - 2048 - 4096

bool FileOfDiagram::alloc_mem()
{
    if(!allocated){
        times=new qint64[total_size];
        values=new int[total_size*lines_count];
        // как избежать вылета при неудачном выделении памяти??
        allocated=true;
    }
    return allocated;
}

bool FileOfDiagram::add_mem()
{
    // increase mem total_size by 20%
    if(!allocated){
        total_size+=total_size/5;
        alloc_mem();
    }
    else {
        int new_total_size=total_size+total_size/5;
        qint64* new_times=new qint64[new_total_size];
        int* new_values=new int[new_total_size*lines_count];
        // как избежать вылета при неудачном выделении памяти??

        memcpy(new_times, times, sizeof(*times)*total_size);
        memcpy(new_values, values, sizeof(*values)*total_size*lines_count);
        delete[] times;
        delete[] values;
        total_size=new_total_size;
        times=new_times;
        values=new_values;
        //qDebug()<<"mem added,  size new "<<total_size<<" points";
    }
    return allocated;
}

bool FileOfDiagram::trim_mem()
{
    // trim mem to points_count;
    if(allocated && ((total_size-points_count)*100/total_size>10)){ // you don't realy need to trim your beard every day...
        int new_total_size=points_count;
        qint64* new_times=new qint64[new_total_size];
        int* new_values=new int[new_total_size*lines_count];
        // как избежать вылета при неудачном выделении памяти??

        memcpy(new_times, times, sizeof(*times)*new_total_size);
        memcpy(new_values, values, sizeof(*values)*new_total_size*lines_count);
        delete[] times;
        delete[] values;
        total_size=new_total_size;
        times=new_times;
        values=new_values;
        //qDebug()<<"mem trimmed to "<<total_size<<" points";
    }
    return allocated;
}

bool FileOfDiagram::free_mem()
{
    if(allocated){
        delete[] times;
        delete[] values;
        times=nullptr;
        values=nullptr;
        allocated=false;
        points_count=0;
    }
    return allocated;
}

bool FileOfDiagram::loadIonV09File(const QString &filename)
{
    QFile file(filename);
    if(!file.exists()){
        qDebug()<<filename<<" not found";
        return false;
    }

    qint64 size = file.size();
    if(size<64){
        qDebug()<<filename<<" invalid";
        return false;
    }

    if(!file.open(QFile::ReadOnly)){
        qDebug()<<filename<<" can't open";
        return false;
    }
    QByteArray bdata = file.readAll();
    file.close();
    uchar *data=reinterpret_cast<uchar *>(bdata.data());

    QByteArray format=bdata.mid(0,7).trimmed();
    if(!ionV09Format.contains(format)){
        qDebug()<<filename<<" wrong format: "<<QString(format);
        return false;
    }

    // на всякий случай читаем название?
    QString name = QString::fromLatin1(bdata.mid(8,24));

    // тут ещё чекнуть дату чтобы совпадала.
    // дата записывается словами, где старший и младший байты содержат соотв. [год,месяц],[день,час],[мин,сек].
    // при переводе в массив байт последовательность получается (месяц,год,час,день,сек,мин).
    QDate date = QDate(data[33]+2000,data[32],data[35]);
    //QTime time = QTime(data[34],data[37],data[36]);
    qint64 time_msec=((data[34]*60+data[37])*60+data[36])*1000;
    if(date!=this->date){
        qDebug()<<filename<<" wrong date: "<<date;
        return false;
    }

    // дальше непосредственно загрузка. (линии могут не совпадать и это не повод их не грузить)

    // кол-во линий
    // !!! кол-во линий файла может отличаться от кол-ва линий заданного настройками, т.к. настройки могут поменяться.
    // и т.к. всё должно загрузиться и отображаться адекватно, кол-во линий файла установить то, которое больше,
    // а пустые линии заполнить нулями.
    int dataLinesCount;
    dataLinesCount = data[38]; // +(data[39)<<8) не имеет смысла, т.к. я бы вообще ограничился 32 линиями в принципе
    if((dataLinesCount<1)||(dataLinesCount>32))
    {
        qDebug()<<filename<<" wrong lines count: "<<dataLinesCount;
        return false;
    }
    else if(dataLinesCount>lines_count) lines_count=dataLinesCount;

    interval_msec=1000; // я вдруг вспомнил что это всё вообще не имеет смысла, т.к. панель не умеет в мсек.

    //data[40)|(data[41)<<8)|(data[42)<<16)|(data[43)<<24); // 2 байта 32000 мсек это мало. Поэтому 4 байта 2000000000 мсек, 2000000 сек, 33333 мин, 555 час, 23 дней.
    //if((data[40)==2)&&(data[41)==0)&&(data[42)==2)&&(data[43)==0)) interval_msec=1000; // на всякий случай, чтобы можно было загрузить файлы сделанные старыми скриптами панели оператора.
    //if(interval_msec<10) return 0; //interval_msec=1000; // return 0; // хз блин. В ionViewLazarus точно надо interval=1000;

    qint64 index=64;
    bool brokenSequence=false;

    total_size=24*60*60*1000/interval_msec; // зарезервировать память сразу под актуальное для данного файла кол-во точек будет разумным
    //qDebug()<<"makening allocation for total_size of "<<total_size;
    if(!allocated) alloc_mem();
    int *current_point = new int[dataLinesCount];

    qDebug()<<"file "<<filename<<" size "<<size; // debug

    int com_0b=0,com_2b=0,com_4b=0,com_8b=0,uncom=0; //debug
    while(index<size)
    {
        // читаем 1й байт и смотрим:
        // 255 - повторение точки без изменений (data[index+1] раз)
        // 2 старших бита уст. - 2 бита на одну точку
        // 1 старший бит - 4 бита точка
        // 2й бит уст. - 1 байт точка.
        // 2 старших бита нули - старт цепочки точек.
        // (добавлено милион лет спустя) 0 - пропускаем и переходим к следующему


        // что можно сделать для оценки способа ужатия следующей точки?
        // - прочитать префикс точки, из него определить способ ужатия
        // - из кол-ва точек и способа ужатия определить кол-во байт на эту точку
        // - в случае если размер файла меньше требуемого на точку - закончить чтение.
        // - если префикс=0, то пропускаем байт.
        // - что делать если вдруг что-то сбилось? как понять с какой точки дальше читать?


        if(brokenSequence && ((data[index]&0xC0)!=0x00)) index+=2; // что-то надо придумать с этой brokenSequence
        else
        //if(data[index] == 0x00) index++; // если вместо начала следующей точки мы встречаем "ноль", то переходим к следующему байту (так не надо будет париться за четность кол-ва байт на одну точку)
        //else
        if(data[index] == 0xFF) // чтение точкек без изменений (ужатых до "нуля")
        {
            if((size-index)<2)
            {
                qDebug()<<"eof";
                break;
            }
            index++;
            for(int i=0;i<data[index];i++)
            {
                if(points_count>=total_size)
                {
                    qDebug()<<"points_count("<<points_count<<")>=total_size ("<<total_size<<")"<<"   add some more";
                    add_mem();
                }

                time_msec+=interval_msec;
                times[points_count]=time_msec;
                for(int line=0;line<dataLinesCount;line++)
                    values[points_count*lines_count+line]=current_point[line];
                for(int line=dataLinesCount;line<lines_count;line++)
                    values[points_count*lines_count+line]=0;
                points_count++;
            }
            index++;

            com_0b++; //debug
        }
        else
        {
        // остальные точки обрабатываются похожим образом и большой кусок вынесен за скобки

            if((data[index] & 0xC0) == 0xC0) // ужатие до 2 бит
             {
              if((size-index)<(dataLinesCount/4+1)) break;
              time_msec += (data[index] & 0x3F)*interval_msec; // xor
              index++;
              int c=0;
              for(int line=0;line<dataLinesCount;line++)
               {
                index+=c/8;
                c=c&7; // или c=c%8, без разницы
                current_point[line] += ((data[index]>>c) & 0x03) - 2;
                c+=2;
               }
              index++; // текущий байт полюбому использовался, если только линий не 0
              // currentPoint копируется в points по тексту ниже.

              com_2b++; // debug
             }
            else
            if((data[index] & 0xC0) == 0x80) // ужатие до 4 бит
            {
                if((size-index)<(dataLinesCount/2+1)) break;
                time_msec += (data[index] & 0x3F)*interval_msec; // xor
                index++;
                int line=0;
                while(line < dataLinesCount)
                {
                    current_point[line] += (data[index] & 0x0F) - 8;
                    line++;
                    if(line < dataLinesCount)
                    {   // карявенько, но что делать? выход за рамки кол-ва линий нельзя.
                        current_point[line] += ((data[index]>>4) & 0x0F) - 8;
                        line++;
                    }
                    index++;
                }
                // currentPoint копируется в points по тексту ниже.

                com_4b++; // debug
            }
            else
            if((data[index] & 0xC0) == 0x40) // ужаие до 1 байта
            {
                if((size-index)<(dataLinesCount+1)) break;
                int dtmsec = int(data[index] & 0x3F)*interval_msec;
                time_msec += dtmsec;
                index++;
                for(int line=0;line<dataLinesCount;line++)
                {
                    current_point[line] += data[index] - 128; //qint16?
                    index++;
                }
                // currentPoint копируется в points по тексту ниже.

                com_8b++; // debug
            }
            else // if (data[index) and 0xC0) = 0x00 then // неужатая точка. добавить else обработка ошибки?
            {
                if((size-index)<(dataLinesCount*2+6))
                {
                    qDebug()<<"eof";
                    break;
                }
                QDate p_date = QDate(data[index+1]+2000,data[index]&0x3F,data[index+3]);
                time_msec=((data[index+2]*60+data[index+5])*60+data[index+4])*1000;
                if(p_date!=this->date)
                {
                    qDebug()<<"wrong date "<<p_date<<"(has to be "<<this->date<<")";
                    break; // это в целом затычка для этого формата, т.к. у нас тут не больше суток в файле хранится.
                }

                /* вставляю тут эту затычку. Чтобы не загружались в массив точки непоследовательные
                    и не смущали скроллинг ошибками нечеловеческими */
                if(points_count>0)
                    brokenSequence=(time_msec < times[points_count-1]);
                else brokenSequence=false;
                index+=6;

                for(int line=0;line<dataLinesCount;line++)
                {
                    // тут надо сделать чтение байтов соответственно размерности точки каждой линии
                    // скрипт надо исправить. там в последней точке 0 указан.
                    // или забить?
                    current_point[line] = qint16(data[index]|(data[index+1] << 8));
                    index+=2;
                }
                // currentPoint копируется в points по тексту ниже.

                uncom++; // debug
            }

            // эта часть одинаковая для всех типов ужатия кроме нулевой точки.
            index+=index%2; // доводим n до чётного кол-ва, т.к. панель не умеет подругому.

            if(!brokenSequence)
            {
                if(points_count>=total_size)
                {
                    qDebug()<<"points_count>=total_size "<<points_count<<total_size<<"   adding some mem";
                    add_mem();
                }
                times[points_count]=time_msec;
                for(int line=0;line<dataLinesCount;line++)
                    values[points_count*lines_count+line]=current_point[line];
                for(int line=dataLinesCount;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                    values[points_count*lines_count+line]=0;
                points_count++;
            }
            else qDebug()<<"broken sequence at point "<<points_count;
        }
    }
    qDebug()<<"finish loading points. index"<<index<<"  size"<<size<<"  points loaded"<<points_count;
    qDebug()<<"uncompressed points count "<<uncom<<"  8bit "<<com_8b<<"  4bit "<<com_4b<<"  2bit "<<com_2b<<"  0bit"<<com_0b;

    // if(lines_count==load_lines_count)  points_saved = points_count; else points_saved=0;
    // что делать с переменной points_saved?
    // она должна служить индикатором ИСКЛЮЧЕТИТЕЛЬНО для механизма дополнения файла новыми точками (без полной перезаписи)
    // отсюда вопрос, ну например мы таки загрузили этот фаел, не каждые же 5 минут перестартует программа,
    // ну перезапишется этот долбаный файл 1 раз за перезапуск. Никто от этого не умрет.
    // это ПРОЩЕ, чем определять возможность дозаписи(совпадение формата, кол-ва линий и прочих настроек) и ловить баги.


    trim_mem(); // т.к. это legacy функция, то не предполагаем какое-либо дополнение. просто подрезаем не используемую память.
    delete[] current_point;    
    return true;
}

bool FileOfDiagram::saveIonV09File(const QString &filename)
{
    // эта функция должна перезаписывать файл полностью

    // 1. открыть файл в режиме перезаписи
    QFile file;
    file.setFileName(filename);
    if(!file.open(QIODevice::WriteOnly)){
        qDebug()<<"Can't open diagram file for saving";
        return false;
    }

    // 2. сохранить шапку.
    QTime filetime;
    filetime.setHMS(0,0,0); // в общем виде(без привязки к именно данному объекту) оно тоже должно быть указано.
    char year=char(date.year()-2000),
            month=char(date.month()),
            day=char(date.day()),
            hour=char(filetime.hour()), // т.к. данный объект предполагает создание файлов с начала суток.
            minute=char(filetime.minute()),
            second=char(filetime.second());
    QByteArray emptyness;
    QByteArray fileHeader; // заголовок 64 байта
    fileHeader.append(ionV09Format.constData(), 8);
    fileHeader.append("RS_HMI saved file.",18);
    fileHeader.append(emptyness.fill(0,24-18));
    fileHeader.append(month);
    fileHeader.append(year);
    fileHeader.append(hour);
    fileHeader.append(day);
    fileHeader.append(second);
    fileHeader.append(minute);
    fileHeader.append(char(lines_count));
    fileHeader.append(char(0));
    // пока без указания интервала, по умолчанию считаем 1 секунда.
    fileHeader.append(emptyness.fill(0,24));

    file.write(fileHeader.data(),64);
    // file.write(fileHeader.data(),fileHeader.count());

    // 3.
    QByteArray fileData;
    bool chain_started=false;
    int zero_points_count=0;
    qint64 time_last=0; // должно быть равно hour minute second в шапке.
    QVector<int> values_current;
    QVector<int> values_last;
    QVector<int> dValues;
    values_current.fill(0,lines_count);
    values_last.fill(0,lines_count);
    dValues.fill(0,lines_count);
    int dMax;

    for(int index=0;index<points_count;index++)
    {
        qint64 dsec=qRound64(double(times[index]-time_last)/1000);
        dMax=0;
        for(int line=0;line<lines_count;line++)
        {
            int val=values[index*lines_count+line];
            // График может содержать числа и больше 32767, но этот формат сохранения не поддерживает.
            // Отрицательные значения для текущих целей не актуальны. Этот формат - кАстыль, временно.
            if(val<0)val=0;
            if(val>INT16_MAX)val=INT16_MAX;
            values_current.replace(line, val);
            dValues.replace(line, values_current.at(line) - values_last.at(line));
            dMax=dMax | qAbs(dValues.at(line));
        }

        if(dsec>0) // если точка ближе чем на 1 сек, то просто берем следующую
        {
            bool zero_point_added=false; // нужно для очистки zero_points_count. Возможно если поменять структуру процесса кодировки, эта переменная не нужна будет.
            time_last=times[index];
            for(int line=0;line<lines_count;line++) values_last.replace(line, values_current.at(line));
            if(!chain_started || dsec>60 || dMax>127)
            {
                // не ужатая точка
                chain_started=true;
                QTime pointtime=filetime.addMSecs(time_last);
                fileData.append(month & 0x3F);
                fileData.append(year);
                fileData.append(char(pointtime.hour()));
                fileData.append(day);
                fileData.append(char(pointtime.second()));
                fileData.append(char(pointtime.minute()));

                for(int line=0;line<lines_count;line++)
                {
                    fileData.append(char(values_last.at(line) & 0xFF));
                    fileData.append(char((values_last.at(line)>>8) & 0xFF));
                }
            }
            else if(dsec==1 && dMax==0)
            {
                // ужимаем до нуля
                if((zero_points_count>0)&&(zero_points_count<250))
                {
                    int ilast=fileData.count()-1;
                    fileData[ilast] = fileData.at(ilast)+1;
                    zero_points_count++;
                }
                else
                {
                    zero_points_count = 1;
                    fileData.append(char(0xFF));
                    fileData.append(1);
                }
                zero_point_added = true;
            }
            else if(dMax<2)
            {
                // ужимаем до 2х бит
                fileData.append(char(dsec|0xC0));
                char byte=0;
                int bitn=0,byten=1;
                for(int line=0;line<lines_count;line++)
                {
                    byte=byte|char(((dValues.at(line)+2)&0xFF)<<bitn);
                    bitn+=2;
                    if(bitn>=8)
                    {
                        fileData.append(byte);
                        byte=0;
                        bitn=0;
                        byten++;
                    }
                }
                if(bitn)
                {
                    fileData.append(byte);
                    byten++;
                }
                if(byten%2)fileData.append(char(0));
            }
            else if(dMax<8)
            {
                // ужимаем до 4х бит
                fileData.append(char(dsec|0x80));
                char byte=0;
                int bitn=0,byten=1;
                for(int line=0;line<lines_count;line++)
                {
                    byte=byte|char(((dValues.at(line)+8)&0xFF)<<bitn);
                    bitn+=4;
                    if(bitn>=8)
                    {
                        fileData.append(byte);
                        byte=0;
                        bitn=0;
                        byten++;
                    }
                }
                if(bitn)
                {
                    fileData.append(byte);
                    byten++;
                }
                if(byten%2)fileData.append(char(0));

            }
            else
            {
                // ужимаем до 1 байт
                fileData.append(char(dsec|0x40));
                int byten=1;
                for(int line=0;line<lines_count;line++)
                {
                    fileData.append(char((dValues.at(line)+128)&0xFF));
                    byten++;
                }
                // панель got1000 умеет сохранять только слова, поэтому для преемственности сделал
                // выравнивание до четного кол-ва байт. Но на самом деле это не нужно,
                // т.к. ноль можно интерпретировать просто как пропуск и переходить к следующему байту.
                if(byten%2)fileData.append(char(0));
            }
            if(!zero_point_added) zero_points_count=0;
        }
    }

    file.write(fileData);
    file.close();

    points_saved=points_count;\

    return true;
}

bool FileOfDiagram::loadFile(const QString &filename)
{
    // проверить файл
    QFile file(filename);
    if(!file.exists()){
        qDebug()<<filename<<" not found";
        return false;
    }
    if(!file.open(QFile::ReadOnly)){
        qDebug()<<"can't open "<<filename;
        return false;
    }

    // проверить шапку
    char temp[8];
    file.read(temp, 8);

    // выбрать функцию загрузки
    if(ionV10Format.contains(temp)){
        return loadIonV10File(filename);
    }
    else if(ionV09Format.contains(temp)){
        return loadIonV09File(filename);
    }
    else {
        qDebug()<<filename<<" unknown format: "<<QString(temp);
        return false;
    }
}

bool FileOfDiagram::loadIonV10File(const QString &filename)
{
    // 1. проверка файла
    QFile file(filename);
    if(!file.exists()){
        qDebug()<<filename<<" not found";
        return false;
    }

    if(file.size()<41){ // (24+lines_count*10+7) при кол-ве линий = 1 и отсутствии подписей размер шапки = 24+10+7 = 41. На самом деле там еще хоть одна точка должна быть - (6+lines_count*4) 41+10=51
        qDebug()<<filename<<" is invalid (size)";
        return false;
    }

    if(!file.open(QFile::ReadOnly)){
        qDebug()<<"can't open "<<filename;
        return false;
    }

    //qDebug()<<"file "<<filename<<" size "<<file.size(); // debug

    // 2.0 загрузка шапки. Для простоты реализации шапку будем читать из потока.
    QDataStream dataStream(&file);

    char temp[8];
    dataStream.readRawData(temp, 8);
    if(!ionV10Format.contains(temp)){
        qDebug()<<filename<<" wrong format: "<<QString(temp);
        return false;
    }

    qint16 year;
    qint8 month, day;
    qint32 msec_since_0_AM_of_date;
    dataStream>>year;
    dataStream>>month;
    dataStream>>day;
    dataStream>>msec_since_0_AM_of_date;

    // тут ещё чекнуть дату чтобы совпадала? для чего? здесь ли это надо делать?
    // такс. пока у нас эти все методы являются частью диаграммы организованной вот таким образом,
    // разбиение файлов по суткам и без системы поиска "правильного" файла, имеет смысл проверять дату.
    // потом будет смысл переместить проверку шапки в отдельный метод и эту защиту убрать.
    QDate date = QDate(year, month, day);
    qint64 time_msec=msec_since_0_AM_of_date;
    if(date!=this->date){
        qDebug()<<filename<<" wrong date: "<<date;
        //qDebug()<<"load file year: "<<year<<"  month: "<<month<<"  day: "<<day<<"  msec: "<<msec_since_0_AM_of_date;

        return false;
    }

    // кол-во линий
    // !!! кол-во линий файла может отличаться от кол-ва линий заданного настройками, т.к. настройки могут поменяться.
    // и т.к. всё должно загрузиться и отображаться адекватно, кол-во линий файла установить то, которое больше,
    // а пустые линии заполнить нулями.
    int load_lines_count;
    dataStream>>load_lines_count;//lines_count;
    qDebug()<<"lines count = "<<lines_count<<"    lines in flie = "<<load_lines_count;
    if(load_lines_count>lines_count) lines_count=load_lines_count;
    qDebug()<<"change to lines count = "<<lines_count<<"    lines in flie = "<<load_lines_count;

    // типовой интервал мсек. Он может отличаться в файле и в настройках графика, поэтому чуть сложнее чем просто...
    int file_interval_msec;
    dataStream>>file_interval_msec;
    if(file_interval_msec<interval_msec) interval_msec=file_interval_msec;

    /// 8 байт формат, 8 байт дата-время, 8 байт кол-во линий+интервал
    /// итого 24 байта фиксированной части шапки.

    // 2.1 "резиновая" шапка

    // >десятич.разряды
    _decimals.clear();
    for(int i=0;i<load_lines_count;i++){
        qint8 dec;
        dataStream>>dec;
        _decimals.append(dec);
    }
    // >масштаб
    _maxs.clear();
    for(int i=0;i<load_lines_count;i++){
        qint32 max;
        dataStream>>max;
        _maxs.append(max);
    }
    // >цвет
    _colors.clear();
    for(int i=0;i<load_lines_count;i++){
        QRgb rgb;
        dataStream>>rgb;
        _colors.append(QColor::fromRgb(rgb));
    }
    // цвет фона
    QRgb back_rgb;
    dataStream>>back_rgb;
    _backgroundColor=QColor::fromRgb(back_rgb);

    // дескрипшн
    // как и  подписи, цвета, масштаб и т.п. относится к мета-данным. Я не уверен, что следует что-то в них менять прямо сразу... (это имеется в виду что в xml могут поменяться эти данные)
    // вопервых - это лишняя сложность в функции загрузки
    // вовторых - функция загрузки не место для принятия таких решений.
    // Есть метод setMetaData, хозяин (DiagramLog) его может вызвать и после инициализации и соответственно загрузки файла.

    QByteArray temp_str;
    qint8 temp_char;
    dataStream>>temp_char;
    int n=0;
    while(temp_char){ // тут нужна защита от переполнения!!!
        temp_str.append(temp_char);
        dataStream>>temp_char;
        n++;
    }
    _description=QString::fromUtf8(temp_str);
    qDebug()<<"loaddiag description "<<_description;

    // подписи
    _labels.clear();
    for(int i=0;i<load_lines_count;i++){
        temp_str.clear();
        dataStream>>temp_char;
        while(temp_char){ // тут нужна защита от переполнения!!!
            temp_str.append(temp_char);
            dataStream>>temp_char;
        }
        _labels.append(QString::fromUtf8(temp_str));
    }
    //qDebug()<<"loaddiag labels "<<_labels;

    /// далее идут еще более резиновые пометки на графике. Это уже нешапка, а таки прямо конкретная информация

    // 3. текстовые пометки на графике
    notes.clear();
    qint16 notes_count;
    dataStream>>notes_count;
    for(int i=0;i<notes_count;i++){
        qint32 temp_time;
        dataStream>>temp_time;
        temp_str.clear();
        dataStream>>temp_char;
        while(temp_char){ // тут нужна защита от переполнения
            temp_str.append(temp_char);
            dataStream>>temp_char;
        }
        notes.insert(temp_time, QString::fromUtf8(temp_str));
    }
    //qDebug()<<"loaddiag notes count "<<notes_count;

    // 4. диаграмма

    // не могу придумать как подружить dataStream с раскодировкой, поэтому дальше будем юзать тупо массив
    QByteArray bdata = file.readAll();
    quint8 *data=reinterpret_cast<uchar *>(bdata.data());
    file.close();


    //qDebug()<<"load first data bytes - "<<data[0]<<" "<<data[1]<<" "<<data[2]<<" "<<data[3];
    qint64 index=0;
    bool searchInitial=true;

    //bool debug_first_point=true;

    total_size=24*60*60*1000/interval_msec; // зарезервировать память сразу под актуальное для данного файла кол-во точек будет разумным
    //qDebug()<<"makening allocation for total_size of "<<total_size;
    if(!allocated) alloc_mem();
    int *current_point = new int[load_lines_count];

    int size=bdata.count();
    //qDebug()<<"load data bytes count "<<size;

    int com_0b=0,com_2b=0,com_4b=0,com_8b=0,uncom=0; //debug
    while(index<size){
        // проверяем размер выделенное место в памяти
        if(points_count>=total_size){
            qDebug()<<"points_count "<<points_count<<" >= "<<" total_size "<<total_size<<"   increase total_size and allocate more";
            add_mem();
        }

        // 4.1 ищем точку с которой начинается цепочка значений. начинается с байта = 0x00
        if(searchInitial){
            if(data[index]==0){
                // проверяем, действительно ли эта точка является началом последовательности:
                // a. кол-во оставшихся в буфере точек
                int initialSize = 6+4*load_lines_count;
                if(size-index >= initialSize){
                    // b. идентификатор
                    if((data[index+1]&0xF0)==0x20){
                        // c. проверочные биты
                        int bitCheck = ((data[index+2]>>4)^data[index+2]) &
                                ((data[index+3]>>3)^data[index+3]) &
                                ((data[index+4]>>2)^data[index+4]) &
                                ((data[index+5]>>1)^data[index+5]) & 0x01;
                        if(bitCheck==1){
                            // d. если все проверки прошли, то просто обнуляем searchInitial, позволяя загрузке идти своим чередом.
                            searchInitial=false;
                        }
                    }
                } else break; // ?? по идее это выход из цикла, т.к. данных на стартовую точку уже не хватает.
            }
            if(searchInitial) index++; // если не нашли, то переходим к следущему байту
        }
        else {
            // 4.2 если начало последовательности найдено, то пытаемся определить способ ужатия следующей точки:
            // читаем 1й байт и смотрим:
            // 0xFF - повторение точки без изменений (data[index+1] раз)
            // 0xCx - 2 бита на одну точку
            // 0x8x - 4 бита точка
            // 0x4x - 1 байт точка.
            // 0x00 + 0x2x в следующем байте - старт цепочки точек.
            // (добавлено милион лет спустя)
            // 0x00, но не старт цепочки - если между точками, то пропускаем и переходим к следующему, иначе это ошибка и надо искать начало.

            enum CompressType {initial, delta16bit, delta8bit, delta4bit,
                               delta2bit, replicateStart, replicateNext, none}; // не все сейчас будут использованы
            CompressType compress=none;
            int point_size=0;
            if(data[index]==0xFF){
                compress=replicateStart;
                point_size=2;
            }
            else if((data[index]&0xF0)==0xC0){
                compress=delta2bit;
                point_size=1+int(ceil(double(load_lines_count)/4)); // округление в бОльшую сторону
            }
            else if((data[index]&0xF0)==0x80){
                compress=delta4bit;
                point_size=1+int(ceil(double(load_lines_count)/2)); // округление в бОльшую сторону
            }
            else if((data[index]&0xF0)==0x40){
                compress=delta8bit;
                point_size=1+load_lines_count;
            }
            else if(data[index]==0x00){
                if((size-index)>1){
                    if((data[index+1]&0xF0)==0x20){
                        compress=initial;
                        point_size=6+load_lines_count*4;
                    }
                    else{
                        // по всей видимости этот "ноль" был просто "ноль", а не признак стартовой точки. Его мы пропускаем.
                        index++;
                    }
                }
                else {
                    // останавливаем, т.к. это конец файла и ничего полезного уже не найдется.
                    break;
                }
            }
            else {
                // это явный сбой и надо искать исправную стартовую точку
                index++;
                searchInitial=true;
            }

            // 4.3 оцениваем следующую точку:
            // - в случае если размер файла меньше требуемого на точку - закончить чтение.
            // - если один из байтов в точке == 0, то считаем это сбоем. Переводим index к этому байту и ищем начальную точку.
            // ? какие еще можно сделать оценки?

            if(size-index<point_size){
                // останавливаем, т.к. это конец файла и ничего полезного уже не найдется.
                break;
            }

            qint64 zeroIndex=0;
            for(qint64 i=index+1;i<index+point_size;i++)
                if(data[i]==0){
                    zeroIndex=i;
                    break;
                }
            if(zeroIndex>0){
                // один из байтов, кодирующих точку == 0, а этого быть не должно. Переводим на него index и ищем начальную точку.
                index=zeroIndex;
                searchInitial=true;
            }
            else {
                // 4.4 все проверки сделаны, осталось прочитать точку.
                int bitCheck;
                qint64 time_temp;
                int value_temp;

                switch(compress){
                case initial:
                    // проверка на возможные косяки.
                    // эта проверка может выполняться не только после поиска начальной точки, где она так же выполняется.
                    bitCheck = ((data[index+2]>>4)^data[index+2]) &
                            ((data[index+3]>>3)^data[index+3]) &
                            ((data[index+4]>>2)^data[index+4]) &
                            ((data[index+5]>>1)^data[index+5]) & 0x01;
                    if(bitCheck!=1){
                        // ошибка в данных. переходим к поиску начальной точки.
                        qDebug()<<"error initial at point "<<points_count;
                        searchInitial=true;
                        index+=point_size;
                        break; // ?? по идее это выход из switch() case initial:
                    }
                    else {
                        // читаем время
                        time_temp=0;
                        time_temp|=(((data[index+1]&0x0F) << 24));
                        time_temp|=(((data[index+2]&0xFE) << 20));
                        time_temp|=(((data[index+3]&0xFE) << 13));
                        time_temp|=(((data[index+4]&0xFE) << 6));
                        time_temp|=(((data[index+5]&0xFE) >> 1));

                        /* вставляю тут эту затычку. Чтобы не загружались в массив точки непоследовательные
                            и не смущали скроллинг ошибками нечеловеческими */
                        if((points_count>0) && (time_temp < times[points_count-1])){ // ??? time_msec же еще не определено!! меняю на time_temp. Возможно это была ошибка.
                            qDebug()<<"broken sequence at point "<<points_count;
                            searchInitial=true;
                            index+=point_size;
                            break; // ?? по идее это выход из switch() case initial:
                        }
                        else time_msec = time_temp;
                        index+=6;
                        // читаем значения
                        for(int line=0;line<load_lines_count;line++){
                            bitCheck = ((data[index]>>4)^data[index]) &
                                    ((data[index+1]>>3)^data[index+1]) &
                                    ((data[index+2]>>2)^data[index+2]) &
                                    ((data[index+3]>>1)^data[index+3]) & 0x01;
                            if(bitCheck!=1){
                                qDebug()<<"data error. searching initial point";
                                searchInitial=true;
                                index+=point_size-6-4-line*4;
                                break; // ?? по идее это выход из switch() case initial:
                            }
                            else {
                                value_temp=0;
                                value_temp|=(((data[index  ]&0xFE) << 20));
                                value_temp|=(((data[index+1]&0xFE) << 13));
                                value_temp|=(((data[index+2]&0xFE) << 6));
                                value_temp|=(((data[index+3]&0xFE) >> 1));

                                current_point[line] = value_temp-134217728; // не знаю зачем я вместо преобразования 32 бит числа в 28 бит и обратно использовал ТАКОЙ метод, наверное потому что это проще?
                                index+=4;
                            }
                        }
                    }

                    times[points_count]=time_msec;
                    for(int line=0;line<load_lines_count;line++)
                        values[points_count*lines_count+line]=current_point[line];
                    for(int line=load_lines_count;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                        values[points_count*lines_count+line]=0;
                    points_count++;

                    //if(debug_first_point){
                        //qDebug()<<"first point time:"<<(time_msec/1000/60/60)<<"   value[0]="<<current_point[0]<<"   index:"<<index;
                    //    debug_first_point=false;
                    //}
                    uncom++; //debug

                    break; // case initial:

                case replicateStart:
                    // (надо ли тут делать доп.проверку? думаю нет, т.к. ноль проверяется еще до этого
                    // qDebug()<<"replicate "<<data[index+1]<<" points";
                    for(int i=0;i<data[index+1];i++){
                        time_msec+=file_interval_msec;
                        times[points_count]=time_msec;
                        for(int line=0;line<load_lines_count;line++)
                            values[points_count*lines_count+line]=current_point[line];
                        for(int line=load_lines_count;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                            values[points_count*lines_count+line]=0;
                        points_count++;
                    }
                    index+=point_size;

                    com_0b++; //debug
                    break;

                case delta2bit:{
                    int dtm=data[index]&0x0F; // тут бы еще проверку этого самого dmt, должен быть от 1 до 250
                    time_msec+=file_interval_msec*dtm;

                    int i=0;
                    int bitn=8;
                    int val=data[index+i];
                    for(int line=0;line<load_lines_count;line++){
                        if(bitn>=8){
                            i++;
                            bitn=0;
                            val=data[index+i];
                        }
                        else val>>=2;

                        //int shft=val&0x03-2;
                        //qDebug()<<"delta 2bit = "<<shft;
                        current_point[line]+=(val&0x03)-2;
                        bitn+=2;
                    }}
                    times[points_count]=time_msec;
                    for(int line=0;line<load_lines_count;line++)
                        values[points_count*lines_count+line]=current_point[line];
                    for(int line=load_lines_count;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                        values[points_count*lines_count+line]=0;
                    points_count++;
                    index+=point_size;

                    com_2b++; //debug
                    break;

                case delta4bit:{
                    int dtm=data[index]&0x0F; // тут бы еще проверку этого самого dmt, должен быть от 1 до 250
                    time_msec+=file_interval_msec*dtm;

                    int i=0;
                    int bitn=8;
                    int val=data[index+i];
                    for(int line=0;line<load_lines_count;line++){
                        if(bitn>=8){
                            i++;
                            bitn=0;
                            val=data[index+i];
                        }
                        else val>>=4;

                        //qDebug()<<"delta 4bit = "<<((val&0x0F)-8)<<" - "<<(val&0x0F);
                        current_point[line]+=(val&0x0F)-8;
                        bitn+=4;
                    }}
                    times[points_count]=time_msec;
                    for(int line=0;line<load_lines_count;line++)
                        values[points_count*lines_count+line]=current_point[line];
                    for(int line=load_lines_count;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                        values[points_count*lines_count+line]=0;
                    points_count++;
                    index+=point_size;

                    com_4b++; //debug
                    break;

                case delta8bit:{
                    int dtm=data[index]&0x0F; // тут бы еще проверку этого самого dmt, должен быть от 1 до 250
                    time_msec+=file_interval_msec*dtm;

                    for(int line=0;line<load_lines_count;line++)
                        current_point[line]+=data[index+line+1]-128;
                    }
                    times[points_count]=time_msec;
                    for(int line=0;line<load_lines_count;line++)
                        values[points_count*lines_count+line]=current_point[line];
                    for(int line=load_lines_count;line<lines_count;line++) // если в настройках указано больше строк, чем есть в загружаемом файле
                        values[points_count*lines_count+line]=0;
                    points_count++;
                    index+=point_size;

                    com_8b++; //debug
                    break;
                }
            }
        }
    }

    qDebug()<<"finish loading points. index"<<index<<"  size"<<size<<"  points loaded"<<points_count;
    qDebug()<<"initial "<<uncom<<"   8 bit "<<com_8b<<"   4 bit"<<com_4b<<"   2 bit"<<com_2b<<"   replicate "<<com_0b;

    // if(lines_count==load_lines_count)  points_saved = points_count; else points_saved=0;
    // что делать с переменной points_saved?
    // она должна служить индикатором ИСКЛЮЧЕТИТЕЛЬНО для механизма дополнения файла новыми точками (без полной перезаписи)
    // отсюда вопрос, ну например мы таки загрузили этот фаел, не каждые же 5 минут перестартует программа,
    // ну перезапишется этот долбаный файл 1 раз за перезапуск. Никто от этого не умрет.
    // это ПРОЩЕ, чем определять возможность дозаписи(совпадение формата, кол-ва линий и прочих настроек) и ловить баги.

    if(date!=QDate::currentDate()) trim_mem(); // считаем что такой файл не будет дополняться и поэтому обрезаем лишнюю память.
    delete[] current_point;
    return true;
}

bool FileOfDiagram::saveFile(const QString &filename)
{
    // 0. Проверяем, записывался ли файл (points_saved>0). Если да, то проверяем его целостность (хз как).
    // если всё ок (и notes не обновлялись) открываем в режиме append и пропускаем шапку.
    // иначе сбрасываем points_saved=0 и остальное делаем как обычно.

    QFile file;
    file.setFileName(filename);
    QDataStream fileStream(&file);
    bool save_header=false;
    if(points_saved==0 || notes_saved!=notes.count()) save_header=true; // если график не сохранялся или добавились пометки на графике
    if(!save_header){
        // проверяем файл на открываемость
        if(!file.open(QIODevice::Append)){
            qDebug()<<"Can't open diagram file for append";
            save_header=true;
        }
        else if(file.size()<(24+lines_count*10+7)){ // минимальный рамер шапки при известном кол-ве линий и пустыми подписями и пометками
            qDebug()<<filename<<"Diagram file is too small to append. Overwrite.";
            save_header=true;
        }
        else if(true){} // какие еще можно сделать проверки?
        qDebug()<<"append diagram file";
    }
    if(save_header){
        qDebug()<<"write/overwrite diagram file";

        // 1. открыть файл в режиме перезаписи
        if(!file.open(QIODevice::WriteOnly)){
            qDebug()<<"Can't open diagram file for saving"; //"Ошибка открытия файла для сохранения графика";
            return false;
        }

        // 2. сохранить шапку.
        // QDataStream сохраняет char как 4 байта, поэтому придется использовать signed char или qint8.
        short year=short(date.year());
        qint8 month=qint8(date.month());
        qint8 day=qint8(date.day());
        qint32 msec_since_0_AM_of_date=0; // в данном контексте всегда равно нулю, т.к. данный график 1 файл - 1 сутки (было бы актуально, если много файлов на 1 сутки)
        //qDebug()<<"save file year: "<<year<<"  month: "<<month<<"  day: "<<day<<"  msec: "<<msec_since_0_AM_of_date;

        fileStream.writeRawData(ionV10Format.data(),ionV10Format.count());
        fileStream<<qint8(0);
        fileStream<<year;
        fileStream<<month;
        fileStream<<day;
        fileStream<<msec_since_0_AM_of_date;
        fileStream<<qint32(lines_count);
        fileStream<<qint32(interval_msec);
        // до сюда фиксированные 24 байта

        //qDebug()<<"save diagram linescount "<<lines_count<<"  point interval "<<interval_msec;

        /// это была фиксированная часть шапки. Дальше идет "резиновая", т.к. кол-во линий и длина строк заранее не известна.
        /// ??? надо ли зарезервировать пустое место для совместимости в случае появления новых параметров? не "резиновых"...

        // >десятич.разряды (+ lines_count байт)
        while(_decimals.count()<lines_count) _decimals.append(1);
        for(int i=0;i<lines_count;i++)
            fileStream<<qint8(_decimals.at(i));

        // >масштаб (+ lines_count*4 байт)
        while(_maxs.count()<lines_count) _maxs.append(1000);
        for(int i=0;i<lines_count;i++)
            fileStream<<qint32(_maxs.at(i));

        // >цвет (+ lines_count*4 байт)
        while(_colors.count()<lines_count) _colors.append(Qt::gray);
        for(int i=0;i<lines_count;i++)
            fileStream<<_colors.at(i).rgb();

        // цвет фона (+ 4 байт)
        fileStream<<_backgroundColor.rgb();

        /// далее текстовая часть "резиновой" части шапки

        // дескриптор (+ минимум 1 байт)
        QByteArray dscr=_description.toUtf8();
        fileStream.writeRawData(dscr, dscr.count());
        fileStream<<qint8(0);
        qDebug()<<"savediag description "<<_description;

        // подписи (+ минимум lines_count байт)
        while(_labels.count()<lines_count) _labels.append("");
        for(int i=0;i<lines_count;i++){
            QByteArray lbl=_labels.at(i).toUtf8();
            //qDebug()<<"line "<<i<<"   lbl count "<<lbl.count();
            fileStream.writeRawData(lbl, lbl.count());
            fileStream<<qint8(0);
        }
        //qDebug()<<"savediag labels "<<_labels;

        /// далее идут еще более резиновые пометки на графике. Это уже нешапка, а таки прямо конкретная информация

        // 2.5 текстовые пометки на графике (+ минимум 2 байта (или 2 + кол-во подписей байт))
        fileStream<<short(notes.count()); // если _decimals ограничиваются 255, то и подписи ограничатся 32768
        for(int i=0;i<notes.count();i++){
            fileStream<<qint32(notes.keys().at(i));
            QByteArray nts=notes.values().at(i).toUtf8();
            fileStream.writeRawData(nts, nts.count());
            fileStream<<qint8(0);
        }
        //qDebug()<<"savediag notes count "<<notes.count();

        points_saved=0;
    }

    /// далее сохранение точек на графике.
    /// заполняем буфер fileData (до 64-256-1024-сколькофантазияпозволит байт), а потом отправляем в поток и очищаем.
    /// (это я сделал чтобы начать плавно переходить к дозаписи в файл после заполнения буфера)

    // 3. кодируем точки в буфер, после заполнения сохраняем.
    QByteArray fileBuffer;
    enum CompressType {initial, delta16bit, delta8bit, delta4bit,
                       delta2bit, replicateStart, replicateNext, none}; // не все сейчас будут использованы
    CompressType compress_last=none, compress_actual=none;
    int replicate_points_count=0;
    qint64 time_last=0; // храним время последней точки таким, как оно будет в файле, и считаем дельту dmsec не от времени предыдущей точки, а от ЭТОЙ, выровненной, величины.
    QVector<int> values_current;
    QVector<int> values_last;
    QVector<int> dValues;
    values_current.fill(0,lines_count);
    values_last.fill(0,lines_count);
    dValues.fill(0,lines_count);
    int dMax;
    // bool initiate=true; // принудительный старт новой последовательности. Судя по всему это не актуально, т.к.
    // для возможности восстановления поврежденных данных буфер начинаем со стартовой точки, а это можно проверить так (fileBuffer.count()==0)

    int com_0b=0,com_2b=0,com_4b=0,com_8b=0,uncom=0; //debug

    for(int index=points_saved;index<points_count;index++){
        qint64 dmsec=times[index]-time_last;
        qint64 dtm=qRound64(double(dmsec)/interval_msec);
        dMax=0;
        for(int line=0;line<lines_count;line++){
            int val=values[index*lines_count+line];
            values_current.replace(line, val);
            dValues.replace(line, values_current.at(line) - values_last.at(line));
            dMax=dMax | qAbs(dValues.at(line));
        }

        //qDebug()<<"dmsec - dtm   "<<dmsec<<" - "<<dtm;
        if(dtm>0){ // если точка ближе чем на 1 интервал, то просто берем следующую (когда-нибудь добавлю дробление интервала).
            time_last+=dtm*interval_msec; //dmsec;//times[index]; // так накопление ошибки будет учитываться при сохранении, и не будет проявляться при загрузке.
            for(int line=0;line<lines_count;line++) values_last.replace(line, values_current.at(line));

            // оцениваем степень сжатия
            if(fileBuffer.count()==0 || dtm>15 || dMax>127) compress_actual=initial; // макс tdm = 15 = 0x0F = 0b0001111
            else if(dtm==1 && dMax==0 && replicate_points_count<250 // почему 250 а не 255?
                           && replicate_points_count>0) compress_actual=replicateNext;
            else if(dtm==1 && dMax==0) compress_actual=replicateStart;
            else if(dMax<2)compress_actual=delta2bit;
            else if(dMax<8)compress_actual=delta4bit;
            else compress_actual=delta8bit;

            // обработка сохранения sequenceNext
            if((replicate_points_count>0) && compress_actual!=replicateNext){
                // я сделал именно так, чтобы не менять значение последнего байта в буфере на каждой итерации, а закидывать в буфер кол-во повторений по факту их завершения.
                fileBuffer.append(char(replicate_points_count)); // не случится ли глюк при значениях > 127 с переполнением, знаками и т.п?
                //qDebug()<<"replicate "<<replicate_points_count<<" points";
                replicate_points_count=0;
            }

            // обработка fileBuffer
            // - определить сколько требуется на точку
            int point_size=0;
            switch(compress_actual){
            case initial: point_size=6+lines_count*4; break; // !!! определиться с форматом записи времени !!!
            case delta8bit: point_size=1+lines_count; break;
            case delta4bit: point_size=1+int(ceil(double(lines_count)/2)); break; // округление в бОльшую сторону
            case delta2bit: point_size=1+int(ceil(double(lines_count)/4)); break; // округление в бОльшую сторону
            case replicateStart: point_size=2; break;
            case replicateNext: point_size=0; break;
            case delta16bit: point_size=0; break;
            case none: point_size=0; break;
            }

            // - отправить буфер в поток и обнулить
            if(fileBuffer.count()+point_size > SAVE_BUFFER_SIZE){
                fileStream.writeRawData(fileBuffer.data(), fileBuffer.count());
                fileBuffer.clear();
                //initiate=true; - это не работает, т.к. этот флаг будет опрошен только на следующей итерации, для следующей точки.
                compress_actual=initial;
                point_size=6+lines_count*4;
            }

            // обработка сжатия
            char byte=0;
            int bitn=0;
            qint64 time_temp=0;
            qint64 value_temp;
            int value_coded;

            switch(compress_actual){
            case initial:
                // не ужатая точка
                time_last=times[index]; // т.к. точка не ужата, указываем её точное время на графике
                time_temp|=((time_last<<1)&0xFE);
                time_temp|=((time_last<<2)&0xFE00);
                time_temp|=((time_last<<3)&0xFE0000);
                time_temp|=((time_last<<4)&0xFFE000000) | ((~time_last) & 0x01010101);

                fileBuffer.append(char(0));
                fileBuffer.append(char(0x20 | ((time_temp>>32) & 0x0F)));
                fileBuffer.append(char((time_temp>>24) & 0xFF));
                fileBuffer.append(char((time_temp>>16) & 0xFF));
                fileBuffer.append(char((time_temp>>8) & 0xFF));
                fileBuffer.append(char (time_temp & 0xFF));

                for(int line=0;line<lines_count;line++){
                    // для поиска ошибок и восстановления во время раскодирования, надо исключить байты == 0x00
                    value_temp=values_last.at(line);
                    if(value_temp>0x7FFFFFF) value_temp=0x7FFFFFF; //134217727;
                    else if(value_temp<-0x7FFFFFF) value_temp=-0x7FFFFFF; //134217727;
                    value_temp+=0x8000000; //134217728;
                    value_coded=0;
                    value_coded|=((value_temp<<1)&0xFE);
                    value_coded|=((value_temp<<2)&0xFE00);
                    value_coded|=((value_temp<<3)&0xFE0000);
                    value_coded|=((value_temp<<4)&0xFE000000) | ((~value_temp) & 0x01010101);

                    fileBuffer.append(char((value_coded>>24) & 0xFF));
                    fileBuffer.append(char((value_coded>>16) & 0xFF));
                    fileBuffer.append(char((value_coded>>8) & 0xFF));
                    fileBuffer.append(char( value_coded & 0xFF));
                }
                //initiate=false;

                uncom++;
                break;

            case replicateNext:
                // ужимаем до "нуля"
                replicate_points_count++;
                break;

            case replicateStart:
                // старт ужатия до "нуля"
                replicate_points_count = 1;
                fileBuffer.append(char(0xFF));

                com_0b++;
                break;

            case delta2bit:
                // ужимаем до 2х бит
                fileBuffer.append(char(dtm|0xC0));
                for(int line=0;line<lines_count;line++){
                    //int shft=dValues.at(line);
                    //qDebug()<<"delta 2bit = "<<shft;

                    byte=byte|char(((dValues.at(line)+2)&0x03)<<bitn);
                    bitn+=2;
                    if(bitn>=8){
                        fileBuffer.append(byte);
                        byte=0;
                        bitn=0;
                    }
                }
                if(bitn){
                    fileBuffer.append(byte);
                }

                com_2b++;
                break;

            case delta4bit:
                // ужимаем до 4х бит
                fileBuffer.append(char(dtm|0x80));
                for(int line=0;line<lines_count;line++){
                    //qDebug()<<"delta 4bit = "<<dValues.at(line)<<" - "<<((dValues.at(line)+8)&0x0F);

                    byte=byte|char(((dValues.at(line)+8)&0x0F)<<bitn);
                    bitn+=4;
                    if(bitn>=8){
                        fileBuffer.append(byte);
                        byte=0;
                        bitn=0;
                    }
                }
                if(bitn){
                    fileBuffer.append(byte);
                }

                com_4b++;
                break;

            case delta8bit:
                // ужимаем до 1 байт
                fileBuffer.append(char(dtm|0x40));
                for(int line=0;line<lines_count;line++)
                    fileBuffer.append(char((dValues.at(line)+128)&0xFF));

                com_8b++;
                break;

            default: qDebug()<<"unexpected compression type status. Check your code!"; // не должно быть такого в данной реализации
            }
            compress_last=compress_actual;
        }
    }
    // если повторение выпало на последнюю точку, тогда в буфер необходимо записать кол-во провторений, иначе последний байтом останется 0xFF.
    if(replicate_points_count>0){
        fileBuffer.append(char(replicate_points_count)); // не случится ли глюк при значениях > 127 с переполнением, знаками и т.п?
        //qDebug()<<"replicate "<<replicate_points_count<<" points";
    }
    // наверняка остался несохраненным буфер
    fileStream.writeRawData(fileBuffer.data(), fileBuffer.count());
    file.close();

    qDebug()<<"diag file saved points count "<<points_count;
    qDebug()<<"initial "<<uncom<<"   8 bit "<<com_8b<<"   4 bit"<<com_4b<<"   2 bit"<<com_2b<<"   replicate "<<com_0b;

    notes_saved = notes.count();
    points_saved = points_count;

    return true;
}

bool FileOfDiagram::startTimer()
{
    // таймер сохранения. Гарантирует сохранение последней точки в файле.
    if(saveTimer==nullptr){
        saveTimer=new QTimer(this);
        QObject::connect(saveTimer,SIGNAL(timeout()),this,SLOT(onSaveFileTimer()));
        saveTimer->setSingleShot(true);
        saveTimer->setInterval(600000); // 10 минут
        saveTimer->start();
    }
    else if(!saveTimer->isActive()) saveTimer->start();

    return true;
}

void FileOfDiagram::onSaveFileTimer()
{
    //saveIonV09File(filename);
    saveFile(filename);

//    if(lastsave_time.secsTo(QTime::currentTime())>300)
//    {
//        lastsave_time=QTime::currentTime();
    //    }
}

FileOfDiagram::FileOfDiagram(QDate date, const QString &filename, int linesCount, int interval, int fileSize):QObject(nullptr)
{
    this->date=date;
    this->filename=filename;
    lines_count=linesCount;
    interval_msec=interval;
    points_count=0;
    points_saved=0;
    // дефолтный размер файла для формирования нового файла
    // при загрузке с диска может отличаться? как и количество линий... или нафик?
    total_size=fileSize;
    lastsave_time=QTime::currentTime();
    notes_saved=0;

    // как тут быть? выделять память только при добавлении точек
    allocated=false;                
    times=nullptr;
    values=nullptr;

    // загрузку файла надо бы осуществлять прямо тут.
    QFile file(filename);
    if(file.exists()){
        //if(!loadIonV09File(filename))
        if(!loadFile(filename))
            qDebug()<<filename<<" error loading";
    }
    else qDebug()<<filename<<" not found";

    // для этого чекаем имя файла с соотв. датой... далее перебором искать?
    // оставим пока на потом.
    // что если разбивка на файлы не по суткам?
    // что если ...
}

FileOfDiagram::~FileOfDiagram()
{
    // если таймер сохранения активен, то значит файл дополнялся и его надо сохранить перед завершением работы.
    if(saveTimer!=nullptr)
        if(saveTimer->isActive()){
            saveTimer->stop();
            onSaveFileTimer();
        }

    free_mem();
}

int FileOfDiagram::linesCount()
{
    return lines_count;
}

int FileOfDiagram::pointsCount()
{
    return points_count;
}

int FileOfDiagram::sizeInPoints()
{
    return total_size;
}

int FileOfDiagram::isAllocated()
{
    return allocated;
}

PointOfDiagram FileOfDiagram::getPoint(int num)
{
    PointOfDiagram point;
    if(allocated && (num<total_size)){
        point.time_msec=times[num];
        int values_index=num*lines_count;
        for(int index=values_index; index<values_index+lines_count; index++){
            point.lines.append(values[index]);
        }
    }

    return point;
}

int FileOfDiagram::findPointIndex(qint64 msec)
{
    if((points_count<1)||(msec<times[0])) return 0;
    if(msec>times[points_count-1]) return points_count;

    int left=0, right=points_count-1;

    // дихотомией ищем наиболее точное совпадение
    while((right-left)>1){
        int center=(right+left)/2;
        if(msec<times[center]) right=center;
        else left=center;
    }
    if((times[right]-msec)<(msec-times[left])) return right;
    else return left;
}

int FileOfDiagram::findPointNextTo(qint64 msec)
{
    if((points_count<1)||(msec<times[0])) return 0;
    if(msec>times[points_count-1]) return points_count;

    int left=0, right=points_count-1;

    // дихотомией ищем наиболее точное совпадение
    while((right-left)>1){
        int center=(right+left)/2;
        if(msec<times[center]) right=center;
        else left=center;
    }
    if(times[left]<msec) return right;
    else return left;
}

PointOfDiagram FileOfDiagram::findPoint(qint64 msec)
{
    PointOfDiagram point;
    int num=findPointIndex(msec);
    if((num>0)&&(num<points_count)) point=getPoint(num);
    return point;
}

PointOfDiagram FileOfDiagram::findPoint(QDateTime time)
{
    PointOfDiagram point;
    if(time.date()==this->date){
        int num=findPointIndex(QDateTime(this->date).msecsTo(time));
        if((num>0)&&(num<points_count)) point=getPoint(num);
    }
    return point;
}

void FileOfDiagram::startAverage(qint64 left)
{
    last_average_index=findPointNextTo(left);
}

PointOfDiagram FileOfDiagram::getAverage(qint64 left, qint64 right)
{
    last_average_index=findPointNextTo(left);
    return nextAverage(right);
    /*
    PointOfDiagram point;
    qint64 *sum=new qint64[lines_count];
    int count=0;
    for(int i=0;i<lines_count;i++) sum[i]=0;
    int index=findPointNextTo(left);
    while((index<points_count)&&(times[index]<right))
    {
        for(int line=0;line<lines_count;line++) sum[line]+=values[index*lines_count+line];
        count++;
    }
    if(count)
    {
        point.time_msec=left;
        for(int line=0;line<lines_count;line++)
        {
            point.lines.append(sum[line]/count);
        }
    }
    last_average_index=index;
    delete[] sum;
    */
}

PointOfDiagram FileOfDiagram::nextAverage(qint64 right)
{
    PointOfDiagram point;
    qint64 *sum=new qint64[lines_count];
    int count=0;
    for(int i=0;i<lines_count;i++) sum[i]=0;
    int index=last_average_index;
    while((index<points_count)&&(times[index]<right)){
        for(int line=0;line<lines_count;line++) sum[line]+=values[index*lines_count+line];
        count++;
    }
    if(count){
        point.time_msec=times[last_average_index];
        for(int line=0;line<lines_count;line++){
            point.lines.append(int(sum[line]/count));
        }
    }
    last_average_index=index;
    delete[] sum;
    return point;
}

QString FileOfDiagram::findNote(qint64 time)
{
    // как лучше, так?
    QMap<qint64,QString>::const_iterator it = notes.lowerBound(time);
    if((it==notes.begin()) && (it.key()<time)) return "";  // В случае если курсор указывает на начало файла,
                          // а в этом конкретном файле пометок пока нет, то функции следует вернуть очевидное значение,
                          // которое намекнуло бы, что пометку следует поискать в предыдущем файле.
    else return it.value();

    // или так?
    /*
    for(int i=notes.count()-1;i>=0;i--)
    {
        QDateTime key=notes.keys().at(i);
        if(key<time) return notes.value(key);
        else return "";
    }
    */
}

QString FileOfDiagram::findNote(QDateTime time)
{
    return "";
}

QMap<qint64, QString> &FileOfDiagram::getNotes()
{
    return notes;
}

/*
int FileOfDiagram::findPointNextTo(QDateTime time)
{
    // для построения графика нужен вот такой метод поиска (индекса)
    // ближайшей точки удовлетворящей условию (point.time>=time)

    if((times.count()<1)||(time<times.first())) return 0;
    if(time>times.last()) return times.count();

    int left=0, right=times.count()-1;

    // дихотомией ищем наиболее точное совпадение
    while((right-left)>1)
    {
        int center=(right+left)/2;
        if(time<times.at(center))
            right=center;
        else left=center;
    }
    if(times.at(left)<time)
        return right;
    else return left;
}
*/
/*
int FileOfDiagram::findPointIndex(QDateTime time)
{
    // PointOfDiagram point;
    // в случае если такая точка не существует возвращаем ерунду:
    // point.time.;
    // или вообще-то стоит возвернуть близжайший осмысленный результат
    // например если график только начал заполняться и попасть курсором точно в границу будет невозможно.

    if(times.count()>0)
    {
        if(time<times.first()) return 0;
        if(time>times.last()) return times.count();

        // проверить будет ли ошибка если массив будет содержать 0,1 или 2е точки.
        // !!! в вычислениях будет ошибка если пусто или точка одна

        int left=0, right=times.count()-1;
        // тут - методом дихотомии натыкать нужную точку.
        // но на самом деле номер точки можно и вычислить с ошибкой в 1-2 позиции.
        // как это встроить в дихотомию так, чтобы это реально сократило кол-во итераций?

        // например так:
        // 1. вычисляем номер точки
        // 2. вычисляем ошибку времени
        // 3. двойное смещение номера точки исходя из этого смещения
        // 4. устанавливаем left и right в соотв. с получившимися результатами:
        // одна граница = вычисл.точка, вторая граница вычисл.точка + смещение
          я пока это уберу. Оно выглядит сложно, некрасиво и кажется малополезно...
        qint64 Tsec = times.first().secsTo(times.last());
        qint64 Pcount = times.count(); // тут кол-во точек или номер последней???
        qint64 pTsec = times.first().secsTo(time);
        double dp = Pcount/Tsec;
        int pnum = pTsec * dp;
        if(pnum<0) pnum = 0;
        else if(pnum>=Pcount) pnum = Pcount-1;

        if(time>times.at(pnum))
        {
            qint64 dTsec = times.at(pnum).secsTo(time);
            right = pnum;
            left = pnum - dTsec*dp*2;

        }
        else
        {
            qint64 dTsec = time.secsTo(times.at(pnum));
            left = pnum;
            right = pnum + dTsec*dp*2;

        }


        // дихотомией догоняем до наиболее точного значения
        while((right-left)>1)
        {
            int center=(right+left)/2;
            if(time<times.at(center))
                right=center;
            else left=center;
        }
        if(time.msecsTo(times.at(right)) < times.at(left).msecsTo(time)) return right;
        else return left;
    }
    else return 0;
}
*/

bool FileOfDiagram::addPoint(const PointOfDiagram &new_point)
{
    if(!allocated) alloc_mem();
    if(points_count>=total_size){
        qDebug()<<"points count maxed ("<<points_count<<" of "<<total_size<<")"<<"   additing moar memo...";
        add_mem();
    }
    if((points_count>0)&&(times[points_count-1]>new_point.time_msec)){
        qDebug()<<"wrong point time ("<<new_point.time_msec<<")";
        return false; // защита от перевода часов назад.

        // вообще не очень понятно что делать в таком случае, стирать старую информацию или игнорить новую?
        // если стирать старую, то при переводе часов на пару дней придется стирать несколько файлов?
        // проще игнорировать новые данные. Пока оставим так.
    }

    times[points_count]=new_point.time_msec;
    for(int line=0;line<lines_count;line++){
        if(line<new_point.lines.count())
            values[points_count*lines_count+line]=new_point.lines.at(line);
        else values[points_count*lines_count+line]=0;
    }
    points_count++;

    startTimer(); // таймер сохранения графика
    return true;
}

bool FileOfDiagram::addNote(qint64 time, QString note)
{
    notes.insert(time, note);    
    startTimer(); // таймер сохранения графика
    // я не знаю какую неудачу тут можно обрабатывать... может если такой ключ уже есть? я хз что вообще в таком случае делать... ну кроме как заменить на другую запись.
    return true;
}

void FileOfDiagram::setMetaData(const QList<int> &decimals, const QList<int> &maxs, const QList<QColor> &colors, const QColor backgroundColor,
                                const QString &description, const QList<QString> &labels)
{
    // надо ли контролировать размеры контейнеров? вдруг кол-во элементов не будет совпадать с кол-вом линий?
    // надо. Но в каком месте программы? При чтении и записи? При создании файла? Здесь?
    // запись в файл может проходить и без установки метадаты, т.к. она необязательна,
    // значит надо либо делать проверку и при создании файла и здесь, либо только при записи.
    _decimals = decimals;
    _maxs = maxs;
    _colors = colors;
    _backgroundColor = backgroundColor;
    _description = description;
    _labels = labels;
}

QList<int> &FileOfDiagram::getDecimals()
{
    return _decimals;
}

QList<int> &FileOfDiagram::getMaxs()
{
    return _maxs;
}

QList<QColor> &FileOfDiagram::getColors()
{
    return _colors;
}

QColor &FileOfDiagram::getBackgroundColor()
{
    return _backgroundColor;
}

QString &FileOfDiagram::getDescription()
{
    return _description;
}

QList<QString> &FileOfDiagram::getLabels()
{
    return _labels;
}


// ------------------------------
//  реализация класса DiagramLog
// ------------------------------

/*   Реализация вложенного класса Screen   */

DiagramLog::Screen::Screen(DiagramLog *parent):QWidget(parent)
{
    this->parent = parent;
    this->setMouseTracking(true);
}

void DiagramLog::Screen::paintEvent(QPaintEvent *)
{
    //QTime start=QTime::currentTime(); // debug paint time

    QPainter *painter = new QPainter(this);

    // отрисовка линий:
    // рисует уже подготовленные поли-линии. А готовиться эти линии должны при поступлении новых данных.

    QBrush brush = painter->brush();
    brush.setColor(parent->backgroundColor);
    brush.setStyle(Qt::SolidPattern);
    painter->setBrush(brush);
    painter->setRenderHint(QPainter::Qt4CompatiblePainting,true);
    painter->drawRect(0,0,width()-1,height()-1);
    style()->pixelMetric(QStyle::PM_SmallIconSize);

    const int gridIntervals[15]={5,10,20,30,60,120,300,600,1200,1800,3600,7200,21600,43200,86400};
    // 1. определяем границы сетки
    // 2. вычисляем приблизительный период сетки (порядка 10-20 линий)
    // 3. определяем ближайший цельный делитель
    int gridIndex=15;
    do gridIndex--;
    while((gridIndex>0)&&(parent->screenWidthSec/gridIntervals[gridIndex]<5));
    int gridInterval = gridIntervals[gridIndex];
    int gridCount = parent->screenWidthSec/gridInterval;

    // 4. от ближайшего кратного момента времени отрисовываем вертикальные линии
    QDate anchorDate;
    anchorDate.setDate(parent->screenPositionTime.date().year(),1,1);
    int gridSeconds=QDateTime(anchorDate).secsTo(parent->screenPositionTime);
    QDateTime gridTime=QDateTime(anchorDate).addSecs((gridSeconds/gridInterval)*gridInterval);
    // целочисленное деление должно автоматом подогнать gridSeconds до значения кратного gridInterval

    QFont fix_fnt=QFont("fixed");
    fix_fnt.setPixelSize(parent->CHARSIZE); // *
    //painter->setFont(QFont("fixed",parent->CHARSIZE));
    painter->setFont(fix_fnt); // *
    //int bottomBorder=painter->fontMetrics().height()*2;
    int bottomBorder=parent->CHARSIZE*2.5; // *
    //qDebug()<<"bottomBorder "<<bottomBorder<<"   CHARSIZE*3 "<<(parent->CHARSIZE*3);

    QPen pen=painter->pen();
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::darkGray); // !!! сделать отдельный цвет для разметки и дефолтный для линий
    painter->setPen(pen);
    pen.setStyle(Qt::DashLine);
    painter->drawLine(0,height()-bottomBorder,width(),height()-bottomBorder);
    painter->setPen(pen);
    for(int d=1;d<5;d++){
        int y=(height()-bottomBorder)*d/5;
        painter->drawLine(0,y,width(),y);
    }

    int day=-1;
    for(int i=0;i<gridCount;i++){
        gridTime=gridTime.addSecs(gridInterval);
        int x=parent->screenPositionTime.secsTo(gridTime)*parent->screenWidthPix/parent->screenWidthSec;
        painter->drawLine(x,0,x,height()-bottomBorder);
        // и подпись
        int text_x=x-parent->CHARSIZE; // *
        if(day!=gridTime.date().day()){
            painter->drawText(text_x,height()-parent->CHARSIZE*1.3,gridTime.toString("HH:mm:ss")); // **
            painter->drawText(text_x,height()-parent->CHARSIZE/2.8,gridTime.toString("dd/MM/yyyy")); // **
            day=gridTime.date().day();
        }
        else painter->drawText(text_x,height()-parent->CHARSIZE,gridTime.toString("HH:mm:ss")); // **
    }

    int linesCount=parent->linesCount;
    int index=0;
    int x1=0;
    int* y = new int[linesCount];
    int* x = new int[linesCount];
    QPen* pens = new QPen[linesCount];

    for(int line=0;line<linesCount;line++){
        y[line]=parent->EMPTY_POINT;
        x[line]=0;
        pens[line]=painter->pen();
        pens[line].setWidth(1);
        pens[line].setStyle(Qt::SolidLine);
        if(line<parent->colors.count())pens[line].setColor(parent->colors.at(line));
        else pens[line].setColor(Qt::darkGray); // <- !!! не черный, а как-то зависит от фона? Если вдруг фон черный...
    }

    while(index<parent->screenBuffer.count()){
        for(int line=0;line<linesCount;line++){
            int y1 = parent->screenBuffer.at(index);
            if((y[line]==parent->EMPTY_POINT)||(y[line]==parent->INTERPOL_POINT)){
                y[line]=y1;
                x[line]=x1;
            }
            else {
                if(y1 != parent->INTERPOL_POINT){
                    // дальше будем чертить
                    painter->setPen(pens[line]);
                    if(y1 == parent->EMPTY_POINT){
                        // просто ставим точку, т.к. если это была одна несчастная точка среди разрывов, то иначе она не будет никак отрисована.
                        painter->drawPoint(x[line],y[line]);
                        y[line]=y1;
                    }
                    else {
                        painter->drawLine(x[line],y[line],x1,y1);
                        y[line]=y1;
                        x[line]=x1;
                    }
                }
                else {
                    // ничего не делаем, нам надо соединить линией разрыв, поэтому сохраненные точки нельзя затирать
                }
            }
            index++;
        }
        x1++;
    }
    delete [] x;
    delete [] y;
    delete [] pens;

    // painter->drawPolyline(parent->lines.at(i)); - работает с такой же скоростью, так что не актуально.

    // отрисовка курсора и цифр:
    // сюда же добавляем полупрозрачные кнопки перемотки
    // рассчитываем ширину текста подписей и цифр, смещения, межстрочных интервалов
    int labelMaxWidth=0;
    if(parent->labelMax.isEmpty()){
        for(int i=0;i<linesCount;i++){
            int labelWidth = painter->fontMetrics().width(parent->labels.at(i));
            if(labelMaxWidth<labelWidth){
                labelMaxWidth=labelWidth;
                parent->labelMax=parent->labels.at(i);
            }
        }
    }
    labelMaxWidth=painter->fontMetrics().width(parent->labelMax);
    int valuesMaxWidth=painter->fontMetrics().width(QString::number(INT_MIN));

    int buttwidth=parent->CHARSIZE*3; // **
    int buttheight=parent->CHARSIZE*2.3; // **
    int buttboarder=parent->CHARSIZE*0.8; // **

    int totalWidth=labelMaxWidth+valuesMaxWidth+parent->CHARSIZE*3;
    int valuesShift=labelMaxWidth+parent->CHARSIZE*2.5;
    int totalShift=0;
    static bool baloon_at_right;
    if(parent->cursorPos<totalWidth+parent->CHARSIZE*2) baloon_at_right=true;
    if(parent->cursorPos>width()-totalWidth-parent->CHARSIZE*2) baloon_at_right=false;
    if(baloon_at_right) totalShift=width()-parent->CHARSIZE-totalWidth;

    // рисуем полупрозрачные кнопки перемотки и сохраняем их координаты
    painter->setPen(QPen(Qt::transparent));
    painter->setBrush(QBrush(QColor(230,230,230,230)));
    parent->scrollLeftRect=QRect(width()/2-buttwidth*4-buttboarder*2,buttboarder,buttwidth,buttheight);
    parent->scrollRightRect=QRect(width()/2,buttboarder,buttwidth,buttheight);
    parent->rewindToTheEndRect=QRect(width()/2+buttwidth+buttboarder,buttboarder,buttwidth,buttheight);

    parent->ScaleOutRect=QRect(width()/2+(buttwidth+buttboarder)*2,buttboarder,buttwidth,buttheight);
    parent->scaleInRect= QRect(width()/2+(buttwidth+buttboarder)*3,buttboarder,buttwidth,buttheight);

    parent->inputDateRect=QRect(width()/2-buttwidth*3-buttboarder,buttboarder,buttwidth*3,buttheight);
    painter->drawRect(parent->scrollLeftRect);
    painter->drawRect(parent->scrollRightRect);
    painter->drawRect(parent->rewindToTheEndRect);

    painter->drawRect(parent->scaleInRect);
    painter->drawRect(parent->ScaleOutRect);
    painter->drawRect(parent->inputDateRect);
    painter->setPen(Qt::black);  // !!! использовать цвет разметки (символы для кнопок: ⏪ ⏩ ⏮ ⏭ ◀ ▶ ➕ ➖ (еще такие есть:             )
    painter->drawText(parent->scrollLeftRect.left()+buttboarder,parent->scrollLeftRect.bottom()-buttboarder," ◀"); // эти стрелочки почему-то плохо центрируются
    painter->drawText(parent->scrollRightRect.left()+buttboarder,parent->scrollRightRect.bottom()-buttboarder," ▶"); // поэтому смещения задал ручками.
    QString toTheEnd=" ▶|";
    if(parent->screenPositionTime>QDateTime::currentDateTime()) toTheEnd="|◀";
    painter->drawText(parent->rewindToTheEndRect.left()+buttboarder,parent->rewindToTheEndRect.bottom()-buttboarder,toTheEnd); // и тут тоже.
    //painter->drawText(parent->rewindToTheEndRect,Qt::AlignCenter,toTheEnd); // вариант с нативной центровкой

    painter->drawText(parent->inputDateRect,Qt::AlignCenter,
                      parent->screenPositionTime.date().toString("dd.MM.yyyy"));
    QFont norm_fnt=QFont("normal"); // **
    norm_fnt.setPixelSize(parent->CHARSIZE); // **
    norm_fnt.setBold(true);
    painter->setFont(norm_fnt);
    painter->drawText(parent->scaleInRect,Qt::AlignCenter,"+");
    painter->drawText(parent->ScaleOutRect,Qt::AlignCenter,"-");

    // отрисовка курсора и цифр:
    painter->setFont(fix_fnt);
    if(parent->cursorPos>0){
        painter->setPen(QPen(QColor(150,150,150,150)));
        painter->drawLine(parent->cursorPos,1,parent->cursorPos,height()-2);

        // нарисовать квадратик полупрозрачный
        painter->setPen(QPen(Qt::transparent));
        painter->setBrush(QBrush(QColor(230,230,230,230)));
        painter->drawRect(5+totalShift,5,totalWidth,height()-10-bottomBorder);

        // нарисовать подписи
        for(int line=0;(line<linesCount)&&(line<parent->cursorPoint.lines.count());line++){
            painter->setPen(QPen(parent->colors.at(line)));
            int y=parent->CHARSIZE/2+(line+1)*(parent->CHARSIZE*1.2); // *
            painter->drawText(parent->CHARSIZE+totalShift,y,parent->labels.at(line));
            QString str=QString::number((double)parent->cursorPoint.lines.at(line)/parent->dividers.at(line),'f',parent->decimals.at(line));
            painter->drawText(valuesShift+totalShift,y,str);
        }
        painter->setPen(QPen(Qt::black)); // !!! использовать цвет разметки
        //if(parent->cursorPoint.time.isValid())
            painter->drawText(parent->CHARSIZE+totalShift,parent->CHARSIZE+(parent->linesCount+1)*(parent->CHARSIZE*1.2), // *
                              QTime(0,0,0).addMSecs(parent->cursorPoint.time_msec).toString("HH:mm:ss")); //  - dd/MM/yyyy
    }
    painter->end();
    delete painter;

    //qDebug()<<"draw time "<<start.msecsTo(QTime::currentTime())<<" msec";
}

/*   Реализация вложенного класса Screen   */


DiagramLog::DiagramLog(const QString &cap, const QString &name, const QString description, int interval, const QList<PlcRegister> &lineRegisters,
                       const QList<int> &lineDecimals, const QList<int> &lineMaxs, const QList<QString> &lineLabels,
                       const QList<QColor> lineColors, QColor backColor, QWidget *parent): ViewElement(cap,PlcRegister("none"),true,parent)
{

    /*
     * Надо скопировать список регистров и другие настройки,
     * сделать что-то с интервалом,
     * создать контейнер с графиками разбитыми по дням,
     * создать и настроить screen для отрисовки,
     * что-то ещё?
     */

    this->name=name;
    this->description=description;

    path=Consts::workDirPath+"/"+name;
    QDir dir;
    dir.mkpath(path);

    // сохраняем рабочие "константы"
    registers=lineRegisters;
    linesCount=registers.count();

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

    backgroundColor=backColor;

    values=new int[linesCount]; // сразу заполняем массив текущих значений по количеству линий
    for(int i=0;i<linesCount;i++)values[i]=0;

    // экран отрисовки графика
    screen = new Screen(this);
    screen->installEventFilter(this);

    // размер буфера будет равен ширине картинки в пикселах помножить на количество линий.
    // надо реализовать изменение размеров буфера при изменении размеров экрана...
    screenWidthPix=1000;
    screenBuffer.insert(0,screenWidthPix*linesCount,0);

    // добавляем элемент на виджет и задаем дефолтные размеры и размещение
    layout()->addWidget(screen);
    if(caption) caption->setWordWrap(false);
    canExpand=true;
    adjustCaption();
    adjustChildrenSize();

    // положение просмотра по умолчанию последние сутки
    screenPositionTime = QDateTime::currentDateTime().addSecs(-10*60*60);
    screenWidthSec = 12*60*60;

    // где будем определяться с количеством точек в файлах?
    // делать их условно резиновыми?
    // проще всего посчитать 24*60*60*1000/interval
    // но этот interval где хранить? или читать сразу из timer?

    // запускаем таймер, заполняющий диаграмму
    timer=new QTimer(this);
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(onTimer()));

    timer->start(interval);
}

DiagramLog::~DiagramLog()
{
    foreach (FileOfDiagram* value, dataFiles.values()) {
        delete value;
    }
    dataFiles.clear();

    delete[] values;
}

void DiagramLog::onTimer()
{
    // 1. найти/создать файл графика
    // 2. добавить точку
    // 3. отрисовать, если диаграмма видна.

    FileOfDiagram* file = openFileOfDiagram(QDate::currentDate());
    PointOfDiagram point;

    // добавляем текущие значения в буфер и смещаем position
    point.time_msec = QTime::currentTime().msecsSinceStartOfDay();

    if(DEBUG_TEST){ // !!! дебаг
        int seed=0;
        switch(int(qrand()%30)){
        case 0: seed=-1; break;
        case 1:
        case 2:
        case 3: seed=100; break;
        case 4:
        case 5:
        case 6:
        case 7: seed=10; break;
        case 8:
        case 9:
        case 10:
        case 11: seed=3; break;
        default: seed=0; break;
        }
        if((file->pointsCount()==0)||(seed==-1))
            for(int line=0;line<linesCount;line++) point.lines.append(qrand()%300+line*30+100);
        else {
            PointOfDiagram last_point=file->getPoint(file->pointsCount()-1);
            if(seed==0){ point=last_point; point.time_msec = QTime::currentTime().msecsSinceStartOfDay(); }
            else for(int line=0;line<linesCount;line++){
                if(!line)point.lines.append(INT_MIN);
                else point.lines.append(qrand()%seed-seed/2+last_point.lines[line]);
            }
        }
        file->addPoint(point);
    }
    else if(gotValues){ // добавляем точку только если данные пришли с контроллера
        for(int line=0;line<linesCount;line++) point.lines.append(values[line]);
        file->addPoint(point);
    }

    // защита от перегрузки в случае очень малого интервала timer:
    if((this->isVisible())&&(abs(last_update_time.secsTo(QTime::currentTime()))>1)){
      updateDiag();
      updateCursor();
      screen->repaint();
      last_update_time=QTime::currentTime();
    }
}

void DiagramLog::moveToDate(QDate date)
{
    screenPositionTime.setDate(date);

    updateDiag();
    updateCursor();
    screen->repaint();
}

void DiagramLog::view()
{
    // определяем позицию регистра в листе и записываем значение регистра в соответствующую ячейку массива
    int n=registers.indexOf(newValReg);
    if(n>=0) values[n]=value;

    // для того чтобы не сохранять нулевые значения на графике, используем флаг "значения получены"
    // (!!! надо бы получить все значения прежде чем устанавливать флаг?)
    gotValues=true;
}

void DiagramLog::adjustChildrenSize()
{
    // !!! это копипаста из DiagViewDirectPaint. Так что если делать правки то сначала там.

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

bool DiagramLog::eventFilter(QObject *target, QEvent *event)
{
    if((target==screen)&&(event->type()==QEvent::MouseMove)){
        QMouseEvent *moveEvent = static_cast<QMouseEvent *>(event);
        if(cursorPos!=moveEvent->pos().x()){
            cursorPos=moveEvent->pos().x(); // выход курсора за пределы сцены будет обрабатываться в updateCursor()
            updateCursor();
            // Это затычка для того чтобы не буферились отрисовки курсора. С игровой мышью отрисовка начинает отставать и этот эффект накапливается.
            int interval=abs(last_repaint_time.msecsTo(QTime::currentTime()));
            if(interval>10){
                screen->repaint();
                last_repaint_time=QTime::currentTime();
            }
        }

        event->accept();
        return true;
    }

    if((target==screen)&&((event->type()==QEvent::MouseButtonPress)||(event->type()==QEvent::MouseButtonDblClick))){
        QMouseEvent *clickEvent = static_cast<QMouseEvent *>(event);

        // сюда вставить обработку нажатий на кнопки перемотки.
        // надо бы сделать 3 кнопки:
        //  1. перемотка на пол экрана влево
        //  2. перемотка на пол экрана вправо
        //  3. выбор даты для перемотки в открывающемся окне (потому что я не очень отдупляю
                                                 // как редактор даты впердолить в имеющийся интерфейс).

        int x=clickEvent->pos().x();
        int x_sec=screenWidthSec*x/screenWidthPix;

        if(clickEvent->button()==Qt::LeftButton){
            if(scrollLeftRect.contains(clickEvent->pos())){
                screenPositionTime=screenPositionTime.addSecs(-screenWidthSec/5);
            } else
            if(scrollRightRect.contains(clickEvent->pos())){
                screenPositionTime=screenPositionTime.addSecs(screenWidthSec/5);
            } else
            if(rewindToTheEndRect.contains(clickEvent->pos())){
                screenPositionTime=QDateTime::currentDateTime().addSecs(-screenWidthSec/2);
            } else
            if(scaleInRect.contains(clickEvent->pos())){
                if(screenWidthSec>10){
                    screenWidthSec=screenWidthSec/2;
                    screenPositionTime=screenPositionTime.addSecs(screenWidthSec/2);
                }
            } else
            if(ScaleOutRect.contains(clickEvent->pos())){
                if(screenWidthSec<100000){ // это приблизительно сутки
                    screenWidthSec=screenWidthSec*2;
                    screenPositionTime=screenPositionTime.addSecs(-screenWidthSec/4);
                }
            } else
            if(inputDateRect.contains(clickEvent->pos())){
                // pop-up окошко редактирования даты:
                // 1. нарисовать его на месте кнопки с датой
                // 2. добавить кнопку закрытия.
                // 3. сделать чтобы кнопка закрытия "Х" закрывала окошко.
                // 4. прописать окошку тип окна поп-ап
                // 5. прописать окошку delete on close.
                // 6. от редактора даты завести сигнал к графику, чтобы при изменении даты перематывался экран
                QWidget *popwind=new QWidget();
                popwind->setLayout(new QHBoxLayout());
                QDateEdit *dedit=new QDateEdit();
                QFont font=getDefaultFont();
                font.setPointSize(font.pointSize()+2);
                dedit->setFont(font);
                dedit->setDisplayFormat("dd.MM.yyyy");
                dedit->setDate(screenPositionTime.date());
                QPushButton *butt=new QPushButton("✕"); // варианты символов: ✓✔ ✕✖ ✅ ✗✘
                butt->setFont(font);
                QPalette pal(butt->palette());
                pal.setColor(QPalette::ButtonText, Qt::darkRed); // или Qt::darkGreen если галочка
                butt->setPalette(pal);
                butt->setFixedSize(font.pointSize()*2,qRound(font.pointSize()*1.8));
                popwind->layout()->addWidget(dedit);
                popwind->layout()->addWidget(butt);
                popwind->layout()->setSpacing(font.pointSize());
                popwind->setWindowFlags(Qt::Popup); // поп-ап окна не имеют рамки и закрываются при потере фокуса
                popwind->move(screen->mapToGlobal(inputDateRect.topLeft()));
                popwind->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(butt,&QPushButton::clicked,popwind,&QWidget::close); // такой способ соединять сигналы и слоты менее громоздкий
                QObject::connect(dedit,&QDateEdit::dateChanged,this,&DiagramLog::moveToDate);
                popwind->show();
                // просто создаем окошко с редактированием даты, поэтому обновлять на экране ничего не надо.
                event->accept();
                return true;
            } else
            if((event->type()==QEvent::MouseButtonDblClick)||(!Resources::TOUCH_SCREEN)){
                if(screenWidthSec>10){
                    screenWidthSec=screenWidthSec/2;
                    screenPositionTime=screenPositionTime.addSecs(x_sec-screenWidthSec*x/screenWidthPix);
                }
            }
        }
        if(clickEvent->button()==Qt::RightButton){
            if(screenWidthSec<100000){ // это приблизительно сутки
                screenWidthSec=screenWidthSec*2;
                screenPositionTime=screenPositionTime.addSecs(x_sec-screenWidthSec*x/screenWidthPix);
            }
        }

        updateDiag();
        updateCursor();
        screen->repaint();

        event->accept();
        return true;
    }

    if((target==screen)&&(event->type()==QEvent::Wheel)){
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

        // !!! прокрутку курсором переделать для ЭТОГО варианта диаграммы
        int sec=screenWidthSec*wheelEvent->delta()/5000;
        if(abs(sec)<1){
            if(wheelEvent->delta()>0) sec=1;
            else sec=-1;
        }
        screenPositionTime=screenPositionTime.addSecs(sec);


        updateDiag(true);
        updateCursor();
        screen->repaint();

        event->accept();
        return true;
    }
    else if((target==screen)&&(event->type()==QEvent::Resize)){
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        int w=resizeEvent->size().width()-2;
        screenBufferResize(w);
        updateDiag();
        updateCursor();

        event->accept();
        return true;
    }
    else if((target==screen)&&(event->type()==QEvent::Leave)){
        cursorPos=-1;
        updateCursor();
        screen->repaint();

        event->accept();
        return true;
    }

    return false;
}

QString DiagramLog::generateFilename(QDate date)
{
    QString filename;
    filename=path+"/"+name+"-"+date.toString("dd.MM.yyyy")+".log";
    return filename;
}

void DiagramLog::screenBufferResize(int width)
{
    // в данном случае буфер у нас работает как промежуточное хранилище усредненных значений.
    // и повторно данные будут использоваться только для перемотки, чтобы уменьшить нагрузку на проц.
    // в случае же изменения размеров экрана, каждую точку придется перерисовать заново.
    // так что проще очистить и перерисовать этот буфер?
    //screenBuffer.clear();
    screenWidthPix=width;
    screenBuffer.resize(screenWidthPix*linesCount);
    screenBuffer.squeeze();
    // т.к. соотношение charsize и размеров текста в пикселах зависит от DPI, то надо придумать более валидный способ?
    CHARSIZE=6+width*7/1000; // *
    //screen_pos_correction=0;
    // => last_screen_position???
}

void DiagramLog::updateCursor()
{
    QDateTime cursorTime = screenPositionTime.addMSecs((qint64)cursorPos*screenWidthSec*1000/screenWidthPix);
    FileOfDiagram *file = openFileOfDiagram(cursorTime.date());
    cursorPoint = file->findPoint(cursorTime);
    cursorNote = file->findNote(cursorTime);
}

void DiagramLog::updateDiag(bool fast)
{
    //QTime start=QTime::currentTime(); // debug buffer time

    // в этом методе ищутся файлы и строится график из них.

    /* очередная попытка расписать алгоритм:
     * 1- устанавливаем актуальный пиксел в 0,
     *    файл и номер точки тоже обнуляем,
     *    актуальное время в начало экрана,
     *    пиксел соответствующий актуальному времени тоже обнуляем.
     *  - пока актуальный пиксел попадает в экран повторяем:
     * 2    - если файл закончился или не начинался
     *          ищем следующий файл, ищем в нем точку.
     * 3    - смещаем актуальное время либо к точке либо к началу файла,
     *          определяем пиксел куда указывает актуальное время
     * 4    - если (это_значение - актуальный_пиксел > 0), то
     *          повторяем пока (актуальный_пиксел < это_значение и < ширина_экрана):
     *              заполняем разрывом или интерполяцией
     * 5    - пока есть файл и точки в нем и пиксел не сменится повторяем:
     *          суммируем точку и переключаем следующую,
     *          устанавливаем актуальное время на точку
     *          вычисляем куда попадает следующее актуальное время
     * 6    - если сумма не пустая
     *          вычисляем значение пиксела и переключаем пиксел
     * 7- переходим к отрисовке.
     *
     * !!! Не доделано. всё ещё есть дыры при переходе от файла к файлу, которых быть не должно.
     *
     */

    // 1.
    FileOfDiagram* dataFile=nullptr;
    qint64 actualFileDays=0;
    qint64 screenPositionMSec=screenPositionTime.time().msecsSinceStartOfDay();
    qint64 actualPointMSec=screenPositionMSec;
    //QDateTime lastActualPointTime;
    //int actualBorderLeft=0;
    int actualPixelCount = screenWidthPix;
    int actualPixel=0; // это по сути положение в буфере, который мы заполняем
    int actualTimePixel=0; // а это то место куда реально попадает текущая точка файла
    int dataFileActualPoint=-1;
    int sumCount=0;
    int dpixmax=screenWidthPix*timer->interval()*2/screenWidthSec;
    qint64 *sum=new qint64[linesCount];

    for(int i=0;i<linesCount;i++) sum[i]=0;

    // !!! сюда в последствии надо будет вставить смещение буфера и начальной и конечной точек отрисовки
    double shift;
    int shift_pix;
    if(fast){
        shift=(double)(screenPositionTime.secsTo(last_screen_position))*screenWidthPix/screenWidthSec - screen_pos_correction;
        shift_pix = qRound(shift);
        screen_pos_correction=shift_pix-shift;
        //qDebug()<<"shift "<<shift_pix<<" correction "<<screen_pos_correction;
    }
    else {
        shift=screenWidthPix;
        shift_pix=screenWidthPix;
    }
    last_screen_position=screenPositionTime;

    if(abs(shift_pix)>=screenWidthPix){
        screen_pos_correction=0;
    }
    else if(shift_pix<0){
        // сдвигаем влево и дорисовываем правую часть
        actualPixel=screenWidthPix+shift_pix;
        actualPixelCount=screenWidthPix;
        for(int i=0;i<(actualPixel)*linesCount;i++)
            screenBuffer.replace(i, screenBuffer.at(i-shift_pix*linesCount));
        // чтобы правильно найти файл с которого будет осуществляться дорисовка:
        actualPointMSec=screenPositionMSec+actualPixel*screenWidthSec*1000/screenWidthPix;
        while(actualPointMSec>MSEC_PER_DAY){
            actualPointMSec-=MSEC_PER_DAY;
            actualFileDays++;
        }
    }
    else if(shift_pix>0){
        // сдвигаем вправо и дорисовываем левую часть
        actualPixel=0;
        actualPixelCount=shift_pix;
        for(int i=screenWidthPix*linesCount-1;i>=(actualPixelCount)*linesCount;i--)
            screenBuffer.replace(i, screenBuffer.at(i-(actualPixelCount)*linesCount));
    }

    //for(int i=actualPixel*linesCount;i<actualPixelCount*linesCount;i++)
    //    screenBuffer.replace(i, 0);

    int pointCounter=0; // дебаг
    int itercnt=0;

    /* появилась идея для отрисовки, которая не требует поиска новой точки в файле каждый раз.
     * и всё остальное будет выглядеть гораздо аккуратнее.
     * 1. ячейки буфера экрана делаем qint64, чтобы не вышло перегрузки
     * 2. заводим ещё один буфер размером = кол-во пикселов (т.е. в linesCount раз меньше)
     * 3. все ячейки очищаем сразу
     * 4. убираем пункт [4]. вместо этого каждую точку файла суммируем к соотв. пикселу (ячейке буфера)
     * 5. по достижении точки выходящей за пределы экрана выходим из цикла
     * 6. вычисляем среднее арифметическое для каждой точки буфера, а  если там было пусто
     * то заполняем интерполяцией или разрывами.
     * вот.
     * Что из этого соответствует?
     */
    bool variant1=true;

    if(variant1){
        while(actualPixel<actualPixelCount){
        // 2.
            if((dataFile==nullptr)||(dataFileActualPoint>=dataFile->pointsCount())){
                dataFile=openFileOfDiagram(screenPositionTime.date().addDays(actualFileDays));
                if(dataFileActualPoint<0)
                    dataFileActualPoint=dataFile->findPointNextTo(actualPointMSec);
                else dataFileActualPoint=0;
                //qDebug()<<"time last point"
                //if(dataFileActualPoint<dataFile->pointsCount())
                //    qDebug()<<"["<<dataFileActualPoint<<"] = "<<dataFile->times[dataFileActualPoint];
            }

        // 3.
            if(dataFileActualPoint < dataFile->pointsCount())
                actualPointMSec = dataFile->times[dataFileActualPoint];
            else {   // если точки в файле закончились, то устанавливаем актуальное время в начало следующего файла
                actualFileDays++;
                actualPointMSec=0;
            }
            actualTimePixel = (actualFileDays * MSEC_PER_DAY + actualPointMSec-screenPositionMSec) * screenWidthPix / (screenWidthSec * 1000);

        // 4. !!! оставим интерполяцию на потом.
            int px=actualTimePixel;
            if(px>actualPixelCount)px=actualPixelCount;
            int pntVal=EMPTY_POINT;
            if((actualTimePixel-actualPixel)<dpixmax)
                pntVal=INTERPOL_POINT;
            while(actualPixel < px){
                int index = actualPixel*linesCount;
                for(int i=index; i<index+linesCount; i++){
                    screenBuffer.replace(i, pntVal);
                }
                actualPixel++;
            }

        // 5.
            if((actualTimePixel<actualPixel)&&(dataFileActualPoint < dataFile->pointsCount())){
                // !!! это всё ещё остается проблемой на стыках файлов. Временно отключаю дебаг сообщение. Не забудь!
                //qDebug()<<"error serching point["<<dataFileActualPoint<<"] pix "<<actualTimePixel
                //        <<" time "<<dataFile->times[dataFileActualPoint]
                //        <<" screen pix "<< actualPixel<<" actual time "<<0;
            }
            // (actualTimePixel<=actualPixel) - такое условие необходимо воизбежание подвисания бесконечного цикла, в случае если по каким-то причинам пиксел для актуальной точки окажется меньше текущего. Но вообще такой ситуации быть не должно.
            int dataFileLinesCount = dataFile->lines_count;
            while((dataFileActualPoint < dataFile->pointsCount()) && (actualTimePixel<=actualPixel)&&(actualPixel<actualPixelCount)){
                // !!! тут может не совпадать кол-во линий файла и диаграммы !!!
                // т.к. мы лезем напрямую в "кишки" файла(для скорости), то необходимо эту разницу учесть!
                int j=dataFileActualPoint*dataFileLinesCount; //linesCount;
                if(actualTimePixel==actualPixel){
                    for(int line=0;line<linesCount;line++){
                        if(line<dataFileLinesCount) sum[line]+=dataFile->values[j];
                        else sum[line]+=0; // ммм ну или типа того.
                        j++;
                    }
                    sumCount++;
                }
                dataFileActualPoint++;
                pointCounter++; // дебаг

                // эта часть буквально повторяет [3.] как избежать этого повторения я пока не придумал.
                if(dataFileActualPoint < dataFile->pointsCount())
                    actualPointMSec = dataFile->times[dataFileActualPoint];
                else {   // если точки в файле закончились, то устанавливаем актуальное время в начало следующего файла
                    actualFileDays++;
                    actualPointMSec=0;
                }
                actualTimePixel = (actualFileDays * MSEC_PER_DAY + actualPointMSec-screenPositionMSec) * screenWidthPix / (screenWidthSec * 1000);
            }

            // 6.
            if(sumCount>0){
                int index = actualPixel*linesCount;
                int h0 = screen->height()-CHARSIZE*2.5; // *
                for(int line=0;line<linesCount;line++){
                    int temp = h0-sum[line]*h0/(maxs.at(line)*sumCount);
                    screenBuffer.replace(index, temp);
                    index++;
                    sum[line]=0;
                }
                actualPixel++;
                sumCount=0;
            }
            itercnt++;
        }
    }
    else {}
    //qDebug()<<"buf iterations "<<itercnt;

    delete[] sum;

    //qDebug()<<"buffer time "<<start.msecsTo(QTime::currentTime())<<" msec" << pointCounter;
}

FileOfDiagram *DiagramLog::openFileOfDiagram(QDate fileDate)
{
    if(dataFiles.contains(fileDate)){
        return dataFiles.value(fileDate);
    }
    else {
        FileOfDiagram* dataFile = new FileOfDiagram(fileDate, generateFilename(fileDate), linesCount, timer->interval(), MSEC_PER_DAY/timer->interval()); // (MSEC_PER_DAY/timer->interval()) <- прикидываем размер файла 1 сутки
        // !!! перед добавлением в коллекцию надо определиться с:
        // > новый файл или нет.
        // > надо ли запихнуть в него новые метаданные? (формат, цвет, подпись)
        if(dataFile->pointsCount()==0){
            //decimals, maxs, RGBA colors, labels.
            dataFile->setMetaData(decimals, maxs, colors, backgroundColor, description, labels);
        }
        dataFiles.insert(fileDate, dataFile);

        // !!! эта часть чисто для дебага
        if(DEBUG_TEST && false)
        if(fileDate<=QDate::currentDate()){
            qint64 tm=0;//QDateTime(fileDate).msecsTo(QDateTime::currentDateTime().addSecs(-60*60*2));
            qint64 now=QDateTime(fileDate).msecsTo(QDateTime::currentDateTime());
            if(now>MSEC_PER_DAY)now=MSEC_PER_DAY;
            //qDebug()<<"make new file tm = "<<tm<<" msec, now = "<<now<<" msec";
            if(tm<0)tm=0;
            PointOfDiagram point;
            for(int line=0;line<linesCount;line++)
                point.lines.append(0);
            while(tm<now){
                point.time_msec = tm;
                for(int line=0;line<linesCount;line++){
                    int value=point.lines.at(line)+qrand()%20-10;
                    if(value<0)value=0;
                    else if(value>maxs.at(line))value=maxs.at(line);
                    point.lines.replace(line, value);
                    //point.lines.append(dataFile->pointsCount()+(line*30));
                }
                dataFile->addPoint(point);
                tm+=timer->interval();
            }
        }
        // !!!

        return dataFile;
    }
}
