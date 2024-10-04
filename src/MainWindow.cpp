#include <QGraphicsPixmapItem>
#include <QLabel>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    mapView = ui->mapView;

    connect(mapView,&MapGraphicsView::cameraChanged,this,&MainWindow::setMapviewInfo);
    auto cam = mapView->getCamera();
    setMapviewInfo(cam.lon,cam.lat,cam.zoom);
};

void MainWindow::setMapviewInfo(double lon, double lat, double zoom){
    ui->lonLabel->setText(QString::number(lon));
    ui->latLabel->setText(QString::number(lat));
    ui->zoomLabel->setText(QString::number(zoom));
}

MainWindow::~MainWindow(){

}