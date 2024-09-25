#include "core.h"

#include <QLayout>

QNetworkAccessManager *mgr = new QNetworkAccessManager();

Tile::Tile(QString url, int px, int py, QObject *parent) : QObject(parent), px(px), py(py){
    QNetworkRequest req(url);
    QNetworkReply *reply = mgr->get(req);
    connect(reply,&QNetworkReply::finished,[reply,this,url](){
        qDebug() << "Tile [url " << url << "]  [pos "<< this->px << "px" << "," << this->py << "py]";
        assert(!reply->error());

        QPixmap temppix;
        assert(temppix.loadFromData(reply->readAll()));
        this->pixmap = new QGraphicsPixmapItem(temppix);
        this->pixmap->setPos(this->px,this->py);

        emit this->created(this->pixmap);
        reply->deleteLater();
    });
};

Tile::~Tile(){
    if(pixmap) delete pixmap;
}


// =============================

Point mercatorProject(LonLat pos){
    double x = DIAMETER * pos.lon / 360.0;
    double sinlat = sin(pos.lat * M_PI / 180.0);
    double y = DIAMETER * log((1 + sinlat) / (1 - sinlat)) / (4 * M_PI);
    return Point(DIAMETER/2 + x, DIAMETER - (DIAMETER /2 +y));
}


TileLayer::TileLayer(QString baseUrl, QObject *parent) : QObject(parent), baseUrl(baseUrl){

}

QString TileLayer::getTileUrl(int x, int y, int z){
    QString result = baseUrl;
    result = result.replace("{x}",QString::number(x));
    result = result.replace("{y}",QString::number(y));
    result = result.replace("{z}",QString::number(z));
    return result;
}

TileMap TileLayer::renderTiles(TileGrid gridInfo){
    TileMap result;
    for(auto tileInfo: gridInfo){
        Tile *tile = new Tile(getTileUrl(tileInfo.x,tileInfo.y,tileInfo.zoom),tileInfo.px,tileInfo.py);
        result.push_back(tile);
    }
    return result;
}

TileLayer::~TileLayer(){

}

// ================================

MapView::MapView(QWidget *parent) : QWidget(parent), view(new QGraphicsView(new QGraphicsScene,this)){
    scene = view->scene();
};

QVector<TileInfo> MapView::getVisibleTiles(){
    const int clientWidth = parentWidget()->width(); // todo: view width
    const int clientHeight = parentWidget()->height(); // todo: view height


    Point centerm = mercatorProject(cam);
    Point centerpx(
        centerm.x * TILE_SIZE * pow(2,cam.zoom) / DIAMETER,
        centerm.y * TILE_SIZE * pow(2,cam.zoom) / DIAMETER
    );

    BBox bbox(
        floor((centerpx.x - clientWidth / 2) / TILE_SIZE),
        floor((centerpx.y - clientHeight / 2 ) / TILE_SIZE),
        floor((centerpx.x + clientWidth / 2) / TILE_SIZE),
        floor((centerpx.y + clientHeight / 2) / TILE_SIZE)
    );

    QVector<TileInfo> tileInfos;

    for(int x = bbox.xmin; x < bbox.xmax; ++x){
        for(int y = bbox.ymin; y < bbox.ymax; ++y){
            tileInfos.push_back(TileInfo(
                x,y,cam.zoom,
                (x - bbox.xmin) * TILE_SIZE,
                (y - bbox.ymin) * TILE_SIZE
                // x * TILE_SIZE - centerpx.x + clientWidth / 2,
                // y * TILE_SIZE - centerpx.y + clientHeight / 2
            ));
        }
    }

    return tileInfos;
}

void MapView::addTileLayer(TileLayer *layer){
    layer->setParent(this);
    layers.push_back(layer);
}

void MapView::renderTiles(){
    TileGrid grid = getVisibleTiles();
    for(TileLayer *layer: layers){
        TileMap tiles = layer->renderTiles(grid);
        tileStack.append(tiles);
        for(Tile *tile: tiles){
            tile->connect(tile,&Tile::created,this,&MapView::addItem);
        }
    }
}

void MapView::clearTiles(){
    for(Tile *tile: tileStack) scene->removeItem(tile->pixmap);
    tileStack.clear();
}

void MapView::addItem(QGraphicsPixmapItem *item){
    scene->addItem(item);
    view->adjustSize();
}

MapView::~MapView(){

}