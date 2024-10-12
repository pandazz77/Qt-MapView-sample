#include <QApplication>

#include "MainWindow.h"

#define QT_FATAL_WARNINGS 

int main(int argc, char *argv[]){
    QApplication app(argc,argv);

    MainWindow *mw = new MainWindow();
    mw->show();

    mw->mapView->addLayer(new TileLayer("http://t2.openseamap.org/tile/{z}/{x}/{y}.png"));
    mw->mapView->addLayer(new TileLayer("http://tiles.openseamap.org/seamark/{z}/{x}/{y}.png"));

    mw->mapView->setCamera(30.3223,59.9292,12);
    
    return app.exec();
}