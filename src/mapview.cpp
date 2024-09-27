#include "mapview.h"

#include <QResizeEvent>
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


MapGraphicsView::MapGraphicsView(QWidget *parent) : QGraphicsView(new QGraphicsScene(),parent){
    // scale(1,-1); // flip y axis (will flip all tiles)

    #ifdef MAPVIEW_DEBUG // scene center
        scene()->addRect(-5,-5,10,10,QPen(Qt::red))->setZValue(101); // center of scene
        scene()->addLine(0,0,256,0,QPen(Qt::blue))->setZValue(100); // x axis
        scene()->addLine(0,0,0,256,QPen(Qt::green))->setZValue(100); // y axis
    #endif
}

void MapGraphicsView::resizeEvent(QResizeEvent *event){
    // qDebug() << event;
    // scene()->setSceneRect(-width()/2, -height()/2,width(),height());
    QGraphicsView::resizeEvent(event);
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

    const int startX = -clientWidth/2;
    const int startY = -clientHeight/2;


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
                // startX + ((x - bbox.xmin) * TILE_SIZE),
                // startY + ((y - bbox.ymin) * TILE_SIZE)
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