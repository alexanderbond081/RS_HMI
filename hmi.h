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

#ifndef HMI_H
#define HMI_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include "plcregistervalidator.h"
#include "plcregister.h"
#include "viewelement.h"
#include "viewtabcollection.h"
#include "datacollection.h"
#include "resources.h"
#include "datasaveload.h"
#include "hmisecurity.h"
#include "diagramlog.h"
#include "poweroff.h"
#include "registerIOinterface.h"
#include "dataeditor.h"

/*
    Класс занимается созданием HMI на основании XML файла и распихиванием соединений, чтобы данные корректно отображались и обновлялись.
    Помещает визуальные элементы в QWidget, указатель на который можно получить и разместить на форме или отобразить как есть.
    Вместо QWidget была попытка использовать QLayout, но без бубна не получается сменить родителя всех размещённых на лейауте объектов,
    что необходимо для корректного обновления/перемещения/удаления HMI (возможно это и не будет использоваться, но возможность такая должна быть.)

    !!!
    Для лучшей кастомизации надо бы сделать:
    -убрать зависимость от resources.h - добавить setDefaultDirectory
    -картинки визуальных элементов будут искаться в "defaultDirectory/название-элемента/название-цвета.png"

*/

class Hmi : public RegisterIOInterface
{
    Q_OBJECT

private:
    QPointer<QWidget> hmiWidget; // т.к. hmiWidget может быть уничтожен родительским QWidget
    PowerOff *poweroff=nullptr;
    HmiSecurity *hmiSecurity=nullptr;
    ViewTabCollection *viewCollection=nullptr;
    QMap<PlcRegister, QPointer<ViewElement>> actualRegViewMap;

    QString title;
    // набор списков строк для использования в выпадающих списках, таблицах, сообщениях об ошибках и т.п.
    QList<QStringList> textLists, typeLists;

    // делегированные хранилище данных и валидатор имён регистров
    QPointer<DataCollection> registerDataCollection;
    QPointer<PLCRegisterValidator> registerValidator;

    QList<QPointer<QMenu>> hmiMenuList; // Менюшки для вставки в mainwindow::menubar. QPointer не нужен, т.к. эти менюшки будут детьми hmiWidget, и нужны будут только 1 раз.
    QList<RecipeStorage*> hmiRecipeStorageList;
    QList<RecipeEditorWindow*> hmiRecipeEditorWindowsList;

    // вспомогательные поля для загрузки HMI из XML    
    QXmlStreamReader *xmlHMIfile;
    QStack<QMenu*> xmlMenus; // стек менюшек. Для создания иерархии менюшек при чтении xml.
    QStack<QBoxLayout*> xmlLayouts; // стек лейаутов.
    QStack<QGridLayout*> xmlGrids; // стек гридов.
    QStack<QTabWidget*> xmlTabWidgets; // стек виджетов управления вкладками.
    QStack<ViewTabCollection*> xmlTabCollections; // стек коллекций визуальных элементов, соответствующий вкладкам.
    QStack<RegisterIOInterface*> hmiMasterz; // это надо только для того чтобы знать куда сигналы от ViewElement'ов присоединять.

    QColor strToColor(QString colr);
    // методы создания интерфейса (HMI) из XML файла.
    bool xmlReadNext();
    void xmlStartProgramHMI();
    void xmlCommonAttributes(ViewElement *element);

    void xmlPowerOff();

    void xmlSecurity();
    void xmlSecureWidget(QWidget *widget);
    void xmlSecureAction(QAction *action);

    void xmlRecipeStorage();
    void xmlRecipeEditor();
    void xmlMakeTextLog();

    void xmlMenu();
    void xmlMenuAction();

    void xmlMakeLayout();
    void xmlMakeSpacer();
    void xmlMakeStretch();
    void xmlMakeGrid();
    void xmlMakeGridLayout();
    void xmlMakeTabs();
    void xmlMakeTab();
    void xmlMakeLabel();
    void xmlMakeLedViewElement();
    void xmlMakeLedButtonElement();
    void xmlMakeNumButtonElement();
    void xmlMakeStatusViewElement();
    void xmlMakeAlarmListElement();
    void xmlMakeAlarmLog();
    void xmlMakeNumericViewElement();
    void xmlMakeNumericEditElement();
    void xmlAppendTextList();
    void xmlAppendTypeList();
    void xmlMakeTableElement();
    void xmlMakeDiagram();
    void xmlMakeDiagramLog();

public:
    explicit Hmi(QObject *parent = nullptr);
    virtual ~Hmi();

    void updateRegisterValue(const PlcRegister &reg, int val) override;

    void setRegisterData(DataCollection *data);
    void setValidator(PLCRegisterValidator *validator);
    void loadFromXml(const QString &filename);
    void clear();

    QString getTitle() const;
    QWidget* getWidget();
    QList<QMenu*> getMenuList();

private slots:
    void onHmiWidgetDelete();
    // void onRegisterChangedByView(PlcRegister reg, int val);
    void onViewCollectionTabUpdated();
    void onRegisterDataUpdated();


public slots:
    void setWaitCursor();
    void setNormalCursor();

signals:

public slots:
};

#endif // HMI_H
