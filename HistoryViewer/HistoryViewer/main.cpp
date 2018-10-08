#include "historyviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HistoryViewer w;
    w.show();

    return a.exec();
}
