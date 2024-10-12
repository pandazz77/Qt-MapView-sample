#pragma once

#include <QMainWindow>

#include "ui_MainWindow.h"
#include "mapview.h"


namespace Ui{
    class MainWindow;
}

class MainWindow : public QMainWindow{
    Q_OBJECT
    
    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

        MapGraphicsView *mapView;

    private slots:
        void setLonLatInfo(double lon, double lat);
        void setZoomInfo(double zoom);

    private:
        Ui::MainWindow *ui;
};