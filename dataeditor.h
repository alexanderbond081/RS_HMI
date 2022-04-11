#ifndef RECIPEEDITOR_H
#define RECIPEEDITOR_H

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
