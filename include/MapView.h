#pragma once

#include <QGraphicsView>
#include <QWidget>

#include "MapViewCore.h"

#include <math.h>

#ifndef NDEBUG
    #define MAPVIEW_DEBUG
#endif

using Camera = LonLatZoom;

Point mercatorProject(LonLat pos);
LonLat mercatorUnproject(Point pos);
Point3D lonlat2tile(LonLatZoom pos);
LonLatZoom tile2lonlat(Point3D pos);

Point _lonlat2scenePoint(LonLat pos, int mapWidth, int mapHeight);
LonLat _scenePoint2lonLat(Point scenePoint, int mapWidth, int mapHeight);

Point lonlat2scenePoint(LonLatZoom pos, int tileSize=256);
LonLatZoom scenePoint2lonLat(Point scenePoint, int zoom, int tileSize=256);

class MapGraphicsView: public QGraphicsView{
    Q_OBJECT

    public:
        MapGraphicsView(QWidget *parent = nullptr);
        ~MapGraphicsView();

        void setCamera(Camera cam);
        void setCamera(double lon, double lat, double zoom);
        Camera getCamera();

        void addLayer(ILayer *layer);

    protected:

        void resizeEvent(QResizeEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

        #ifdef MAPVIEW_DEBUG
            QGraphicsLineItem *camHLine;
            QGraphicsLineItem *camVLine;
        #endif

    signals:
        void lonLatChanged(double lon, double zoom);
        void zoomChanged(double zoom);
        void sizeChanged(int widht, int height);

    private:
        Camera cam = Camera(-3,40,7);
        QPointF previousP;
        QVector<ILayer*> layers;

    private slots:
        void onLonLatChanged();
        void onZoomChanged();

    public slots:
        void addItem(QGraphicsItem *item);
};