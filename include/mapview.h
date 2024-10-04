#pragma once

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>

#include <math.h>

#define TILE_SIZE 256
#define WEBMERCATOR_R 6378137.0
#define DIAMETER (WEBMERCATOR_R * 2 * M_PI)

#ifndef NDEBUG
    #define MAPVIEW_DEBUG
#endif

class Point{
    public:
        double x,y;
        Point(double x, double y): x(x), y(y) {};
        Point &operator=(const Point & other){
            x = other.x;
            y = other.y;
            return *this;
        }
};

class Point3D : public Point{
    public:
        double z;
        Point3D(double x, double y, double z): Point(x,y), z(z) {};
        Point3D &operator=(const Point3D & other){
            Point::operator=(other);
            z = other.z;
            return *this;
        }
};

class LonLat : private Point{
    public:
        LonLat(double lon, double lat) : Point(lon, lat) {};
        double &lon = Point::x;
        double &lat = Point::y;
        LonLat &operator=(const LonLat & other){
            Point::operator=(other);
            return *this;
        }
};

class LonLatZoom : public LonLat{
    public:
        LonLatZoom(double lon, double lat, double zoom): LonLat(lon, lat), zoom(zoom) {};
        double zoom;
        LonLatZoom &operator=(const LonLatZoom & other){
            LonLat::operator=(other);
            zoom = other.zoom;
            return *this;
        }
};

using Camera = LonLatZoom;

class Tile : public QObject{
    Q_OBJECT

    public:
        Tile(QString url, int px, int py, QObject *parent=nullptr);
        ~Tile();

        const int px, py;
        QGraphicsItem *pixmap = nullptr;

    private:
        QNetworkReply *reply = nullptr;
        QString url;

    private slots:
        void processResponse();

    signals:
        void created(QGraphicsItem *pixmap);
};

Point mercatorProject(LonLat pos);
LonLat mercatorUnproject(Point pos);
Point3D lonlat2tile(LonLatZoom pos);
LonLatZoom tile2lonlat(Point3D pos);

Point lonlat2scenePoint(LonLatZoom pos, int tileSize=TILE_SIZE);
LonLatZoom scenePoint2lonLat(Point scenePoint, int zoom, int tileSize=TILE_SIZE);

struct TileInfo{
    int x, y, zoom, px, py;
    TileInfo(int x, int y, int zoom, int px, int py) :
        x(x), y(y), zoom(zoom), px(px), py(py) {}
};

struct BBox{
    int xmin, ymin, xmax, ymax;
    BBox(int xmin, int ymin, int xmax, int ymax) :
        xmin(xmin), ymin(ymin), xmax(xmax), ymax(ymax) {}
};

using TileGrid = QVector<TileInfo>;
using TileMap = QVector<Tile*>;

class TileLayer: public QObject{
    Q_OBJECT

    public:
        TileLayer(QString baseUrl, QObject *parent=nullptr);
        ~TileLayer();

        bool validateTileUrl(int x, int y, int z);
        QString getTileUrl(int x, int y, int z);
        TileMap renderTiles(TileGrid gridInfo);

        int maxZoom = 18;
    private:
        QString baseUrl;
};

class MapGraphicsView: public QGraphicsView{
    Q_OBJECT

    public:
        MapGraphicsView(QWidget *parent = nullptr);
        ~MapGraphicsView();

        void setCamera(Camera cam);
        void setCamera(double lon, double lat, double zoom);
        Camera getCamera();

        void addTileLayer(TileLayer *tileLayer);
        QVector<TileInfo> getVisibleTiles();
        void renderTiles();
        void clearTiles();

        void resizeEvent(QResizeEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

        #ifdef MAPVIEW_DEBUG
            QGraphicsLineItem *camHLine;
            QGraphicsLineItem *camVLine;
        #endif

    signals:
        void cameraChanged(double lon, double lat, double zoom);

    private:
        Camera cam = Camera(-3,40,7);
        QPointF previousP;
        QVector<TileLayer*> layers;
        QHash<QPair<int,int>,Tile*> tileStack;

    public slots:
        void onZoomChanged();
        void addItem(QGraphicsItem *item);
};