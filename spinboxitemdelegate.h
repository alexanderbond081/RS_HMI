#ifndef SPINBOXITEMDELEGATE_H
#define SPINBOXITEMDELEGATE_H

#include <QObject>
#include <QItemDelegate>
#include <QDoubleSpinBox>

class SpinboxItemDelegate : public QItemDelegate
{
    Q_OBJECT
private:
    double minVal, maxVal;
    int DecDigits;

public:
    explicit SpinboxItemDelegate(double minValue, double maxValue, int decimalDigits, QObject *parent=0);

    // QAbstractItemDelegate interface

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // SPINBOXITEMDELEGATE_H
