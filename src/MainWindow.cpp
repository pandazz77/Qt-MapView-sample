#include <QGraphicsPixmapItem>
#include <QLabel>

#include "MainWindow.h"
#include "core.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    mapView = ui->mapView;

    connect(ui->camLat,&QDoubleSpinBox::valueChanged,[=](double value){ mapView->cam.lat = value; });
    connect(ui->camLon,&QDoubleSpinBox::valueChanged,[=](double value){ mapView->cam.lon = value; });
    connect(ui->camZoom,&QSpinBox::valueChanged,[=](int value){ mapView->cam.zoom = value; });
    connect(ui->renderBtn,&QPushButton::clicked,this,&MainWindow::onRenderClicked);

    // init values
    mapView->cam.lat = ui->camLat->value();
    mapView->cam.lon = ui->camLon->value();
    mapView->cam.zoom = ui->camZoom->value();
};

void MainWindow::onRenderClicked(){
    mapView->clearTiles();
    mapView->renderTiles();
}

MainWindow::~MainWindow(){

}