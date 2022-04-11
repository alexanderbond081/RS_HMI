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

#ifndef RESOURCES
#define RESOURCES

#include <QString>
#include <QMap>

class Resources{
public:
    static bool TOUCH_SCREEN;
    static void loadSettings();
    static bool EXPAND_FLAGBOXLIST; // попробуем... это конечно кастылем пахнет.
    static QString LANGUAGE; // язык интерфейса, указанный в файле настроек.
    static QMap<QString, QString> dictionary; // словарь перевода интерфейса на текущий язык
    static void loadDictionary(QString language = "en"); // загрузка словаря, название файла и пути вроде как именно в этом модуле и должны быть известны в первую очередь
    static QString translate(QString key){ // перевод интерфейса по ключу, если значение не найдено, то возвращает ключ
        return dictionary.value(key,"#"+key);
    }
};

class Consts{

public:
    static const QString upDirectory;
    static QString workDirPath;
    static QString xmlHMIPath;

    static const QStringList heightMetricUnits;
    static const QStringList widthMetricUnits;

    static const QString programTag;
    static const QString securityTag;
    static const QString recipestorageTag;
    static const QString recipeeditorwindowTag;
    static const QString menuTag;
    static const QString menuActionTag;

    static const QString layoutTag;
    static const QString spacerTag;
    static const QString stretchTag;
    static const QString tabsTag;
    static const QString tabTag;
    static const QString gridTag;
    static const QString gridlayoutTag;
    static const QString labelTag;
    static const QString ledviewTag;
    static const QString ledbuttonTag;
    static const QString numbuttonTag;
    static const QString statusviewTag;
    static const QString alarmlistTag;
    static const QString numericviewTag;
    static const QString numericeditTag;
    static const QString tableTag;
    static const QString diagViewTag;
    static const QString diagLogViewTag;
    static const QString textLogTag;
    static const QString alarmLogTag;
    static const QString PowerOffTag;

    static const QString textListTag; // для listType ячеек в таблицах.
    static const QString typeListTag; // для varibleType ячеек в таблицах.

    static const QString intType; // целые числа с фиксированной точкой
    static const QString binType; // двоичные числа
    static const QString octType; // восьмеричные числа
    static const QString hexType; // шеснадцатиричные
    static const QString floatType; // плавающие числа. Пока не реализовано.
    static const QString timeType; // время. Значение = абсолютное количесво секунд.
    static const QString textListType; // выпадающий список. Значение = номер в списке.
    static const QString flagListType; // выпадающий список checkbox`ов. Каждый чекбокс в списке соответствует биту.
    static const QString stringType; // надписи. Не реализовано... и не ясно как.
    static const QString typeListType; // тип зависит от значений в других ячейках.


    static const QString defaultHeightAttr;
    static const QString defaultFontSizeAttr;

    static const QString levelsAttr;
    static const QString accessLevelAttr;
    static const QString viewLevelAttr;
    static const QString MD5passAttr;
    static const QString actionsAttr;
    static const QString verticalAttr;
    static const QString rowAttr;
    static const QString colAttr;
    static const QString rowspanAttr;
    static const QString colspanAttr;
    static const QString captionAttr;
    static const QString titleAttr;
    static const QString registerAttr;
    static const QString nameAttr;
    static const QString dateformatAttr;
    static const QString borderAttr;
    static const QString spacingAttr;
    static const QString confirmOnAttr;
    static const QString confirmOffAttr;
    static const QString sizeAttr;
    static const QString colorAttr;
    static const QString fontSizeAttr;
    static const QString fixedWidthAttr;
    static const QString fixedHeightAttr;
    static const QString inversedAttr;
    static const QString colorsAttr;
    static const QString captionColorAttr;
    static const QString captionFontSizeAttr;
    static const QString captionWordWrapAttr;
    static const QString captionFixedHeightAttr;
    static const QString captionFixedWidthAttr;
    static const QString captionAlignmentAttr;
    static const QString backgroundAttr;
    static const QString backgroundColorAttr;
    static const QString imageAttr;
    static const QString addStretchAttr;

    static const QString textonAttr;
    static const QString textoffAttr;

    static const QString coloronAttr;
    static const QString coloroffAttr;
    static const QString imageonAttr;
    static const QString imageoffAttr;
    static const QString minAttr;
    static const QString maxAttr;
    static const QString decimalsAttr;
    static const QString rowcountAttr;
    static const QString colcountAttr;
    static const QString labelsAttr;
    static const QString tooltipsAttr;
    static const QString registersAttr;
    static const QString regintervalsAttr;
    static const QString minvaluesAttr;
    static const QString maxvaluesAttr;
    static const QString valuesAttr;
    static const QString valueAttr;
    static const QString intervalAttr;
    static const QString textAttr;
    static const QString typesAttr;
    static const QString descriptionAttr;
    static const QString delayAttr;
    static const QString messageAttr;


    static const QMap<QString,QString> lampOnColorSkin;
    static const QMap<QString,QString> lampOffColorSkin;
    static const QMap<QString,QString> buttonOnColorSkin;
    static const QMap<QString,QString> buttonOffColorSkin;

    static void loadPathCfg(QString pathSettingsFilePath);
};

#endif // RESOURCES
