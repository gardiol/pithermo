#ifndef HISTORYVIEWER_H
#define HISTORYVIEWER_H

#include <QMainWindow>

#include <QFile>

#include "../../daemon/src/historyitem.h"

namespace Ui {
class HistoryViewer;
}

class HistoryViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit HistoryViewer(QWidget *parent = 0);
    ~HistoryViewer();

public slots:
    void openFile();

private:
    Ui::HistoryViewer *ui;

    QString _history_file;

    QList<HistoryItem*> _items;

    QVector<double> _x;
    QVector<double> _y;

};

#endif // HISTORYVIEWER_H
