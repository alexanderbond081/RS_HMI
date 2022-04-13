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

#include "hmi.h"
#include "hmisecurity.h"
#include <QScrollArea>
#include <QSizePolicy>


//-----------------------
// создание / уничтожение

Hmi::Hmi(QObject *parent) : RegisterIOInterface(parent)
{
    // виджет и коллекция создаются в clear(). Правильно ли это? ведь getWidget могут сделать и ДО
    clear();
}

Hmi::~Hmi()
{
    delete hmiWidget;
    // viewCollection удалится по сигналу onDelete от hmiWidget.
}

//----------------
// приём делегатов

void Hmi::setRegisterData(DataCollection *data)
{
    registerDataCollection = data;
    QObject::connect(registerDataCollection, SIGNAL(DataReadingCompleted()), this, SLOT(onRegisterDataUpdated()));
}

void Hmi::setValidator(PLCRegisterValidator *validator)
{
    registerValidator = validator;
}

//--------------------
// загрузка интерфейса

void Hmi::loadFromXml(const QString &filename)
{
    QFile *file = new QFile(filename);
    if(!file->open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Can't open HMI file";//"Файл с настройками HMI не найден.";
        return;
    }

    clear(); // очистка рабочих объектов необходима т.к. этот метод может быть вызван не один раз за время жизни объекта
    hmiMasterz.push(this);
    xmlTabCollections.push(viewCollection);

    xmlHMIfile = new QXmlStreamReader(file);
    PlcRegister::setDefaultValidator(registerValidator);

    //QFont fnt = hmiWidget->font();
    //fnt.setPointSize(8);
    //hmiWidget->setFont(fnt);

    xmlLayouts.push((QBoxLayout*)hmiWidget->layout()); // добавляем главный layout в стек.
    xmlReadNext(); // этот метод рекурсивный, так что его достаточно запустить 1 раз.

    hmiMasterz.clear();
    xmlLayouts.clear();
    xmlTabCollections.clear();
    xmlGrids.clear();
    xmlTabWidgets.clear();
    delete xmlHMIfile;
}

void Hmi::clear()
{
    actualRegViewMap.clear();

    delete viewCollection;
    viewCollection = new ViewTabCollection();

    hmiMenuList.clear(); // содержимое - дети hmiWidget, за родителем удаляться.
    hmiRecipeStorageList.clear(); // то же самое.
    hmiSecurity=nullptr; // то же самое.

    if(hmiWidget) // проще было бы удалить hmiWidget и создать заново, но предполагается, что перезаполнение виджета не будет приводить к его ищезновению с экрана, как и переподключению сигналов-слотов.
        foreach(QObject *obj, hmiWidget->children()){ delete obj;} // удаляется ли при этом layout? должен по идее.
    else{
        hmiWidget=new QWidget(); //new QScrollArea();
        //((QScrollArea*) hmiWidget.data())->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        //((QScrollArea*) hmiWidget.data())->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

        connect(hmiWidget.data(), SIGNAL(destroyed(QObject*)),SLOT(onHmiWidgetDelete()));
    }
    hmiWidget->setLayout(new QVBoxLayout());
}

QString Hmi::getTitle() const
{
    return title;
}

QWidget *Hmi::getWidget()
{
    // вот такой дебаг устойчивости к удалению главного виджета.
/*
    qDebug()<<"Craaash!!! no?";
    delete hmiWidget;
    hmiWidget = new QWidget();
*/
    // и он приводит к DoubleFreeCorruption. А почему? А потому, что часть объектов содержащихся в ViewElement встраивается в цепочку рекурсивного удаления QWidget. И ничего с этим я сделать не могу.

    return hmiWidget;
}

QList<QMenu *> Hmi::getMenuList()
{
    QList<QMenu*> list;
    foreach(QPointer<QMenu> pm, hmiMenuList){
        if(!pm.isNull()) list<<pm;
    }
    qDebug()<<hmiMenuList.count()<<"  "<<list.count();
    return list;
}

void Hmi::onHmiWidgetDelete()
{
    // предохранение от неожиданного удаления hmiWidget
    delete viewCollection;
    viewCollection=nullptr;
    actualRegViewMap.clear();
    if(registerDataCollection) registerDataCollection->clearReadList();
}

//-----------------
// внутренние слоты

void Hmi::updateRegisterValue(const PlcRegister &reg, int val) // override virtual slot
{
    // это можно было бы напрямую подключить, но что-то меня останавливает и пожалуй зря.
    if(registerDataCollection){
        registerDataCollection->submitRegisterValue(reg, val);
        qDebug()<<reg.toString()<<" - "<<val;
    }
}

void Hmi::onRegisterDataUpdated()
{
    if(registerDataCollection)
        foreach (PlcRegister reg, registerDataCollection->takeUpdatedList()){
            // обновление вьюшки. Одному ключу(регистру) может соответствовать несколько вьюшек.

            foreach(ViewElement *view, actualRegViewMap.values(reg)){
                view->setValue(registerDataCollection->getValue(reg),reg);
            }
        }
}

void Hmi::setWaitCursor()
{
    hmiWidget->setCursor(Qt::BusyCursor);
}

void Hmi::setNormalCursor()
{
    hmiWidget->setCursor(Qt::ArrowCursor);
}

void Hmi::onViewCollectionTabUpdated()
{
    // и это тоже можно было бы напрямую подключить, но что-то меня останавливает, а зря.
    actualRegViewMap = viewCollection->getActualMap();
    if(registerDataCollection){
        registerDataCollection->clearReadList();
        registerDataCollection->addToReadList(actualRegViewMap.keys());
    }

    // !!! пробую сделать так, чтобы представление получало обновленные данные модели сразу, а не после синхронизации с ПЛК !!!
    // добавил вызов сигнала обновления данных...    посмотрим, не приведет ли это к багам (например сбоям в синхронизации)
    if(registerDataCollection->fastSync) onRegisterDataUpdated();
}

//-----------------------------------------------
// формирование HMI на основе данных из XML файла

bool Hmi::xmlReadNext()
{
    bool itWasNextElement=false; // чтобы отличать концовки от текущего Layout/Menu или идентичных вложенных. Остальные элементы друг в друга пока напрямую не вкладываются

    if(!xmlHMIfile->atEnd() && !xmlHMIfile->hasError()) xmlHMIfile->readNext();
    else qDebug() << "xml end or error";
    //qDebug() << "    next - " << xmlHMIfile->tokenString() << " - " << xmlHMIfile->name();

    if (xmlHMIfile->tokenType() == QXmlStreamReader::StartDocument){
        qDebug()<<"startDoc";
        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() && (xmlHMIfile->tokenType()!=QXmlStreamReader::EndDocument))
          xmlReadNext();
    }
    else if(xmlHMIfile->tokenType() == QXmlStreamReader::StartElement){
            if(xmlHMIfile->name() == Consts::programTag)xmlStartProgramHMI();//{ qDebug()<<"prog"; xmlStartProgramHMI();}
            else if(xmlHMIfile->name() == Consts::PowerOffTag)/*xmlPowerOff();*/{ qDebug()<<"poweroff"; xmlPowerOff();}
            else if(xmlHMIfile->name() == Consts::securityTag)xmlSecurity();//{ qDebug()<<"security"; xmlSecurity();}
            else if(xmlHMIfile->name() == Consts::recipestorageTag)xmlRecipeStorage();//{ qDebug()<<"recipe"; xmlRecipeStorage();}
            else if(xmlHMIfile->name() == Consts::recipeeditorwindowTag){ qDebug()<<"recipe editor"; xmlRecipeEditor();}
            else if(xmlHMIfile->name() == Consts::menuTag)xmlMenu();//{ qDebug()<<"menu"; xmlMenu();}
            else if(xmlHMIfile->name() == Consts::menuActionTag)xmlMenuAction();//{ qDebug()<<"menuAct"; xmlMenuAction();}
            else if(xmlHMIfile->name() == Consts::layoutTag)xmlMakeLayout();//{ qDebug()<<"layout"; xmlMakeLayout();}
            else if(xmlHMIfile->name() == Consts::spacerTag)xmlMakeSpacer();//{ qDebug()<<"spacer"; xmlMakeSpacer();}
            else if(xmlHMIfile->name() == Consts::stretchTag)xmlMakeStretch();//{ qDebug()<<"stretch"; xmlMakeStretch();}
            else if(xmlHMIfile->name() == Consts::tabsTag)xmlMakeTabs();//{ qDebug()<<"tabs"; xmlMakeTabs();}
            else if(xmlHMIfile->name() == Consts::tabTag)xmlMakeTab();//{ qDebug()<<"tab"; xmlMakeTab();}
            else if(xmlHMIfile->name() == Consts::gridTag)xmlMakeGrid();//{ qDebug()<<"grid"; xmlMakeGrid();}
            else if(xmlHMIfile->name() == Consts::gridlayoutTag)xmlMakeGridLayout();//{ qDebug()<<"gridlay"; xmlMakeGridLayout();}
            else if(xmlHMIfile->name() == Consts::ledviewTag)xmlMakeLedViewElement();//{ qDebug()<<"ledv"; xmlMakeLedViewElement();}
            else if(xmlHMIfile->name() == Consts::labelTag)xmlMakeLabel();//{ qDebug()<<"label"; xmlMakeLabel();}
            else if(xmlHMIfile->name() == Consts::numbuttonTag){ qDebug()<<"ledb"; xmlMakeNumButtonElement();}
            else if(xmlHMIfile->name() == Consts::ledbuttonTag)xmlMakeLedButtonElement();//{ qDebug()<<"ledb"; xmlMakeLedButtonElement();}
            else if(xmlHMIfile->name() == Consts::numericviewTag)xmlMakeNumericViewElement();//{ qDebug()<<"numv"; xmlMakeNumericViewElement();}
            else if(xmlHMIfile->name() == Consts::statusviewTag)xmlMakeStatusViewElement();//{ qDebug()<<"statv"; xmlMakeStatusViewElement();}
            else if(xmlHMIfile->name() == Consts::alarmlistTag)xmlMakeAlarmListElement();//{ qDebug()<<"alarmlst"; xmlMakeAlarmListElement();}
            else if(xmlHMIfile->name() == Consts::alarmLogTag)xmlMakeAlarmLog();//{ qDebug()<<"alarmLog"; xmlMakeAlarmLog();}
            else if(xmlHMIfile->name() == Consts::numericeditTag)xmlMakeNumericEditElement();//{ qDebug()<<"nume"; xmlMakeNumericEditElement();}
            else if(xmlHMIfile->name() == Consts::textListTag)xmlAppendTextList();//{ qDebug()<<"textlist"; xmlAppendTextList();}
            else if(xmlHMIfile->name() == Consts::typeListTag)xmlAppendTypeList();//{ qDebug()<<"typelist"; xmlAppendTypeList();}
            else if(xmlHMIfile->name() == Consts::tableTag)xmlMakeTableElement();//{ qDebug()<<"table"; xmlMakeTableElement();}
            else if(xmlHMIfile->name() == Consts::diagViewTag)xmlMakeDiagram();//{ qDebug()<<"diargam"; xmlMakeDiagram();}
            else if(xmlHMIfile->name() == Consts::diagLogViewTag)xmlMakeDiagramLog();//{ qDebug()<<"diargamLog"; xmlMakeDiagramLog();}
            else if(xmlHMIfile->name() == Consts::textLogTag)xmlMakeTextLog();//{ qDebug()<<"textLog"; xmlMakeTextLog();}
            itWasNextElement=true;
        }
    return itWasNextElement;
}

void Hmi::xmlStartProgramHMI()
{
    if(xmlHMIfile->name() == Consts::programTag){

        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::titleAttr)){
            title = attr.value(Consts::titleAttr).toString().trimmed();
            // далее надо запихнуть название в подпись главного окна и т.п.
            // для этого послать сигнал.

        }
        if(attr.hasAttribute(Consts::defaultHeightAttr)){
            int dh= attr.value(Consts::defaultHeightAttr).toInt();
            ViewElement::setDefaultHeight(dh);

            int spacing=ViewElement::getDefaultSpacing();
            int margin = spacing;
            hmiWidget->layout()->setSpacing(spacing);
            hmiWidget->layout()->setMargin(margin);
        }
        if(attr.hasAttribute(Consts::defaultFontSizeAttr)){
            int dfs= attr.value(Consts::defaultFontSizeAttr).toInt();
            QFont fnt=hmiWidget->font();
            fnt.setPointSize(dfs);
            hmiWidget->setFont(fnt);
            ViewElement::setDefaultFont(fnt);
        }

        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::programTag)))
          xmlReadNext();

        connect(viewCollection, SIGNAL(actualMapUpdated()), SLOT(onViewCollectionTabUpdated())); // переключение вкладок будет менять набор регистров для опроса.
        onViewCollectionTabUpdated(); // имитируем сигнал обновления, чтобы сформировать набор регистров начальной страницы
        if(hmiSecurity)hmiSecurity->resetAccess(); // сбрасываем доступ к виджетам (чтобы спрятать тех кто не должен быть виден).
    }
}

void Hmi::xmlCommonAttributes(ViewElement *element)
{
    QXmlStreamAttributes attr = xmlHMIfile->attributes();

    // уровень доступа
    if(hmiSecurity){
        int accessLvl=0;
        int viewLvl=0;
        if(attr.hasAttribute(Consts::accessLevelAttr))accessLvl = attr.value(Consts::accessLevelAttr).toInt();
        if(attr.hasAttribute(Consts::viewLevelAttr))viewLvl = attr.value(Consts::viewLevelAttr).toInt();
        hmiSecurity->SecureAccess(element,viewLvl,accessLvl); // надо ли сделать игнор в случае уровень==0
    }

    // цвет текста
    if(attr.hasAttribute(Consts::colorAttr)){
        QColor textColor;//=Qt::black;
        textColor=strToColor(attr.value(Consts::colorAttr).toString());
        element->setColor(textColor);
    }
    // цвет подписи
    if(attr.hasAttribute(Consts::captionColorAttr)){
        QColor textColor;//=Qt::black;
        textColor=strToColor(attr.value(Consts::captionColorAttr).toString());
        element->setCaptionColor(textColor);
    }
    // размер текста
    if(attr.hasAttribute(Consts::fontSizeAttr)){
        int fs;
        fs=attr.value(Consts::fontSizeAttr).toInt();
        element->setFontSize(fs);
    }
    // размер текста подписи
    if(attr.hasAttribute(Consts::captionFontSizeAttr)){
        int fs;
        fs=attr.value(Consts::captionFontSizeAttr).toInt();
        element->setCaptionFontSize(fs);
    }
    // перенос слов подписи
    if(attr.hasAttribute(Consts::captionWordWrapAttr)){
        bool ww;
        ww=attr.value(Consts::captionWordWrapAttr).toInt();
        element->setCaptionWordWrap(ww);
    }
    // высота
    if(attr.hasAttribute(Consts::fixedHeightAttr)){
        int fix_h;
        QString str_h;
        bool ok;

        str_h=attr.value(Consts::fixedHeightAttr).toString();
        int n;
        for(n=Consts::heightMetricUnits.count()-1;n>0;n--){
            if(str_h.contains(Consts::heightMetricUnits.at(n)))
                break;
        }
        if(n>0) str_h=str_h.remove(Consts::heightMetricUnits.at(n)).trimmed();
        fix_h=str_h.toInt(&ok);
        if(ok){
            // !!! требует доработки, т.к. высота в линиях текста работает приблизительно.
            if(n>4) fix_h = fix_h*(element->fontMetrics().lineSpacing()+1)+ViewElement::getDefaultSpacing()*1+4; // + ViewElement::getDefaultSpacing()*0.5);// + 16; // height(); // lineSpacing();
            element->setHeight(fix_h);
        }
    }
    // ширина
    if(attr.hasAttribute(Consts::fixedWidthAttr)){
        int fix_w;
        QString str_w;
        bool ok;

        str_w=attr.value(Consts::fixedWidthAttr).toString();
        int n;
        for(n=Consts::widthMetricUnits.count()-1;n>0;n--){
            if(str_w.contains(Consts::widthMetricUnits.at(n)))
                break;
        }
        if(n>0) str_w=str_w.remove(Consts::widthMetricUnits.at(n)).trimmed();
        fix_w=str_w.toInt(&ok);
        if(ok){
            // !!! требует доработки, т.к. ширина в знакоместах текста работает приблизительно. А так же "element->fontMetrics()" не имеет отношения к шрифту элемента и подписи.
            if(n>4) fix_w = (fix_w+2)*(element->fontMetrics().averageCharWidth()); // + ViewElement::getDefaultSpacing()*0.5);// + 16; // height(); // lineSpacing();
            element->setWidth(fix_w);
        }
    }
    // высота подписи
    if(attr.hasAttribute(Consts::captionFixedHeightAttr)){
        int fix_h;
        QString str_h;
        bool ok;

        str_h=attr.value(Consts::captionFixedHeightAttr).toString();
        int n;
        for(n=Consts::heightMetricUnits.count()-1;n>0;n--){
            if(str_h.contains(Consts::heightMetricUnits.at(n)))
                break;
        }
        if(n>0) str_h=str_h.remove(Consts::heightMetricUnits.at(n)).trimmed();
        fix_h=str_h.toInt(&ok);
        if(ok){
            // !!! требует доработки, т.к. высота в линиях текста работает приблизительно.
            if(n>4) fix_h = fix_h*(element->fontMetrics().lineSpacing()+1)+ViewElement::getDefaultSpacing()*1+4; // + ViewElement::getDefaultSpacing()*0.5);// + 16; // height(); // lineSpacing();
            element->setCaptionHeight(fix_h);
        }
    }
    // ширина подписи
    if(attr.hasAttribute(Consts::captionFixedWidthAttr)){
        int fix_w;
        QString str_w;
        bool ok;

        str_w=attr.value(Consts::captionFixedWidthAttr).toString();
        int n;
        for(n=Consts::widthMetricUnits.count()-1;n>0;n--){
            if(str_w.contains(Consts::widthMetricUnits.at(n)))
                break;
        }
        if(n>0) str_w=str_w.remove(Consts::widthMetricUnits.at(n)).trimmed();
        fix_w=str_w.toInt(&ok);
        if(ok){
            // !!! требует доработки, т.к. ширина в знакоместах текста работает приблизительно.А так же "element->fontMetrics()" не имеет отношения к шрифту элемента и подписи.
            if(n>4) fix_w = (fix_w+2)*(element->fontMetrics().averageCharWidth()); // + ViewElement::getDefaultSpacing()*0.5);// + 16; // height(); // lineSpacing();
            element->setCaptionWidth(fix_w);
        }
    }
    // выравнивание(alignment) подписи
    if(attr.hasAttribute(Consts::captionAlignmentAttr)){
        QString align;

        align=attr.value(Consts::captionAlignmentAttr).toString();
        if(align=="left") element->setCaptionAlighnment(Qt::AlignLeft);
        else if (align=="right") element->setCaptionAlighnment(Qt::AlignRight);
        else element->setCaptionAlighnment(Qt::AlignCenter);
    }

    // цвет фона???
}

void Hmi::xmlPowerOff()
{
    ///////////////////////////////////////////////////////
    //  выключение панели управления по сигналу от контроллера  //
    ///////////////////////////////////////////////////////

    QString regstr;
    int value=1;
    int delay=10;
    QString msg;
    if(xmlHMIfile->name() == Consts::PowerOffTag){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();

        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::valueAttr)) value =  attr.value(Consts::valueAttr).toInt();
        if(attr.hasAttribute(Consts::delayAttr)) delay = attr.value(Consts::delayAttr).toInt();
        if(attr.hasAttribute(Consts::messageAttr))msg = attr.value(Consts::messageAttr).toString();

        poweroff = new PowerOff(PlcRegister(regstr), value, delay, msg, hmiWidget);
        viewCollection->addViewElement(regstr, poweroff); // добавить в коллекцию виджетов синхронизировать с контроллером.
    }
}

void Hmi::xmlSecurity()
{

    ///////////////////////////////////////////////////////
    //  секьюрити. вкл/выкл видимости/доступа к виджетам
    //////////////////////////////////////////////////////

    QString regstr;
    QList<int> lvlsList;
    QList<QString> passList;
    if(xmlHMIfile->name() == Consts::securityTag){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();

        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::levelsAttr))
            foreach(QString str, attr.value(Consts::levelsAttr).toString().split(",")){
                lvlsList<<str.trimmed().toInt();
            }
        if(attr.hasAttribute(Consts::MD5passAttr))
            foreach(QString pass, attr.value(Consts::MD5passAttr).toString().split(","))
                passList<<pass.trimmed();

        qDebug()<<lvlsList;
        qDebug()<<passList;
        if(lvlsList.count()>0){ // возможно надо сделать > 1? смысл в одном варианте доступа...
            if(hmiSecurity) delete hmiSecurity;
            hmiSecurity = new HmiSecurity(PlcRegister(regstr), lvlsList, passList, hmiWidget);
            viewCollection->addViewElement(regstr, hmiSecurity); // добавить в коллекцию виджетов синхронизировать с контроллером.
        }
    }
}

void Hmi::xmlSecureWidget(QWidget *widget)
{
    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    // уровень доступа
    if(hmiSecurity){
        int accessLvl=0;
        int viewLvl=0;
        if(attr.hasAttribute(Consts::accessLevelAttr))accessLvl = attr.value(Consts::accessLevelAttr).toInt();
        if(attr.hasAttribute(Consts::viewLevelAttr))viewLvl = attr.value(Consts::viewLevelAttr).toInt();
        hmiSecurity->SecureAccess(widget,viewLvl,accessLvl); // надо ли сделать игнор в случае уровень==0
    }
}

void Hmi::xmlSecureAction(QAction *action)
{
    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    // уровень доступа
    if(hmiSecurity){
        int accessLvl=0;
        int viewLvl=0;
        if(attr.hasAttribute(Consts::accessLevelAttr))accessLvl = attr.value(Consts::accessLevelAttr).toInt();
        if(attr.hasAttribute(Consts::viewLevelAttr))viewLvl = attr.value(Consts::viewLevelAttr).toInt();
        hmiSecurity->SecureAccess(action,viewLvl,accessLvl); // надо ли сделать игнор в случае уровень==0
    }
}

void Hmi::xmlRecipeStorage()
{
    //////////////////////////////////
    // рецепт. сохранение и загрузка
    /////////////////////////////////

    /* что нужно сделать:
       1.прочитать теги:
            -имя
            -список регистров
       2.создать объект и засунуть в лист
       3.сигналы слоты? кажется их не сюда. */

    QString cap="recipe";
    QList<PlcRegister> registerslist;

    if(xmlHMIfile->name() == Consts::recipestorageTag){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registersAttr))
            foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
                if(str.contains("-")){
                    // вставить рассчёт диапозона регистров
                    QStringList strsplit=str.split("-");
                    if(strsplit.count()==2){
                        PlcRegister reg1=strsplit.at(0).trimmed(),
                                    reg2=strsplit.at(1).trimmed();
                        if(reg1.getType()==reg2.getType())
                            for(int i=0;i<=(reg2.getIndex()-reg1.getIndex());i++)
                            {
                                registerslist<<(reg1+i);
                                //qDebug()<<registerslist.last().toString();
                            }
                    }
                }
                else registerslist<<PlcRegister(str.trimmed());
            }

        RecipeStorage *stor=new RecipeStorage(cap,registerslist,registerDataCollection,hmiWidget);
        hmiRecipeStorageList.append(stor);
        connect(stor,SIGNAL(saveStart()),SLOT(setWaitCursor()));
        connect(stor,SIGNAL(saveFinish()),SLOT(setNormalCursor()));
    }
}

void Hmi::xmlRecipeEditor()
{
      /////////////////////////////////
     //   окно редактора рецептов   //
    /////////////////////////////////

    QString cap="recipe";
    QList<PlcRegister> registerslist;

    if(xmlHMIfile->name() == Consts::recipeeditorwindowTag){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registersAttr))
            foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
                if(str.contains("-")){
                    // вставить рассчёт диапозона регистров
                    QStringList strsplit=str.split("-");
                    if(strsplit.count()==2){
                        PlcRegister reg1=strsplit.at(0).trimmed(),
                                    reg2=strsplit.at(1).trimmed();
                        if(reg1.getType()==reg2.getType())
                            for(int i=0;i<=(reg2.getIndex()-reg1.getIndex());i++)
                            {
                                registerslist<<(reg1+i);
                                //qDebug()<<registerslist.last().toString();
                            }
                    }
                }
                else registerslist<<PlcRegister(str.trimmed());
            }

        RecipeEditorWindow *reditor=new RecipeEditorWindow(cap,registerslist,registerDataCollection,hmiWidget);
        // !!! здесь - вставляем editor в стек для подключения сигналов от viewElement'ов, запускаем xmlReadNext..
        ViewTabCollection *editorTab = new ViewTabCollection(); // создаём новую вкладку коллекции
        xmlTabCollections.push(editorTab); // и помещаем в стек
        hmiMasterz.push(reditor);
        xmlLayouts.push((QBoxLayout*)reditor->getEditorLayout());

        qDebug()<< " заполняем окно редактора";

        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::recipeeditorwindowTag)))
          xmlReadNext();

        qDebug()<< " окно редактора готово.";

        xmlLayouts.pop();
        hmiMasterz.pop();
        reditor->setViewElementsMap(xmlTabCollections.pop()->getAllMap());
        delete editorTab;
        hmiRecipeEditorWindowsList.append(reditor);
        connect(reditor,SIGNAL(loadFromPLCStart()),this,SLOT(setWaitCursor()));
        connect(reditor,SIGNAL(loadFromPLCFinish()),this,SLOT(setNormalCursor()));
    }
}

void Hmi::xmlMakeTextLog()
{
    QString name = "textlog";
    int interval=1000;
    QList<PlcRegister> registerslist;
    QList<int> decimalslist;
    QList<QString> labelslist;

    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    if(attr.hasAttribute(Consts::nameAttr))name = attr.value(Consts::nameAttr).toString();
    if(attr.hasAttribute(Consts::intervalAttr))interval = attr.value(Consts::intervalAttr).toInt();
/*    if(attr.hasAttribute(Consts::registersAttr))
        foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
            registerslist<<PlcRegister(str.trimmed());
        }*/
    if(attr.hasAttribute(Consts::registersAttr))
        foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
            if(str.contains("-")){
                // вставить рассчёт диапозона регистров
                QStringList strsplit=str.split("-");
                if(strsplit.count()==2){
                    PlcRegister reg1=strsplit.at(0).trimmed(),
                                reg2=strsplit.at(1).trimmed();
                    if(reg1.getType()==reg2.getType())
                        for(int i=0;i<=(reg2.getIndex()-reg1.getIndex());i++)
                            registerslist<<(reg1+i);
                }
            }
            else registerslist<<PlcRegister(str.trimmed());
        }

    if(attr.hasAttribute(Consts::decimalsAttr))
        foreach(QString str, attr.value(Consts::decimalsAttr).toString().split(",")){
            decimalslist<<str.trimmed().toInt();
        }
    if(attr.hasAttribute(Consts::labelsAttr))
        foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
            labelslist<<lbl.trimmed();


    // создаём элемент на основани известных атрибутов
    ViewElement *element = new TextfileLog(name, interval, registerslist, decimalslist, labelslist, hmiWidget);

    foreach(PlcRegister reg, registerslist.toSet())
        viewCollection->addViewElement(reg, element);

}

void Hmi::xmlMenu()
{
    ///////////
    // меню
    //////////

    QString cap="menu";
    if(xmlHMIfile->name() == Consts::menuTag){
        qDebug()<<"qmenu";
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();

        QMenu *menu = new QMenu(cap,hmiWidget);
        xmlMenus.push(menu);

        //xmlSecureAction(menuCallAction);
    /* это тупик. До вставки меню в надменю не существует QAction к которому можно применить
       xmlSecureAction. И если с вложенными менюшками ещё можно извернуться прочитав тег вначале
       ЭТОГО метода, а применив secure в конце, то с ВЕРХНИМИ меню, которые вставляются в файле
       mainwindow.cpp в методе addHmi() я вообще не знаю что делать. */
        int accessLvl=0;
        int viewLvl=0;
        if(attr.hasAttribute(Consts::accessLevelAttr))accessLvl = attr.value(Consts::accessLevelAttr).toInt();
        if(attr.hasAttribute(Consts::viewLevelAttr))viewLvl = attr.value(Consts::viewLevelAttr).toInt();


        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd())// &&
            // !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::menuTag)))
            //if(!xmlReadNext()) break;
        if(!xmlReadNext()&& // если следующие условия не относятся к вложенному элементу.
                (xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&
                (xmlHMIfile->name()==Consts::menuTag))
            break;

        xmlMenus.pop();
        if(xmlMenus.isEmpty()) hmiMenuList<<menu;
        else {
            QAction *menuCallAction = xmlMenus.top()->addMenu(menu);
            // вложенные меню мы можем обработать SecureAccess т.е. на этом этапе можем получить QAction, который их вызывает.
            if(hmiSecurity)
                hmiSecurity->SecureAccess(menuCallAction ,viewLvl,accessLvl);
        }
    }
}

void Hmi::xmlMenuAction()
{
    /////////////////
    // пункт меню
    ////////////////
    /*
        активирует указанное действие.
        действия придётся указывать сложной строкой
        и делать соотв. разбор (парсить)
        возможные действия:
        - рецепты сохранение/загрузка
        - уровень доступа
        - возможно скрипты будут
    */

    QString cap="menuAction";
    QStringList actions;
    if((xmlHMIfile->name() == Consts::menuActionTag)&&(!xmlMenus.isEmpty())){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::actionsAttr))
            foreach(QString act, attr.value(Consts::actionsAttr).toString().split(","))
                actions << act.trimmed();

        QAction *menuAction = xmlMenus.top()->addAction(cap);

        xmlSecureAction(menuAction);

        // дальше парсим [actions] и подключаемся к слотам.
        foreach(QString act, actions){
            QStringList args=act.split(":");
            // учесть количество аргументов
            // определить тип приёмника
            // определить индекс
            // определить действие если требуется.

            // -- вызов хранилища рецептов -- //
            if((args.count()>2)&&(args.at(0)==Consts::recipestorageTag)){
                qDebug()<<args<<" - "<<hmiRecipeStorageList.count();
                int i=args.at(1).toInt();
                if((i>=0)&&(hmiRecipeStorageList.count()>i)){
                    if(args.at(2)=="save") QObject::connect(menuAction, SIGNAL(triggered(bool)), hmiRecipeStorageList.at(i), SLOT(save()));
                    else if(args.at(2)=="load") QObject::connect(menuAction, SIGNAL(triggered(bool)), hmiRecipeStorageList.at(i), SLOT(load()));
                }
                else qDebug()<<"recipe action index out of bounds"; //"индекс выходит за рамки";
            }
            else   // -- вызов смены уровня доступа -- //
                if((hmiSecurity)&&(args.count()>1)&&(args.at(0)==Consts::securityTag)){
                    if(args.at(1)=="reset") QObject::connect(menuAction, SIGNAL(triggered(bool)), hmiSecurity, SLOT(resetAccess()));
                    else
                    if(args.at(1)=="access") QObject::connect(menuAction, SIGNAL(triggered(bool)), hmiSecurity, SLOT(passwordDialog()));
                }
            else   // -- вызов окна редактора рецептов -- //
                if((args.count()>2)&&(args.at(0)==Consts::recipeeditorwindowTag)){
                    qDebug()<<args<<" - "<<hmiRecipeEditorWindowsList.count();
                    int i=args.at(1).toInt();
                    if((i>=0)&&(hmiRecipeEditorWindowsList.count()>i)){
                        if(args.at(2)=="show") QObject::connect(menuAction, SIGNAL(triggered(bool)), hmiRecipeEditorWindowsList.at(i), SLOT(show()));
                    }
                    else qDebug()<<"recipe action index out of bounds"; //"индекс выходит за рамки";
                }
        }
    }
}

void Hmi::xmlMakeLayout()
{
    //////////////////
    // просто лейаут
    /////////////////

    if(xmlHMIfile->name() == Consts::layoutTag){
        QBoxLayout *newLayout;
        int vertical=1;
        int border=0;

        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::borderAttr))border = attr.value(Consts::borderAttr).toInt();

        if(vertical)newLayout = new QVBoxLayout();
        else newLayout = new QHBoxLayout();

        int spacing=ViewElement::getDefaultSpacing();
        if(attr.hasAttribute(Consts::spacingAttr)) spacing = attr.value(Consts::spacingAttr).toInt();
        int margin = spacing/3;
        newLayout->setSpacing(spacing);
        newLayout->setMargin(margin);

        //newLayout->setAlignment(Qt::AlignLeft|Qt::AlignTop);

        QFrame *frame=nullptr;
        if(border)
        {
            frame=new QFrame;
            frame->setFrameShape(QFrame::StyledPanel);
            frame->setFrameShadow(QFrame::Plain);
        }

        xmlLayouts.push(newLayout);
        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd())
          if(!xmlReadNext()&& // если следующие условия не относятся к вложенному элементу.
                  (xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&
                  (xmlHMIfile->name()==Consts::layoutTag))
              break;

        // !!! debug
        //QLabel *lab = new QLabel("<eol>");
        //lab->setFixedHeight(22);
        //newLayout->addWidget(lab);

        int addstretch=1;
        if(attr.hasAttribute(Consts::addStretchAttr))addstretch = attr.value(Consts::addStretchAttr).toInt();
        if(addstretch) newLayout->addStretch();

        xmlLayouts.pop();
        if(border)
        {
            frame->setLayout(newLayout);
            xmlLayouts.top()->addWidget(frame);
        }
        else xmlLayouts.top()->addLayout(newLayout);
    }
}

void Hmi::xmlMakeSpacer()
{
    /////////////////////////////
    // просто Spacer в лейауте //
    /////////////////////////////

    if(xmlHMIfile->name() == Consts::spacerTag){
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        int size=24;
        if(attr.hasAttribute(Consts::sizeAttr)) size = attr.value(Consts::sizeAttr).toInt();
        xmlLayouts.top()->addSpacing(size);
    }
}

void Hmi::xmlMakeStretch()
{
    /////////////////////////////
    // расширялка в лейауте //
    /////////////////////////////

    if(xmlHMIfile->name() == Consts::stretchTag){
        xmlLayouts.top()->addStretch();
    }
}

void Hmi::xmlMakeGrid()
{
    //////////////
    //  сетка
    /////////////

    if(xmlHMIfile->name() == Consts::gridTag){
        QGridLayout *grid = new QGridLayout();
        xmlGrids.push(grid);

        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::gridTag)))
          xmlReadNext();

        xmlGrids.pop();
        xmlLayouts.top()->addLayout(grid);
    }
}

void Hmi::xmlMakeGridLayout()
{
    //////////////////////////////
    //  лейаут в составе сетки
    /////////////////////////////

    if(xmlHMIfile->name() == Consts::gridlayoutTag){
        QBoxLayout *newLayout;
        int vertical=1,row=1,col=1,rowSpan=1,colSpan=1;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::rowAttr))row = attr.value(Consts::rowAttr).toInt();
        if(attr.hasAttribute(Consts::colAttr))col = attr.value(Consts::colAttr).toInt();
        if(attr.hasAttribute(Consts::rowspanAttr))rowSpan = attr.value(Consts::rowspanAttr).toInt();
        if(attr.hasAttribute(Consts::colspanAttr))colSpan = attr.value(Consts::colspanAttr).toInt();
        if(vertical) newLayout = new QVBoxLayout();
        else newLayout = new QHBoxLayout();

        int spacing=ViewElement::getDefaultSpacing();
        if(attr.hasAttribute(Consts::spacingAttr)) spacing = attr.value(Consts::spacingAttr).toInt();
        int margin = spacing/3;
        newLayout->setSpacing(spacing);
        newLayout->setMargin(margin);

        QFrame *frame=new QFrame;
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Plain);
        frame->setLayout(newLayout);

        xmlLayouts.push(newLayout);
        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::gridlayoutTag)))
          xmlReadNext();
        xmlLayouts.pop();

        //if(!xmlGridList.empty()) xmlGridList.top()->addLayout(newLayout,row,col,rowSpan,colSpan,Qt::AlignLeft|Qt::AlignTop); //Qt::AlignCenter
        if(!xmlGrids.empty()) xmlGrids.top()->addWidget(frame,row,col,rowSpan,colSpan,Qt::AlignLeft|Qt::AlignTop); //Qt::AlignCenter
    }
}

void Hmi::xmlMakeTabs()
{
    ////////////////////
    //  набор вкладок
    ///////////////////

    if(xmlHMIfile->name() == Consts::tabsTag){
        QTabWidget *tabs = new QTabWidget();
        QTabBar *bar = tabs->tabBar();
        //bar->set
        xmlTabWidgets.push(tabs);
        qDebug()<<xmlTabCollections.count();
        if(!xmlTabCollections.empty()){
          xmlTabCollections.top()->connectTabWidget(tabs);
        }
        else qDebug()<<"xmlTabCollectionlist is empty -- error";
        qDebug()<< "";
        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::tabsTag)))
          xmlReadNext();

        xmlTabWidgets.pop();
        // !!! по умолчанию сделаем набор вкладок расширяемым до предела ???
        xmlLayouts.top()->addWidget(tabs,1);
    }
}

void Hmi::xmlMakeTab()
{
    ////////////////
    //   вкладка
    ///////////////

  // layout тут наверное не используется, т.к. тут делаются всего-лишь вкладки
    if(xmlHMIfile->name() == Consts::tabTag){
        QWidget *tab = new QWidget();

        QString cap = "tab";
        QBoxLayout *newLayout;
        int vertical=1;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(vertical) newLayout = new QVBoxLayout();
        else newLayout = new QHBoxLayout();

        int accessLvl=0;
        int viewLvl=0;
        if(attr.hasAttribute(Consts::accessLevelAttr))accessLvl = attr.value(Consts::accessLevelAttr).toInt();
        if(attr.hasAttribute(Consts::viewLevelAttr))viewLvl = attr.value(Consts::viewLevelAttr).toInt();

        newLayout->setSpacing(6);
        newLayout->setMargin(3);
        //newLayout->setAlignment(Qt::AlignLeft|Qt::AlignTop);

        ViewTabCollection *viewTab = new ViewTabCollection(); // создаём новую вкладку коллекции
        xmlTabCollections.push(viewTab); // и помещаем в стек

        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        tab->setLayout(newLayout);

        xmlLayouts.push(newLayout);
        while(!xmlHMIfile->hasError() && !xmlHMIfile->atEnd() &&
            !((xmlHMIfile->tokenType()==QXmlStreamReader::EndElement)&&(xmlHMIfile->name()==Consts::tabTag)))
          xmlReadNext();
        xmlLayouts.pop();

        if(!xmlTabWidgets.empty()>0){
            xmlTabWidgets.top()->addTab(tab, cap);

            if(hmiSecurity){
                // т.к. напрямую сигналы на вкладки завести не удастся, то делаем посредника TabWidgetSetEnabledSignalAdapter.
                QAction *act=new TabWidgetSetEnabledSignalAdapter(xmlTabWidgets.top(),xmlTabWidgets.top()->count()-1);
                hmiSecurity->SecureAccess(act,viewLvl,accessLvl);
            }
        }
        else qDebug()<<"TabWidget list is empty";

        xmlTabCollections.pop(); // когда вкладка заполнена, её можно забирать из стека, и запихивать в корневую вкладку коллекции.
        if(!xmlTabCollections.empty()) xmlTabCollections.top()->addTab(viewTab); // если тут возникает ошибка, значит главную коллекцию вьюшек правильно не инициализировали

    }
}

void Hmi::xmlMakeLabel()
{
    //////////////////////////
    //   создание подписи
    /////////////////////////

    QString cap;
    Qt::Alignment alignment=Qt::AlignCenter;
    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
    if(attr.hasAttribute(Consts::captionAlignmentAttr)){
        QString align;
        align=attr.value(Consts::captionAlignmentAttr).toString().toLower();
        if(align=="left") alignment=Qt::AlignLeft;
        else if (align=="right") alignment=Qt::AlignRight;
        else if (align=="top") alignment=Qt::AlignTop;
        else if (align=="bottom") alignment=Qt::AlignBottom;
        // else alignment=Qt::AlignCenter; // по умолчанию уже...
    }

    QLabel *lbl = new QLabel(cap);
    QFont fnt = ViewElement::getDefaultFont(); //lbl->font();

    // размер текста подписи
    int fs=fnt.pointSize()-1;
    if(attr.hasAttribute(Consts::fontSizeAttr)) fs=attr.value(Consts::fontSizeAttr).toInt();
    if(attr.hasAttribute(Consts::captionFontSizeAttr)) fs=attr.value(Consts::captionFontSizeAttr).toInt();
    fnt.setPointSize(fs);
    lbl->setFont(fnt);

    int h=-1;
    if(attr.hasAttribute(Consts::fixedHeightAttr)) h=attr.value(Consts::fixedHeightAttr).toInt();
    if(attr.hasAttribute(Consts::captionFixedHeightAttr)) h=attr.value(Consts::captionFixedHeightAttr).toInt();
    if(h>0) lbl->setFixedHeight(h);
    else lbl->setFixedHeight(ViewElement::getDefaultHeight());

    int w=-1;
    if(attr.hasAttribute(Consts::fixedWidthAttr)) w=attr.value(Consts::fixedWidthAttr).toInt();
    if(attr.hasAttribute(Consts::captionFixedWidthAttr)) w=attr.value(Consts::captionFixedWidthAttr).toInt();
    if(w>0) lbl->setFixedWidth(w);
    else lbl->setMargin(ViewElement::getDefaultSpacing()*0.5);

    xmlSecureWidget(lbl);

    xmlLayouts.top()->addWidget(lbl,0,alignment);
}

void Hmi::xmlMakeLedViewElement()
{
    //////////////////////////
    //   создание лампочки
    /////////////////////////

    if(xmlHMIfile->name() == Consts::ledviewTag){
        QString cap = "";//"led";
        QString regstr = "";
        int vertical = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();

        // создаём визуальный элемент на основани известных атрибутов
        // пути к картинкам берём из статического класса Consts.
        PlcRegister reg(regstr);
        //ViewElement
        LedView *element = new LedView(cap, reg, vertical);
        xmlCommonAttributes(element);

        bool inversed=false;
        if(attr.hasAttribute(Consts::inversedAttr)) inversed=attr.value(Consts::inversedAttr).toInt();
        if(attr.hasAttribute(Consts::colorAttr)){
            QColor colr=QColor(attr.value(Consts::colorAttr).toString());
            element->setColor(colr,inversed);
        }
        if((attr.hasAttribute(Consts::coloronAttr)) || (attr.hasAttribute(Consts::coloroffAttr))){
            QColor colOn("#777"),colOff("#777");
            if(attr.hasAttribute(Consts::coloronAttr)) colOn = QColor(attr.value(Consts::coloronAttr).toString());
            if(attr.hasAttribute(Consts::coloroffAttr)) colOff = QColor(attr.value(Consts::coloroffAttr).toString());
            element->setExactColors(colOn,colOff);
        }
        QString imagenameOn,imagenameOff;
        if(attr.hasAttribute(Consts::imageonAttr)) imagenameOn = attr.value(Consts::imageonAttr).toString();
        if(attr.hasAttribute(Consts::imageoffAttr)) imagenameOff = attr.value(Consts::imageoffAttr).toString();
        if(QFile(imagenameOn).exists() && QFile(imagenameOff).exists())
            element->setImages(imagenameOn,imagenameOn);

        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        xmlTabCollections.top()->addViewElement(reg, element);

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

void Hmi::xmlMakeLedButtonElement()
{
    ///////////////////////////
    //   создание кнопочки
    //////////////////////////

    if(xmlHMIfile->name() == Consts::ledbuttonTag){
        QString cap = "";//"button";
        QString regstr = "";//, colorOn = "gray", colorOff = "gray";
        QString textOn = Resources::translate("on");
        QString textOff = Resources::translate("off");
        int vertical = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::textonAttr))textOn = attr.value(Consts::textonAttr).toString();
        if(attr.hasAttribute(Consts::textoffAttr))textOff = attr.value(Consts::textoffAttr).toString();

//        if(attr.hasAttribute(Consts::coloronAttr))colorOn = attr.value(Consts::coloronAttr).toString();
//        if(attr.hasAttribute(Consts::coloroffAttr))colorOff = attr.value(Consts::coloroffAttr).toString();

        // создаём визуальный элемент на основани известных атрибутов
        // пути к картинкам берём из статического класса Consts.But then again he'd just w
        PlcRegister reg(regstr);
        LedButton *element = new LedButton(cap, reg, vertical);
        element->setTextOn(textOn);
        element->setTextOff(textOff);
        // Consts::buttonOffColorSkin.value(colorOff),Consts::buttonOnColorSkin.value(colorOn), - не знаю что с этим делать

        if(attr.hasAttribute(Consts::colorAttr)){
            QColor colr = QColor(attr.value(Consts::colorAttr).toString());
            element->setColor(colr);
        }
        if(attr.hasAttribute(Consts::confirmOnAttr)){
            QString str = attr.value(Consts::confirmOnAttr).toString();
            element->setConfirmOn(str);
        }
        if(attr.hasAttribute(Consts::confirmOffAttr)){
            QString str = attr.value(Consts::confirmOffAttr).toString();
            element->setConfirmOff(str);
        }

        // ??? надо ли делать проверку наличия данных в стеках? чтобы использование stack.top() не вызвало ошибку.
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого.
        xmlTabCollections.top()->addViewElement(reg, element);
        xmlCommonAttributes(element);

        // подключаем сигнал редактирования значения.
        QObject::connect(element, &ViewElement::valueChanged, hmiMasterz.top(), &RegisterIOInterface::updateRegisterValue);
        //QObject::connect(element, SIGNAL(valueChanged(PlcRegister, int)), this,
        //                          SLOT(updateRegisterValue(PlcRegister, int)));

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

void Hmi::xmlMakeNumButtonElement()
{
    ///////////////////////////
    //   создание кнопочки
    //////////////////////////

    if(xmlHMIfile->name() == Consts::numbuttonTag){
        QString cap = "";//"button";
        QString regstr = "";//, colorOn = "gray", colorOff = "gray";
        QString textOn = "";//Resources::translate("");
        QString textOff = "";//Resources::translate("");
        int vertical = 0;
        int theValue = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::valueAttr))theValue = attr.value(Consts::valueAttr).toInt();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::textonAttr))textOn = attr.value(Consts::textonAttr).toString();
        if(attr.hasAttribute(Consts::textoffAttr))textOff = attr.value(Consts::textoffAttr).toString();
        if(attr.hasAttribute(Consts::textAttr)){textOff = attr.value(Consts::textAttr).toString();textOn = attr.value(Consts::textAttr).toString();}

//        if(attr.hasAttribute(Consts::coloronAttr))colorOn = attr.value(Consts::coloronAttr).toString();
//        if(attr.hasAttribute(Consts::coloroffAttr))colorOff = attr.value(Consts::coloroffAttr).toString();

        // создаём визуальный элемент на основани известных атрибутов
        // пути к картинкам берём из статического класса Consts.But then again he'd just w
        PlcRegister reg(regstr);
        NumButton *element = new NumButton(cap, reg, theValue, vertical);
        element->setTextOn(textOn);
        element->setTextOff(textOff);
        // Consts::buttonOffColorSkin.value(colorOff),Consts::buttonOnColorSkin.value(colorOn), - не знаю что с этим делать

        if(attr.hasAttribute(Consts::colorAttr)){
            QColor colr = QColor(attr.value(Consts::colorAttr).toString());
            element->setColor(colr);
        }
        if(attr.hasAttribute(Consts::confirmOnAttr)){
            QString str = attr.value(Consts::confirmOnAttr).toString();
            element->setConfirmOn(str);
        }
        if(attr.hasAttribute(Consts::confirmOffAttr)){
            QString str = attr.value(Consts::confirmOffAttr).toString();
            element->setConfirmOff(str);
        }

        // ??? надо ли делать проверку наличия данных в стеках? чтобы использование stack.top() не вызвало ошибку.
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого.
        xmlTabCollections.top()->addViewElement(reg, element);
        xmlCommonAttributes(element);

        // подключаем сигнал редактирования значения.
        QObject::connect(element, &ViewElement::valueChanged, hmiMasterz.top(), &RegisterIOInterface::updateRegisterValue);
        //QObject::connect(element, SIGNAL(valueChanged(PlcRegister, int)), this,
        //                          SLOT(updateRegisterValue(PlcRegister, int)));

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

QColor Hmi::strToColor(QString colr)
{
/*
    if(colr=="black")return Qt::black;
    else if(colr=="blue")return Qt::darkBlue;
    else if(colr=="red")return Qt::darkRed;
    else if(colr=="green")return Qt::darkGreen;
    else if(colr=="gray")return Qt::darkGray;
    else if(colr=="cyan")return Qt::darkCyan;
    else if(colr=="magenta")return Qt::darkMagenta;
    else if(colr=="yellow")return Qt::darkYellow;
    else if(colr=="pink")return QColor(200,100,150);
    else if(colr=="orange")return QColor(200,100,0);
    else if(colr=="brown")return QColor(100,40,0);
    else return Qt::black;*/
    QColor color=QColor(colr);
    if(!color.isValid())color=Qt::black;
    return color;
}

void Hmi::xmlMakeStatusViewElement()
{
    ////////////////////////////////
    //   создание строки статуса
    ///////////////////////////////

    if(xmlHMIfile->name() == Consts::statusviewTag){
        QString cap = "";
        QString regstr = "";
        int vertical = 0;
        QList<QColor> colors;
        QList<QString> labelslist;
        QList<int> values;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::labelsAttr))
            foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
                labelslist<<lbl.trimmed();
        if(attr.hasAttribute(Consts::colorsAttr))
            foreach(QString str, attr.value(Consts::colorsAttr).toString().split(","))
                colors<<strToColor(str.trimmed());
        if(attr.hasAttribute(Consts::valuesAttr))
            foreach(QString str, attr.value(Consts::valuesAttr).toString().split(","))
                values<<str.trimmed().toInt();

        PlcRegister reg(regstr);
        // создаём визуальный элемент на основани известных атрибутов
        ViewElement *element = new StatusView(cap, reg, values, labelslist, colors, vertical);
        xmlCommonAttributes(element);
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        xmlTabCollections.top()->addViewElement(reg, element);

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

void Hmi::xmlMakeAlarmListElement()
{
    /////////////////////////////////////////////
    //   создание списка сообщений об ошибках
    ////////////////////////////////////////////

    if(xmlHMIfile->name() == Consts::alarmlistTag){
        QString cap = "";
        QList<PlcRegister> registerslist;
        QList<QColor> colors;
        QList<QString> labelslist;
        int rowcount=0;
        bool vertical=true;

        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::rowcountAttr))rowcount = attr.value(Consts::rowcountAttr).toInt();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::labelsAttr))
            foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
                labelslist<<lbl.trimmed();
        if(attr.hasAttribute(Consts::colorsAttr))
            foreach(QString str, attr.value(Consts::colorsAttr).toString().split(","))
                colors<<strToColor(str.trimmed());

/*        if(attr.hasAttribute(Consts::registersAttr))
            foreach(QString str, attr.value(Consts::registersAttr).toString().split(","))
                registerslist<<PlcRegister(str.trimmed());
*/

        if(attr.hasAttribute(Consts::registersAttr))
            foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
                if(str.contains("-")){
                    // вставить рассчёт диапозона регистров
                    QStringList strsplit=str.split("-");
                    if(strsplit.count()==2){
                        PlcRegister reg1=strsplit.at(0).trimmed(),
                                    reg2=strsplit.at(1).trimmed();
                        if(reg1.getType()==reg2.getType())
                            for(int i=0;i<=(reg2.getIndex()-reg1.getIndex());i++)
                            {
                                registerslist<<(reg1+i);
                                //qDebug()<<registerslist.last().toString();
                            }
                    }
                }
                else registerslist<<PlcRegister(str.trimmed());
            }

        // создаём визуальный элемент на основани известных атрибутов
        ViewElement *element = new AlarmList(cap, rowcount, registerslist, labelslist, colors, vertical);
        xmlCommonAttributes(element);
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        foreach(PlcRegister reg, registerslist.toSet()) // toSet чтобы исключить повторение регистров.
            xmlTabCollections.top()->addViewElement(reg, element);

        // и помещаем его на Layout. В случае совпадения направления expanding и layout, ставим stretch=1
        int stretch=0;
        if((xmlLayouts.top()->direction()==QBoxLayout::LeftToRight)&&(element->getWidth()==0))stretch=1;
        xmlLayouts.top()->addWidget(element, stretch);
    }

}

void Hmi::xmlMakeAlarmLog()
{
    /////////////////////////////////////////////
    //   создание лога сообщений об ошибках
    ////////////////////////////////////////////


    if(xmlHMIfile->name() == Consts::alarmLogTag){
        QString cap = "";
        QString name = "";
        QString dateformat = "";
        QList<PlcRegister> registerslist;
        QList<QString> labelslist;
        QList<QString> tooltipslist;
        bool vertical=true;

        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::nameAttr))name = attr.value(Consts::nameAttr).toString();
        if(attr.hasAttribute(Consts::dateformatAttr))dateformat = attr.value(Consts::dateformatAttr).toString();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();
        if(attr.hasAttribute(Consts::labelsAttr))
            foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
                labelslist<<lbl.trimmed();
        if(attr.hasAttribute(Consts::tooltipsAttr))
            foreach(QString lbl, attr.value(Consts::tooltipsAttr).toString().split(","))
                tooltipslist<<lbl.trimmed();

        if(attr.hasAttribute(Consts::registersAttr))
            foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
                if(str.contains("-")){
                    // вставить рассчёт диапозона регистров
                    QStringList strsplit=str.split("-");
                    if(strsplit.count()==2){
                        PlcRegister reg1=strsplit.at(0).trimmed(),
                                    reg2=strsplit.at(1).trimmed();
                        if(reg1.getType()==reg2.getType())
                            for(int i=0;i<=(reg2.getIndex()-reg1.getIndex());i++)
                            {
                                registerslist<<(reg1+i);
                                //qDebug()<<registerslist.last().toString();
                            }
                    }
                }
                else registerslist<<PlcRegister(str.trimmed());
            }


        // создаём визуальный элемент на основани известных атрибутов
        ViewElement *element = new AlarmLog(cap, name, registerslist, labelslist, tooltipslist, dateformat, vertical);
        xmlCommonAttributes(element);
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        foreach(PlcRegister reg, registerslist.toSet()) // toSet чтобы исключить повторение регистров.
            viewCollection->addViewElement(reg, element);

        // и помещаем его на Layout. В случае совпадения направления expanding и layout, ставим stretch=1
        int stretch=0;
        if((xmlLayouts.top()->direction()==QBoxLayout::TopToBottom)&&(element->getHeight()==0))stretch=1;
        if((xmlLayouts.top()->direction()==QBoxLayout::LeftToRight)&&(element->getWidth()==0))stretch=1;
        xmlLayouts.top()->addWidget(element, stretch);
    }
}

void Hmi::xmlMakeNumericViewElement()
{
    /////////////////////////
    //   создание циферки
    ////////////////////////

    if(xmlHMIfile->name() == Consts::numericviewTag){
        QString cap = "";//"numview";
        QString regstr = "";
        int digits = 0;
        int vertical = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::decimalsAttr))digits = attr.value(Consts::decimalsAttr).toInt();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();

        PlcRegister reg(regstr);
        // создаём визуальный элемент на основани известных атрибутов
        ViewElement *element = new NumericView(cap, reg, digits, vertical);
        xmlCommonAttributes(element);
        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        xmlTabCollections.top()->addViewElement(reg, element);

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

void Hmi::xmlMakeNumericEditElement()
{
  //////////////////////////////////////
 //    создание редактора циферки    //
//////////////////////////////////////

    if(xmlHMIfile->name() == Consts::numericeditTag){
        QString cap = "";//"numedit"
        QString regstr = "";
        int digits = 0, minimum = 0, maximum = 1000;
        int vertical = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::registerAttr))regstr = attr.value(Consts::registerAttr).toString();
        if(attr.hasAttribute(Consts::minAttr))minimum = attr.value(Consts::minAttr).toInt();
        if(attr.hasAttribute(Consts::maxAttr))maximum = attr.value(Consts::maxAttr).toInt();
        if(attr.hasAttribute(Consts::decimalsAttr))digits = attr.value(Consts::decimalsAttr).toInt();
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();

        PlcRegister reg(regstr);
        // создаём визуальный элемент на основани известных атрибутов
        ViewElement *element = new NumericEdit(cap, reg, minimum, maximum, digits, vertical);
        xmlCommonAttributes(element);

        // добавляем элемент в коллекцию, подключаем его сигнал изменения содержимого
        xmlTabCollections.top()->addViewElement(reg, element);

        // подключаем сигнал редактирования значения.
        QObject::connect(element, &ViewElement::valueChanged, hmiMasterz.top(), &RegisterIOInterface::updateRegisterValue);
        //QObject::connect(element, SIGNAL(valueChanged(PlcRegister, int)), this,
        //                          SLOT(updateRegisterValue(PlcRegister, int)));

        // и помещаем его на Layout.
        xmlLayouts.top()->addWidget(element);
    }
}

void Hmi::xmlAppendTextList()
{
    // добавляем список текстовых строк в коллекцию
    if(xmlHMIfile->name() == Consts::textListTag){
        QStringList list;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::textAttr))
            foreach(QString str, attr.value(Consts::textAttr).toString().split(","))
                list<<str.trimmed(); // надо ли в списке строк отсекать пробелы по краям?
        textLists<<list;
        qDebug()<<list;
    }
}

void Hmi::xmlAppendTypeList()
{
    // добавляем список типов ячеек таблиц в коллекцию
    if(xmlHMIfile->name() == Consts::typeListTag){
        QStringList types;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::typesAttr))
            foreach(QString str, attr.value(Consts::typesAttr).toString().split(","))
                types<<str.trimmed();
        typeLists<<types;
        qDebug()<<types;
    }
}

void Hmi::xmlMakeTableElement()
{
  //////////////////////////////////////
 //   создание редактора табличек    //
//////////////////////////////////////
   //  не закончено!!!   //
  ////////////////////////

    if(xmlHMIfile->name() == Consts::tableTag){
        QString cap = "";//"table";
        QList<QString> labelslist,celltypelist;
        QList<PlcRegister> validregisterslist;
        QList<int> minlist, maxlist,intervalslist;
        int rows=0;
        int vertical = 0;
        QXmlStreamAttributes attr = xmlHMIfile->attributes();
        if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
        if(attr.hasAttribute(Consts::rowcountAttr))rows = attr.value(Consts::rowcountAttr).toInt();
        if(attr.hasAttribute(Consts::labelsAttr))
            foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
                labelslist<<lbl.trimmed();
        if(attr.hasAttribute(Consts::registersAttr))
            foreach (QString reg, attr.value(Consts::registersAttr).toString().split(","))
                validregisterslist<<PlcRegister(reg.trimmed());
        if(attr.hasAttribute(Consts::regintervalsAttr))
            foreach (QString intr, attr.value(Consts::regintervalsAttr).toString().split(",")){
                int interval=intr.trimmed().toInt();
                if(interval<1)interval=1;
                intervalslist.append(interval);
            }
        if(attr.hasAttribute(Consts::minvaluesAttr))
            foreach (QString str, attr.value(Consts::minvaluesAttr).toString().split(","))
                minlist.append(str.trimmed().toInt());
        if(attr.hasAttribute(Consts::maxvaluesAttr))
            foreach (QString str, attr.value(Consts::maxvaluesAttr).toString().split(","))
                maxlist.append(str.trimmed().toInt());
        if(attr.hasAttribute(Consts::typesAttr))//celltypelist=attr.value(Consts::typesAttr).toString().split(",");
            foreach (QString str, attr.value(Consts::typesAttr).toString().split(","))
                celltypelist.append(str.trimmed()); // оставим на потом сложные перекомпоновки списков строк и типов
        if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();


        // создаём визуальный элемент на основани известных атрибутов

        TableDividedByColumns *table = new TableDividedByColumns(cap, rows, labelslist, validregisterslist,
                                                                 intervalslist, vertical);
        // чтобы в упростить себе жизнь списки строк передадим в виде указателя.
        table->addTextList(&textLists);
        table->addTypeList(&typeLists);
        table->setColumnTypes(minlist,maxlist,celltypelist);
        ViewElement *element = table;


        // !!!!!!!!!!!!!!!!!!!!!!!!!!
        // сделать: защита от выхода за пределы
        //      при задании списка параметров
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!

        xmlCommonAttributes(element);

        QList<PlcRegister> list;
        for(int col=0;col<validregisterslist.count();col++)
          for(int row=0;row<rows;row++)
              list.append(validregisterslist.at(col)+row*intervalslist.at(col));
        // воизбежание повторов приходится разделить формирование полного списка регистров и заполнение коллекции.
        foreach(PlcRegister reg, list.toSet())
            xmlTabCollections.top()->addViewElement(reg, element);

        QObject::connect(element, &ViewElement::valueChanged, hmiMasterz.top(), &RegisterIOInterface::updateRegisterValue);
        //connect(element, SIGNAL(valueChanged(PlcRegister, int)), SLOT(updateRegisterValue(PlcRegister, int)));

        // и помещаем его на Layout. В случае совпадения направления expanding и layout, ставим stretch=1
        int stretch=0;
        if((xmlLayouts.top()->direction()==QBoxLayout::TopToBottom)&&(element->getHeight()==0))stretch=1;
        if((xmlLayouts.top()->direction()==QBoxLayout::LeftToRight)&&(element->getWidth()==0))stretch=1;
        xmlLayouts.top()->addWidget(element, stretch);
    }
}

void Hmi::xmlMakeDiagram()
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
   //   создание простой диаграммы, показывающей состояние регистров за определённый промежуток времени //
  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    // !!! в процессе создания !!! //

/*    explicit DiagView(const QString &cap, int interval,
                      const QList<PlcRegister> &lineRegisters,const QList<int> &lineDecimals,
                        const QList<int> &lineMaxs,
                      const QList<QString> &lineLabels,
                    const QList<QColor> lineColors,
                      QObject *parent=0);
  */
    QString cap = "";//"diagram";
    int interval=1000;
    QList<PlcRegister> registerslist;
    QList<int> decimalslist, maxlist;
    QList<QString> labelslist;
    QList<QColor> colorslist;
    QColor backgroundColor = Qt::white;

    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
    if(attr.hasAttribute(Consts::intervalAttr))interval = attr.value(Consts::intervalAttr).toInt();
    if(attr.hasAttribute(Consts::registersAttr))
        foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
            registerslist<<PlcRegister(str.trimmed());
        }
    if(attr.hasAttribute(Consts::decimalsAttr))
        foreach(QString str, attr.value(Consts::decimalsAttr).toString().split(",")){
            decimalslist<<str.trimmed().toInt();
        }
    if(attr.hasAttribute(Consts::maxvaluesAttr))
        foreach(QString str, attr.value(Consts::maxvaluesAttr).toString().split(",")){
            maxlist<<str.trimmed().toInt();
        }
    if(attr.hasAttribute(Consts::labelsAttr))
        foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
            labelslist<<lbl.trimmed();
    if(attr.hasAttribute(Consts::colorsAttr)){
        foreach(QString str, attr.value(Consts::colorsAttr).toString().split(",")){
            colorslist<<strToColor(str.trimmed());
        }
        //qDebug()<<colorslist;
    }
    if(attr.hasAttribute(Consts::backgroundColorAttr))backgroundColor=strToColor(attr.value(Consts::backgroundColorAttr).toString().trimmed());
    int vertical=0;
    if(attr.hasAttribute(Consts::verticalAttr))vertical = attr.value(Consts::verticalAttr).toInt();


    // создаём визуальный элемент на основани известных атрибутов
    //ViewElement *element = new DiagView(cap, interval, registerslist, decimalslist, maxlist, labelslist, colorslist);
    ViewElement *element = new DiagViewDirectPaint(cap, interval, registerslist, decimalslist, maxlist, labelslist,
                                                   colorslist, backgroundColor, vertical);
    xmlCommonAttributes(element);
    foreach(PlcRegister reg, registerslist.toSet())
        viewCollection->addViewElement(reg, element);

    // и помещаем его на Layout. В случае совпадения направления expanding и layout, ставим stretch=1
    int stretch=0;
    if((xmlLayouts.top()->direction()==QBoxLayout::TopToBottom)&&(element->getHeight()==0))stretch=1;
    if((xmlLayouts.top()->direction()==QBoxLayout::LeftToRight)&&(element->getWidth()==0))stretch=1;
    xmlLayouts.top()->addWidget(element, stretch);
}

void Hmi::xmlMakeDiagramLog()
{
    //////////////////////////////////
   //   создание графического лога //
  //////////////////////////////////

    // !!! в процессе создания !!! //

    QString cap = "";
    QString name = "diagram log";
    int interval=1000;
    QList<PlcRegister> registerslist;
    QList<int> decimalslist, maxlist;
    QList<QString> labelslist;
    QList<QColor> colorslist;
    QColor backgroundColor = Qt::white;
    QString description;

    QXmlStreamAttributes attr = xmlHMIfile->attributes();
    if(attr.hasAttribute(Consts::nameAttr))name = attr.value(Consts::nameAttr).toString();
    if(attr.hasAttribute(Consts::captionAttr))cap = attr.value(Consts::captionAttr).toString();
    if(attr.hasAttribute(Consts::descriptionAttr))
        description = attr.value(Consts::descriptionAttr).toString();
    else
        description = title+" "+cap+" "+name; // пока вот так. Если автор xml не указал description, то из этого нагромождения будет хоть чуть-чуть понятно к чему был этот график.
    if(attr.hasAttribute(Consts::intervalAttr))interval = attr.value(Consts::intervalAttr).toInt();
    if(attr.hasAttribute(Consts::registersAttr))
        foreach(QString str, attr.value(Consts::registersAttr).toString().split(",")){
            registerslist<<PlcRegister(str.trimmed());
        }
    if(attr.hasAttribute(Consts::decimalsAttr))
        foreach(QString str, attr.value(Consts::decimalsAttr).toString().split(",")){
            decimalslist<<str.trimmed().toInt();
        }
    if(attr.hasAttribute(Consts::maxvaluesAttr))
        foreach(QString str, attr.value(Consts::maxvaluesAttr).toString().split(",")){
            maxlist<<str.trimmed().toInt();
        }
    if(attr.hasAttribute(Consts::labelsAttr))
        foreach(QString lbl, attr.value(Consts::labelsAttr).toString().split(","))
            labelslist<<lbl.trimmed();
    if(attr.hasAttribute(Consts::colorsAttr)){
        foreach(QString str, attr.value(Consts::colorsAttr).toString().split(",")){
            colorslist<<strToColor(str.trimmed());
        }
    }
    if(attr.hasAttribute(Consts::backgroundColorAttr))backgroundColor=strToColor(attr.value(Consts::backgroundColorAttr).toString().trimmed());


    // создаём визуальный элемент на основани известных атрибутов
    ViewElement *element = new DiagramLog(cap, name, description, interval, registerslist, decimalslist, maxlist, labelslist,
                                          colorslist, backgroundColor);
    xmlCommonAttributes(element);
    foreach(PlcRegister reg, registerslist.toSet())
        viewCollection->addViewElement(reg, element);

    // и помещаем его на Layout. В случае совпадения направления expanding и layout, ставим stretch=1
    int stretch=0;
    if((xmlLayouts.top()->direction()==QBoxLayout::TopToBottom)&&(element->getHeight()==0))stretch=1;
    if((xmlLayouts.top()->direction()==QBoxLayout::LeftToRight)&&(element->getWidth()==0))stretch=1;
    xmlLayouts.top()->addWidget(element, stretch);
}
