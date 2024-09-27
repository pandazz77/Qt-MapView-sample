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

#define MAPVIEW_DEBUG

class Point{
    public:
        double x,y;
        Point(double x, double y): x(x), y(y) {};
};

class Point3D : public Point{
    public:
        double z;
        Point3D(double x, double y, double z): Point(x,y), z(z) {};
};

class LonLat : public Point{
    public:
        LonLat(double lon, double lat) : Point(lon, lat) {};
        double &lon = Point::x;
        double &lat = Point::y;
};

class LonLatZoom : public LonLat{
    public:
        LonLatZoom(double lon, double lat, double zoom): LonLat(lon, lat), zoom(zoom) {};
        double zoom;
};

using Camera = LonLatZoom;

class Tile : public QObject{
    Q_OBJECT

    public:
        Tile(QString url, int px, int py, QObject *parent=nullptr);
        ~Tile();

        const int px, py;
        QGraphicsItem *pixmap = nullptr;

    signals:
        void created(QGraphicsItem *pixmap);
};

Point mercatorProject(LonLat pos);
Point3D lonlat2tile(LonLatZoom pos);
LonLatZoom tile2lonlat(Point3D pos);

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

class MapGraphicsView: public QGraphicsView{
    Q_OBJECT

    public:
        MapGraphicsView(QWidget *parent = nullptr);
        ~MapGraphicsView();

        void resizeEvent(QResizeEvent *event) override;

        int sizeIncrement = 256;
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

        Camera cam = Camera(0,0,0);

    private slots:
        void addItem(QGraphicsItem *item);

    private:
        MapGraphicsView *view = nullptr;

        QVector<TileLayer*> layers;
        TileMap tileStack;
};