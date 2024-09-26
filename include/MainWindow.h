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

        MapView *mapView;

    public slots:
        void onRenderClicked();

    private:
        Ui::MainWindow *ui;
};