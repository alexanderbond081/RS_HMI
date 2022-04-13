#ifndef RECIPEEDITOR_H
#define RECIPEEDITOR_H

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

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "datacollection.h"
#include "plcregister.h"
#include "viewelement.h"
#include "registerIOinterface.h"
#include <QPlatformSurfaceEvent>


class RecipeEditorWindow : public RegisterIOInterface
{
    Q_OBJECT

public:
    explicit RecipeEditorWindow(const QString &name, const QList<PlcRegister> &registers, DataCollection *data, QObject *parent = nullptr);
    // void setLayout(QLayout *layout);
    QLayout *getEditorLayout();
    void setViewElementsMap(const QMap<PlcRegister, QPointer<ViewElement>> &map);

    void updateRegisterValue(const PlcRegister &reg, int val) override;

private:
    QList<PlcRegister> regs; // список регистров. данные нам хранить не нужно.
    QMap<PlcRegister, int> internal_collection; // QList<PlcRegister> regs; // внутреннее хранилище данных.
    QMap<PlcRegister, QPointer<ViewElement>> elements_map; // тут должны лежать указатели на элементы, в которые надо засовывать новые значения регистров...
    QPointer<DataCollection> collection; // указатель на хранилище данных.
    QString caption;
    QString path; // путь определяемый name
    QWidget editor_window;
    QLayout *editor_layout;

    QTimer loadFromPLCTimer;
    QTime loadFromPLCTimeout;
    void UpdateEditorValues();

protected:
    bool eventFilter(QObject*, QEvent* event) override;// нужен чтобы отфильтровать БАГ с QEvent::PlatformSurface в момент закрытия окна (которого небыло еще в начале 2021, и обнаруженного осенью того же года)

private slots:
    void onLoadFromPLCTimer();

signals:
    void loadFromPLCStart();
    void loadFromPLCFinish();

public slots:
    void show();
    void hide();

    void loadFromPLC();
    void saveToPLC();
    void loadFromFile();
    void saveToFile();

};

#endif // RECIPEEDITOR_H
