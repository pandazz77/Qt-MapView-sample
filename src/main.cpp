#include <QApplication>

#include "MainWindow.h"

#define QT_FATAL_WARNINGS 

int main(int argc, char *argv[]){
    QApplication app(argc,argv);

    MainWindow *mw = new MainWindow();
    mw->show();

    mw->mapView->addTileLayer(new TileLayer("https://t2.openseamap.org/tile/{z}/{x}/{y}.png"));
    mw->onRenderClicked(); // fire on startup
    
    return app.exec();
}