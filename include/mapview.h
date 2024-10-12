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

class ILayer: public QObject{
    Q_OBJECT

    public:
        ILayer(int zValue=0, QObject *parent=nullptr);
        virtual ~ILayer();

        virtual void setZValue(int zValue);
        int getZValue();

    public slots:
        virtual void onViewLonLatChanged(double lon, double lat){ };
        virtual void onViewZoomChanged(double zoom){ };
        virtual void onViewSizeChanged(int width, int height){ };

    signals:
        void itemCreated(QGraphicsItem *item);

    protected:
        int zValue;
};

class Layer: public ILayer{
    Q_OBJECT

    public:
        Layer(int px, int py, int zValue, QObject *parent=nullptr);
        ~Layer();

        QGraphicsItem *getItem();
        void setPos(int px, int py, int zValue);
        Point3D getPos();

    protected:
        QGraphicsItem *item = nullptr;
        int px, py;
};

class LayerGroup: public ILayer{
    Q_OBJECT

    public:
        LayerGroup(int zValue, QObject *parent=nullptr);
        ~LayerGroup();

        void addLayer(ILayer *layer);
        void removeLayer(ILayer *layer);

        QSet<QGraphicsItem*> getItems();
        void setZValue(int zValue) override;

    protected:
        // should i use QGraphicsItemGroup ?
        QSet<ILayer*> layers;
};

class Tile : public Layer{
    Q_OBJECT

    public:
        Tile(QString url, int px, int py, int zValue, QObject *parent=nullptr);
        ~Tile();

    private:
        QNetworkReply *reply = nullptr;
        QString url;

    private slots:
        void processResponse();
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

class MapGraphicsView;

class TileLayer: public LayerGroup{
    Q_OBJECT

    public:
        TileLayer(QString baseUrl, MapGraphicsView *parent=nullptr);
        ~TileLayer();

        MapGraphicsView *parentView();

        bool validateTileUrl(int x, int y, int z);
        QString getTileUrl(int x, int y, int z);
        QVector<TileInfo> getVisibleTiles();

        int maxZoom = 18;

    public slots:
        void onViewLonLatChanged(double lon, double lat) override;
        void onViewZoomChanged(double zoom) override;
        void onViewSizeChanged(int width, int height) override;

    private slots:
        void renderTiles();
        void clearTiles();
       
    private:
        QString baseUrl;
        QHash<QPair<int,int>,Tile*> tileStack;
};

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