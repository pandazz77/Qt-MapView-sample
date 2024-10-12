#include <QGraphicsPixmapItem>
#include <QLabel>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    mapView = ui->mapView;

    connect(mapView,&MapGraphicsView::lonLatChanged,this,&MainWindow::setLonLatInfo);
    connect(mapView,&MapGraphicsView::zoomChanged,this,&MainWindow::setZoomInfo);
    auto cam = mapView->getCamera();
    
    setLonLatInfo(cam.lon,cam.lat);
    setZoomInfo(cam.zoom);
};

void MainWindow::setLonLatInfo(double lon, double lat){
    ui->lonLabel->setText(QString::number(lon));
    ui->latLabel->setText(QString::number(lat));
}

void MainWindow::setZoomInfo(double zoom){
    ui->zoomLabel->setText(QString::number(zoom));
}

MainWindow::~MainWindow(){

}