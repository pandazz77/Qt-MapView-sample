#pragma once

#include "MapViewCore.h"
#include "MapView.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define TILE_SIZE 256
#define WEBMERCATOR_R 6378137.0
#define DIAMETER (WEBMERCATOR_R * 2 * M_PI)

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