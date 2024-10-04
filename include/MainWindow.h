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
        void setMapviewInfo(double lon, double lat, double zoom);

    private:
        Ui::MainWindow *ui;
};