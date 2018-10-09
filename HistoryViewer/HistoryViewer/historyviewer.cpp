#include "historyviewer.h"
#include "ui_historyviewer.h"

#include <QFileDialog>
#include <QDebug>
#include <qcustomplot.h>
#include <QTableWidget>
#include <QTableWidgetItem>

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
            if ( item->isValid() )
            {
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
            else
                delete item;
        }
    }
    ui->points->setRowCount( _items.size() );

    int row = 0;
    foreach ( HistoryItem* item, _items )
    {
        QDateTime when;
        when.setSecsSinceEpoch(item->getTime());
        QTableWidgetItem *table_item_time = new QTableWidgetItem( when.toString() );
        QTableWidgetItem *table_item_temp = new QTableWidgetItem( QString("%1").arg( item->getTemp() ) );
        QTableWidgetItem *table_item_etemp = new QTableWidgetItem( QString("%1").arg( item->getExtTemp() ) );
        QTableWidgetItem *table_item_humi = new QTableWidgetItem( QString("%1").arg( item->getHumidity() ) );
        QTableWidgetItem *table_item_ehumi = new QTableWidgetItem( QString("%1").arg( item->getExtHumidity() ) );
        ui->points->setItem(row, 0, table_item_time);
        ui->points->setItem(row, 1, table_item_temp);
        ui->points->setItem(row, 2, table_item_etemp);
        ui->points->setItem(row, 3, table_item_humi);
        ui->points->setItem(row++, 4, table_item_ehumi);
    }
    qDebug() << "Done: " << _items.size();

/*    ui->graph->addGraph();
    ui->graph->graph(0)->setData( _x, _y);
    ui->graph->xAxis->setRange( min_t,  max_t);
    ui->graph->yAxis->setRange(min_y, max_y);
    ui->graph->replot();*/
}
