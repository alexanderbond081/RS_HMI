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

#include "resources.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QSettings>

bool Resources::TOUCH_SCREEN=false;
bool Resources::EXPAND_FLAGBOXLIST=false;
QString Resources::LANGUAGE;
QMap<QString, QString> Resources::dictionary;

void Resources::loadSettings()
{
    QSettings resources(Consts::workDirPath+"/settings.cfg",QSettings::NativeFormat);
    TOUCH_SCREEN = resources.value("useTouchScreen").toBool();
    LANGUAGE = resources.value("language").toString();
}

void Resources::loadDictionary(QString language)
{
    QString filename="../language/"+language+".txt";
    if(language!="en" && !QFile::exists(filename)){
        qDebug()<<"language file "<<filename<<" doesn't exist. Trying to load english."; // as a default? // by default?
        filename="../language/en.txt";
    }
    QFile file(filename);
    if(!file.exists()){
        qDebug()<<"language file "<<filename<<" doesn't exist";
        return;
    }
    if(file.open(QIODevice::ReadOnly)){
        QTextStream text(&file);
        while(!text.atEnd())
        {
            QStringList lines = text.readLine().split(char(9));
            if(lines.count()==2) dictionary.insert(lines.at(0).trimmed(),lines.at(1).trimmed());
        }
        file.close();
    }
    else qDebug()<<"can't open translation file "<<filename;

    //qDebug()<<"dictionary lines count "<<dictionary.count();
}

const QString Consts::upDirectory = "..";
QString Consts::workDirPath=QDir::homePath()+"/.RS_HMI";
QString Consts::xmlHMIPath=QDir::homePath()+"/.RS_HMI/hmi.xml";

const QStringList Consts::heightMetricUnits  = {"","p","pix","pixels","points","lines","textlines"};
const QStringList Consts::widthMetricUnits  = {"","p","pix","pixels","points","l","ch","letters","chars","characters"};

const QString Consts::programTag = "Program";
const QString Consts::securityTag = "Security";
const QString Consts::recipestorageTag="RecipeStorage";
const QString Consts::recipeeditorwindowTag="RecipeEditorWindow";
const QString Consts::menuTag="Menu";
const QString Consts::menuActionTag="MenuAction";

const QString Consts::layoutTag = "Layout";
const QString Consts::spacerTag = "Spacer";
const QString Consts::stretchTag = "Stretch";
const QString Consts::tabsTag = "Tabs";
const QString Consts::tabTag = "Tab";
const QString Consts::gridTag = "Grid";
const QString Consts::gridlayoutTag = "GridLayout";
const QString Consts::labelTag = "Label";
const QString Consts::ledviewTag = "LedView";
const QString Consts::ledbuttonTag = "LedButton";
const QString Consts::numbuttonTag = "NumButton";
const QString Consts::statusviewTag = "StatusView";
const QString Consts::alarmlistTag = "AlarmList";
const QString Consts::numericviewTag = "NumericView";
const QString Consts::numericeditTag = "NumericEdit";
const QString Consts::tableTag = "Table";
const QString Consts::diagViewTag = "Diagram";
const QString Consts::diagLogViewTag = "DiagramLog";
const QString Consts::textLogTag = "TextLog";

const QString Consts::textListTag = "TextList";
const QString Consts::typeListTag = "TypeList";
const QString Consts::alarmLogTag = "AlarmLog";
const QString Consts::PowerOffTag= "PowerOff";

const QString Consts::intType = "int"; // целые числа с фиксированной точкой
const QString Consts::binType = "bin"; // двоичные числа
const QString Consts::octType = "oct"; // восьмеричные числа
const QString Consts::hexType = "hex"; // шеснадцатиричные
const QString Consts::floatType = "float"; // плавающие числа. Пока не реализовано.
const QString Consts::timeType = "time"; // время. Значение = абсолютное количесво секунд.
const QString Consts::textListType = "textlist"; // выпадающий список. Значение = номер в списке.
const QString Consts::flagListType = "flaglist"; // выпадающий список. чекбоксы в списке соотв. битам числа.
const QString Consts::stringType = "string"; // надписи. Не реализовано... и не ясно как.
const QString Consts::typeListType = "typelist"; // тип зависит от значений в других ячейках.

const QString Consts::defaultHeightAttr = "defaultHeight";
const QString Consts::defaultFontSizeAttr = "defaultFontSize";

const QString Consts::levelsAttr = "levels";
const QString Consts::accessLevelAttr = "accesslevel";
const QString Consts::viewLevelAttr = "viewlevel";
const QString Consts::MD5passAttr = "md5pass";
const QString Consts::actionsAttr = "actions";
const QString Consts::verticalAttr = "vertical";
const QString Consts::rowAttr = "row";
const QString Consts::colAttr = "col";
const QString Consts::rowspanAttr = "rowspan";
const QString Consts::colspanAttr = "colspan";
const QString Consts::captionAttr = "caption";
const QString Consts::titleAttr = "title";
const QString Consts::registerAttr = "register";
const QString Consts::nameAttr = "name";
const QString Consts::dateformatAttr = "dateformat";
const QString Consts::borderAttr = "border";
const QString Consts::spacingAttr = "spacing";
const QString Consts::confirmOnAttr = "confirmon";
const QString Consts::confirmOffAttr = "confirmoff";
const QString Consts::sizeAttr = "size";
const QString Consts::colorAttr = "color";
const QString Consts::fixedWidthAttr = "width";
const QString Consts::fixedHeightAttr = "height";
const QString Consts::fontSizeAttr = "fontsize";
const QString Consts::inversedAttr = "inversed";
const QString Consts::colorsAttr = "colors";
const QString Consts::captionColorAttr = "captioncolor";
const QString Consts::captionFontSizeAttr = "captionfontsize";
const QString Consts::captionWordWrapAttr = "captionwordwrap";
const QString Consts::captionFixedHeightAttr = "captionheight";
const QString Consts::captionFixedWidthAttr = "captionwidth";
const QString Consts::captionAlignmentAttr = "captionalignment";

const QString Consts::textonAttr = "texton";
const QString Consts::textoffAttr = "textoff";

const QString Consts::backgroundAttr = "background";
const QString Consts::backgroundColorAttr = "backgroundcolor";
const QString Consts::imageAttr = "image";
const QString Consts::addStretchAttr = "addstretch";
const QString Consts::coloronAttr = "coloron";
const QString Consts::coloroffAttr = "coloroff";
const QString Consts::imageonAttr = "imageon";
const QString Consts::imageoffAttr = "imageoff";
const QString Consts::minAttr = "min";
const QString Consts::maxAttr = "max";
const QString Consts::decimalsAttr = "decimals";
const QString Consts::rowcountAttr = "rowcount";
const QString Consts::colcountAttr = "colcount";
const QString Consts::labelsAttr = "labels";
const QString Consts::tooltipsAttr = "tooltips";
const QString Consts::registersAttr = "registers";
const QString Consts::regintervalsAttr = "regintervals";
const QString Consts::minvaluesAttr = "minvalues";
const QString Consts::maxvaluesAttr = "maxvalues";
const QString Consts::valuesAttr = "values";
const QString Consts::valueAttr = "value";
const QString Consts::intervalAttr = "interval";
const QString Consts::textAttr = "text";
const QString Consts::typesAttr = "types";
const QString Consts::descriptionAttr = "description";
const QString Consts::delayAttr = "delay";
const QString Consts::messageAttr = "message";


const QMap<QString,QString> Consts::lampOnColorSkin = {{"gray", "../controls/led_off.png"},{"red", "../controls/led_red.png"},
                                               {"green","../controls/led_green.png"},{"blue", "../controls/led_blue.png"},
                                               {"yellow","../controls/led_yellow.png"},{"magenta", "../controls/led_magenta.png"}};

const QMap<QString,QString> Consts::lampOffColorSkin = {{"gray", "../controls/led_off.png"},{"red", "../controls/led_red.png"},
                                             {"green","../controls/led_green.png"},{"blue", "../controls/led_blue.png"},
                                             {"yellow","../controls/led_yellow.png"},{"magenta", "../controls/led_magenta.png"}};

const QMap<QString,QString> Consts::buttonOnColorSkin = {{"gray", "../controls/button_gray_on.png"},
                                                 {"red", "../controls/button_red_on.png"},
                                                 {"blue", "../controls/button_blue_on.png"}};

const QMap<QString,QString> Consts::buttonOffColorSkin = {{"gray", "../controls/button_gray_off.png"},
                                                  {"red", "../controls/button_red_off.png"},
                                                  {"blue", "../controls/button_blue_off.png"}};

void Consts::loadPathCfg(QString pathSettingsFilePath)
{
    QFile settingsfile(pathSettingsFilePath);
    if(pathSettingsFilePath.isEmpty() || !settingsfile.exists()){
        settingsfile.setFileName(QDir::homePath()+"/.RS_HMI/pathsettings.cfg");
        if(!settingsfile.exists())
            settingsfile.setFileName("../pathsettings.cfg");
    }
    if(settingsfile.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"open path config file "<<settingsfile.fileName();
        QTextStream loadsettings(&settingsfile);
        if(!loadsettings.atEnd()){
            Consts::workDirPath=QDir::cleanPath(loadsettings.readLine());
            if(!Consts::workDirPath.isEmpty()) Consts::workDirPath.replace("~",QDir::homePath());
            //qDebug()<<"work directory  "<<Consts::workDirPath;
        }
        if(!loadsettings.atEnd()){
            Consts::xmlHMIPath=loadsettings.readLine();
            if(!Consts::xmlHMIPath.isEmpty()) Consts::xmlHMIPath.replace("~",QDir::homePath());
            //qDebug()<<"HMI file "<<Consts::xmlHMIPath;
        }
    }
    else{
        qDebug()<<"can't open path config file "<<settingsfile.fileName();
        qDebug()<<"default work directory "<<Consts::workDirPath;
        qDebug()<<"HMI default file "<<Consts::xmlHMIPath;
    }

    QDir dir(Consts::workDirPath);
    if(!dir.exists()) dir.mkpath(Consts::workDirPath);
}
