#include "mapview.h"

#include <QResizeEvent>
#include <QMouseEvent>
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

        #ifdef MAPVIEW_DEBUG // tile border
            QPainter p(&temppix);
            p.setPen(Qt::black);
            p.drawRect(0,0,temppix.width(),temppix.height());
        #endif


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

inline double rad(double deg) { return deg * (M_PI/180); }
inline double deg(double rad) { return rad * (180/M_PI); }
inline double sec(double rad) { return 1/cos(rad); }

Point mercatorProject(LonLat pos){
    double x = DIAMETER * pos.lon / 360.0;
    double sinlat = sin(pos.lat * M_PI / 180.0);
    double y = DIAMETER * log((1 + sinlat) / (1 - sinlat)) / (4 * M_PI);
    return Point(DIAMETER/2 + x, DIAMETER - (DIAMETER /2 +y));
}

Point3D lonlat2tile(LonLatZoom pos){
    double latRad = rad(pos.lat);
    double n = pow(2,pos.zoom);
    return Point3D(
        n * ((pos.lon+180) / 360),
        n * (1 - (log(tan(latRad) + sec(latRad)) / M_PI )) / 2,
        pos.zoom
    );
}

LonLatZoom tile2lonlat(Point3D pos){
    double n = pow(2,pos.z);
    double latRad = atan(sinh(M_PI * (1-2*pos.y/n)));
    return LonLatZoom(
        pos.x / n * 360.0 - 180.0,
        deg(latRad),
        pos.z
    );

}

Point tile2scenePoint(Point3D tile, Point3D centerTile, int tileSize){
    // CENTER TILE COORDS ARE NOT FLOORED
    // TILE COORDS ARE FLOORED
    return Point(
        floor(centerTile.x - tile.x) * -tileSize,
        floor(centerTile.y - tile.y) * -tileSize
    );
}

Point3D scenePoint2tile(Point scenePoint, Point3D centerTile, int tileSize){
    // CENTER TILE COORDS ARE NOT FLOORED
    // RESULT TILE COORDS ARE FLOORED
    return Point3D(
        centerTile.x + scenePoint.x / tileSize,
        centerTile.y + scenePoint.y / tileSize,
        centerTile.z
    );
}

LonLatZoom scenePoint2lonlat(Point scenePoint, Point3D centerTile, int tileSize){
    auto tile = scenePoint2tile(scenePoint,centerTile,tileSize);
    return tile2lonlat(tile);
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


MapGraphicsView::MapGraphicsView(QWidget *parent) : QGraphicsView(new QGraphicsScene(),parent), centerTile(lonlat2tile(cam)){
    // scale(1,-1); // flip y axis (will flip all tiles)

    #ifdef MAPVIEW_DEBUG // scene center
        scene()->addRect(-5,-5,10,10,QPen(Qt::red))->setZValue(101); // center of scene
        scene()->addLine(0,0,256,0,QPen(Qt::blue))->setZValue(100); // x axis
        scene()->addLine(0,0,0,256,QPen(Qt::green))->setZValue(100); // y axis
    #endif
}

void MapGraphicsView::resizeEvent(QResizeEvent *event){
    // qDebug() << event;
    scene()->setSceneRect(-width()/2, -height()/2,width(),height());
    QGraphicsView::resizeEvent(event);
}

void MapGraphicsView::mousePressEvent(QMouseEvent *event){
    previousP = event->scenePosition();
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent *event){
    auto scenePos = event->scenePosition();
    QPointF delta = previousP - scenePos;
    QRectF oldSceneRect = scene()->sceneRect();
    QRectF newSceneRect(oldSceneRect.x()+delta.x(),oldSceneRect.y()+delta.y(),oldSceneRect.width(),oldSceneRect.height());
    scene()->setSceneRect(newSceneRect);
    // for(auto item: items()) item->setPos(item->pos()-delta); // XD
    previousP = scenePos;

    auto newCam = scenePoint2lonlat(Point(newSceneRect.x(),newSceneRect.y()),centerTile);
    cam.lat = newCam.lat;
    cam.lon = newCam.zoom;

    //qDebug() << scene()->sceneRect();

    QGraphicsView::mousePressEvent(event);
}


MapGraphicsView::~MapGraphicsView(){

}


MapView::MapView(QWidget *parent) : QWidget(parent), view(new MapGraphicsView()){
    setLayout(new QGridLayout());
    qDebug() << layout();
    layout()->addWidget(view);
};

QVector<TileInfo> MapView::getVisibleTiles(){
    const int incrementX = 0, incrementY = 0; 

    const int clientWidth = view->width() + incrementX; 
    const int clientHeight = view->height() + incrementY;

    qDebug() << "Client size:" << clientWidth << clientHeight;

    const int startX = view->scene()->sceneRect().topLeft().x();
    const int startY = view->scene()->sceneRect().topLeft().y();
    const int endX   = view->scene()->sceneRect().bottomRight().x();
    const int endY   = view->scene()->sceneRect().bottomRight().y();

    auto tileCenter = view->centerTile;

    qDebug() << "Tile center" << tileCenter.x << tileCenter.y << tileCenter.z;

    const Point3D firstTile = scenePoint2tile(Point(startX,startY),tileCenter);
    const Point3D lastTile = scenePoint2tile(Point(endX,endY),tileCenter);

    BBox bbox(
        firstTile.x,
        firstTile.y,
        lastTile.x,
        lastTile.y
    );

    QVector<TileInfo> tileInfos;

    for(int x = bbox.xmin; x < bbox.xmax; ++x){
        for(int y = bbox.ymin; y < bbox.ymax; ++y){
            tileInfos.push_back(TileInfo(
                x,y,view->cam.zoom,
                // (x - bbox.xmin) * TILE_SIZE,
                // (y - bbox.ymin) * TILE_SIZE
                startX + ((x - bbox.xmin) * TILE_SIZE),
                startY + ((y - bbox.ymin) * TILE_SIZE)
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
    for(Tile *tile: tileStack){
        tile->deleteLater(); // also delete from scene
    }
    tileStack.clear();
}

void MapView::addItem(QGraphicsItem *item){
    view->scene()->addItem(item);
}

MapView::~MapView(){

}