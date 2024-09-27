#include <QGraphicsPixmapItem>
#include <QLabel>

#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    mapView = ui->mapView;

    connect(ui->camLat,&QDoubleSpinBox::valueChanged,[=](double value){ mapView->view->cam.lat = value; });
    connect(ui->camLon,&QDoubleSpinBox::valueChanged,[=](double value){ mapView->view->cam.lon = value; });
    connect(ui->camZoom,&QSpinBox::valueChanged,[=](int value){ mapView->view->cam.zoom = value; });
    connect(ui->renderBtn,&QPushButton::clicked,this,&MainWindow::onRenderClicked);

    // init values
    mapView->view->cam.lat = ui->camLat->value();
    mapView->view->cam.lon = ui->camLon->value();
    mapView->view->cam.zoom = ui->camZoom->value();
};

void MainWindow::onRenderClicked(){
    mapView->clearTiles();
    mapView->renderTiles();
}

MainWindow::~MainWindow(){

}