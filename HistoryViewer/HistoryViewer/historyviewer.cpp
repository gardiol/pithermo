#include "historyviewer.h"
#include "ui_historyviewer.h"

#include <QFileDialog>
#include <QDebug>
#include <qcustomplot.h>

HistoryViewer::HistoryViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HistoryViewer)
{
    ui->setupUi(this);
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
}

HistoryViewer::~HistoryViewer()
{
    delete ui;
}

void HistoryViewer::openFile()
{
    _history_file = QFileDialog::getOpenFileName(this,
         tr("Open History"), "", tr("History Files (*.*)"));

    bool first = true;
    uint64_t min_t, max_t;
    double min_y, max_y;
    FILE* hf = fopen( _history_file.toStdString().c_str(), "rb");
    if ( hf != NULL )
    {
        while ( !feof(hf) )
        {
            HistoryItem* item = new HistoryItem( hf );
            _items.push_back( item );
            if ( first )
            {
                min_t = max_t = item->getTime();
                min_y = max_y = item->getTemp();
                first = false;
            }
            else
            {
                if ( min_t > item->getTime() )
                    min_t = item->getTime();
                if ( max_t < item->getTime() )
                    max_t = item->getTime();
                if ( min_y > item->getTemp() )
                    min_y = item->getTemp();
                if ( max_y < item->getTemp() )
                    max_y = item->getTemp();
            }
            _x.append( item->getTime() );
            _y.append( item->getTemp() );
        }
    }
    qDebug() << "Done: " << _items.size();

    ui->graph->addGraph();
    ui->graph->graph(0)->setData( _x, _y);
    ui->graph->xAxis->setRange( min_t,  max_t);
    ui->graph->yAxis->setRange(min_y, max_y);
    ui->graph->replot();
}
