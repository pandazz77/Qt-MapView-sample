#include <QApplication>

#include "MainWindow.h"

#define QT_FATAL_WARNINGS 

int main(int argc, char *argv[]){
    QApplication app(argc,argv);

    MainWindow *mw = new MainWindow();
    mw->show();

    mw->mapView->addTileLayer(new TileLayer("http://t2.openseamap.org/tile/{z}/{x}/{y}.png"));
    mw->mapView->renderTiles();
    
    return app.exec();
}