#include <QFileDialog>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include "resources.h"
#include "dataeditor.h"

RecipeEditorWindow::RecipeEditorWindow(const QString &name, const QList<PlcRegister> &registers, DataCollection *data, QObject *parent) : RegisterIOInterface(parent)
{
    caption=name;
    path=Consts::workDirPath+"/"+name;
    QDir dir;
    dir.mkpath(path);
    path+="/"+name+".dat";
    // спутями разобрались, а как сделать имя файла по умолчанию? и как его запоминать?

    foreach(PlcRegister reg, registers)
        if(reg.isValid())regs<<reg; // Проверку делать всё-таки надо, а то насохраняет битых регистров в файл.
    collection=data;
    connect(&loadFromPLCTimer,SIGNAL(timeout()),this,SLOT(onLoadFromPLCTimer()));

    QPushButton *b_to_PLC = new QPushButton(Resources::translate("to PLC"));
    QPushButton *b_from_PLC = new QPushButton(Resources::translate("from PLC"));
    QPushButton *b_save = new QPushButton(Resources::translate("save"));
    QPushButton *b_load = new QPushButton(Resources::translate("load"));
    QPushButton *b_hide = new QPushButton(Resources::translate("close"));
    connect(b_to_PLC, &QPushButton::clicked, this, &RecipeEditorWindow::saveToPLC);
    connect(b_from_PLC, &QPushButton::clicked, this, &RecipeEditorWindow::loadFromPLC);
    connect(b_save, &QPushButton::clicked, this, &RecipeEditorWindow::saveToFile);
    connect(b_load, &QPushButton::clicked, this, &RecipeEditorWindow::loadFromFile);
    connect(b_hide, &QPushButton::clicked, this, &RecipeEditorWindow::hide);

    qDebug()<<"butt layout";
    QLayout *butt_layout = new QHBoxLayout();
    butt_layout->setMargin(6);
    butt_layout->setSpacing(12);
    butt_layout->addWidget(b_to_PLC);
    butt_layout->addWidget(b_from_PLC);
    butt_layout->addWidget(b_save);
    butt_layout->addWidget(b_load);
    butt_layout->addWidget(b_hide);

    qDebug()<<"main layout create";
    QLayout *main_layout = new QVBoxLayout();

    qDebug()<<"main layout additem";
    editor_layout = new QVBoxLayout();
    QWidget *editor_widget = new QWidget(&editor_window);
    editor_widget->setLayout(editor_layout);

    main_layout->addWidget(editor_widget);
    main_layout->addItem(butt_layout);

    qDebug()<<"window set layout";
    editor_window.setLayout(main_layout);

    qDebug()<<"setting name";
    editor_window.setWindowTitle(name);

    editor_window.setAttribute(Qt::WA_DeleteOnClose, false);
    editor_window.setAttribute(Qt::WA_QuitOnClose, false);
    editor_window.installEventFilter(this); // нужен чтобы отфильтровать БАГ с QEvent::PlatformSurface
}

void RecipeEditorWindow::onLoadFromPLCTimer()
{
    // ??? это будет работать ???
    // ждём пока регистры прочитаются
    if(collection->getReadOnceCount()>0){
        if(QTime::currentTime() > loadFromPLCTimeout){
            loadFromPLCTimer.stop();
            emit loadFromPLCFinish();
                editor_window.setCursor(Qt::ArrowCursor);
            qDebug()<<"data load from PLC timeout - ";

            // и тут надо вывести объявление на экран, что сохранить не удалось!!!
            QMessageBox::warning(nullptr,Resources::translate("save failed"),
                                 Resources::translate("plc connection error")); // "Ошибка сохранения данных","Нет связи с ПЛК."
        }
        if(!DEBUG_TEST) return; // игнорируем ошибку, если это дебаг, и тупо грузим что есть.
    }

    // а теперь сохраняем
    loadFromPLCTimer.stop();
    qDebug()<<"загрузка из PLC в хранилище данных редактора";

    //    foreach(PlcRegister reg, internal_collection.keys()){
    //      internal_collection.insert(reg,collection->getValue(reg));
    //    }  // вместо понятного, но не оптимального рекомендуется использовать вот этот ||| код
    //         (т.к. QMap::keys() порождает временный объект)                            VVV
    for(auto it = internal_collection.cbegin(), end=internal_collection.cend(); it!=end; ++it){
        internal_collection.insert(it.key(),collection->getValue(it.key()));
    }

    emit loadFromPLCFinish();
        editor_window.setCursor(Qt::ArrowCursor);
}

void RecipeEditorWindow::show()
{
    if(editor_window.isVisible()){
        if(editor_window.isMinimized())editor_window.showNormal();
        editor_window.raise();
    }
    else editor_window.show();
}

void RecipeEditorWindow::hide()
{
    editor_window.hide();
}

QLayout *RecipeEditorWindow::getEditorLayout()
{
    return editor_layout;
}

void RecipeEditorWindow::setViewElementsMap(const QMap<PlcRegister, QPointer<ViewElement> > &map)
{
    elements_map = map;
}

void RecipeEditorWindow::UpdateEditorValues()
{
        foreach (PlcRegister reg, regs){
            // обновление вьюшки. Одному ключу(регистру) может соответствовать несколько вьюшек.
            foreach(ViewElement *view, elements_map.values(reg)){
                view->setValue(internal_collection.value(reg, 0), reg);
            }
        }
}

bool RecipeEditorWindow::eventFilter(QObject *, QEvent *event)
{
    if(event->type() == QEvent::PlatformSurface){
        QPlatformSurfaceEvent *eve = (QPlatformSurfaceEvent *) event;
        qDebug()<<"stop this event to prevent BUG"<<QEvent::PlatformSurface<<":"<<eve->surfaceEventType();
        return true;
    }
    return false;
}

void RecipeEditorWindow::loadFromPLC()
{
    if(collection==nullptr) return;

    // для того чтобы записать регистры в файл, сначала нужно прочитать их из контроллера
    collection->addToReadOnceList(regs);
    // получаем имя файла
    loadFromPLCTimeout=QTime::currentTime().addSecs(3);
    loadFromPLCTimer.start(100);
    emit loadFromPLCStart();
        editor_window.setCursor(Qt::BusyCursor);
}


void RecipeEditorWindow::saveToPLC()
{
    collection->submitRegisterValues(internal_collection);
}


void RecipeEditorWindow::loadFromFile()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, Resources::translate("load")+" "+caption, path, "*.dat");
    if(filename.isEmpty())return;

    QMap<PlcRegister, int> data;
    QFile *file = new QFile(filename);
    if(!file->open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Can't open file"; //"Ошибка загрузки файла";
        return;
    }
    QTextStream textstream(file);
    QList<PlcRegister> expectedRegs=regs;
    while(!textstream.atEnd()){
        QStringList line=textstream.readLine().split("\t"); // QChar(9)
        PlcRegister reg(line.at(0));
        int val=0;
        if(line.count()>1) val=line.at(1).toInt();
        if(expectedRegs.contains(reg)){
            expectedRegs.removeOne(reg);
            data.insert(reg,val); // проверяем наличие регистра в списке. Чтобы не насувать лишнего.
        }
    }

    // Добавить анализ все ли регистры из ожидаемого списка были прочитаны
    if(expectedRegs.count()>0){
        // сообщаем о том что файл не полный.
        if(QMessageBox::question(nullptr,Resources::translate("warning"),
                                 Resources::translate("file corrupted warning"))!=QMessageBox::Yes) // "Файл содержит не полные данные или повреждён. Загрузить не полные данные?"
            return;
    }
    internal_collection.unite(data);
    path=filename;

    UpdateEditorValues();
}


void RecipeEditorWindow::saveToFile()
{
    QFileDialog savedialog;
    savedialog.setDefaultSuffix(".dat");
    QString filename = savedialog.getSaveFileName(nullptr, Resources::translate("save")+" "+caption, path, "*.dat");
    if(filename.isEmpty())return; // была нажата "отмена"
    path=filename; // ??? это надо чтобы запомнить новое расположение папки с файлами для следующего открытия?

    qDebug()<<filename;
    //QMap<PlcRegister, int> data;
    QFile *file = new QFile(filename);
    if(!file->open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug()<<"Can't open file";//"Ошибка открытия файла";
        QMessageBox::warning(nullptr,Resources::translate("save failed"),
                             Resources::translate("can't open file")); // "Ошибка сохранения данных","Невозможно открыть файл."
        return;
    }
    QTextStream textstream(file);
    foreach(PlcRegister reg, regs){
        textstream<<reg.toString()+"\t"+QString().number(internal_collection.value(reg))+"\n";
    }
    file->close();
    delete file;
}

  // //////// ///////// //
 //   overriden slot   //
// ///////// //////// //

void RecipeEditorWindow::updateRegisterValue(const PlcRegister &reg, int val)
{
    if(reg.isValid())
        internal_collection.insert(reg, val);
}
