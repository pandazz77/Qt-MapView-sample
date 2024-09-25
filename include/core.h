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

class Point{
    public:
        double x,y;
        Point(double x, double y): x(x), y(y) {};
};

class LonLat : public Point{
    public:
        LonLat(double lon, double lat) : Point(lon, lat) {};
        double &lon = Point::x;
        double &lat = Point::y;
};

class Camera : public LonLat{
    public:
        int zoom;
        Camera(double lon = 0, double lat = 0, int zoom = 0) : LonLat(lon, lat), zoom(zoom) {}
};

class Tile : public QObject{
    Q_OBJECT

    public:
        Tile(QString url, int px, int py, QObject *parent=nullptr);
        ~Tile();

        const int px, py;
        QGraphicsPixmapItem *pixmap = nullptr;

    signals:
        void created(QGraphicsPixmapItem *pixmap);
};

Point mercatorProject(LonLat pos);

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

        QString getTileUrl(int x, int y, int z);
        TileMap renderTiles(TileGrid gridInfo);

    private:
        QString baseUrl;
};

class MapView: public QWidget{
    Q_OBJECT

    public:
        MapView(QWidget *parent = nullptr);
        ~MapView();

        void addTileLayer(TileLayer *tileLayer);

        QVector<TileInfo> getVisibleTiles();
        void renderTiles();
        void clearTiles();

        Camera cam;

    private slots:
        void addItem(QGraphicsPixmapItem *item);

    private:
        QGraphicsScene *scene = nullptr;
        QGraphicsView *view = nullptr;

        QVector<TileLayer*> layers;
        TileMap tileStack;
};