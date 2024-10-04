#include "mapview.h"

#include <QResizeEvent>
#include <QMouseEvent>
#include <QLayout>

QNetworkAccessManager *mgr = new QNetworkAccessManager();

Tile::Tile(QString url, int px, int py, QObject *parent) : QObject(parent), px(px), py(py), url(url){
    QNetworkRequest req(url);
    reply = mgr->get(req);
    reply->setParent(this);
    connect(reply,&QNetworkReply::finished,this,&Tile::processResponse);
};

void Tile::processResponse(){
    qDebug() << "Tile [url " << url << "]  [pos "<< this->px << "px" << "," << this->py << "py]";
    bool netError = !reply->error();
    assert(netError);
    if(!netError) return;

    QPixmap temppix;
    bool loadError = temppix.loadFromData(reply->readAll());
    assert(loadError);
    if(!loadError) return;

    #ifdef MAPVIEW_DEBUG // tile border
        QPainter p(&temppix);
        p.setPen(Qt::black);
        p.drawRect(0,0,temppix.width(),temppix.height());
    #endif


    this->pixmap = new QGraphicsPixmapItem(temppix);
    this->pixmap->setPos(this->px,this->py);

    emit this->created(this->pixmap);
    reply->deleteLater();
}

Tile::~Tile(){
    if(pixmap) delete pixmap;
}


// =============================

inline double rad(double deg) { return deg * (M_PI/180); }
inline double deg(double rad) { return rad * (180/M_PI); }
inline double sec(double rad) { return 1/cos(rad); }


#define DEGREE_TO_METER(Y) (log(tan((90.0 + (Y)) * M_PI / 360.0)) / (M_PI / 180.0))*111319.490778
#define DEGREE_TO_METER_REVERSE(Y) (atan(pow(M_E, ((Y)/111319.490778)*M_PI/180.0))*360.0/M_PI-90.0)

Point mercatorProject(LonLat pos){ // 4326 to 3857
    return Point(
        DEGREE_TO_METER(pos.lon),
        DEGREE_TO_METER(pos.lat)
    );
}

LonLat mercatorUnproject(Point pos){ // 3857 to 4326
    return LonLat(
        DEGREE_TO_METER_REVERSE(pos.x),//lon,
        DEGREE_TO_METER_REVERSE(pos.y)//atan(pow(M_E,rad(lat))*360) / M_PI - 90
    );

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

Point lonlat2scenePoint(LonLatZoom pos, int tileSize){
    const double n = pow(2,pos.zoom);
    return Point(
       (pos.lon+180) * (n*tileSize)/360,
       (1-(log(tan(M_PI/4+(rad(pos.lat))/2)) /M_PI)) /2  * (n*tileSize)
    );
}

LonLatZoom scenePoint2lonLat(Point scenePoint, int zoom, int tileSize){
    const double n = pow(2,zoom);
    return LonLatZoom(
        (scenePoint.x*(360/(n*tileSize)))-180,
        deg(atan(sinh((1-scenePoint.y*(2/(n*tileSize)))*M_PI))),
        zoom
    );
}


TileLayer::TileLayer(QString baseUrl, QObject *parent) : QObject(parent), baseUrl(baseUrl){

}


bool TileLayer::validateTileUrl(int x, int y, int z){
    if(x < 0 || y < 0 || z < 0) return false;
    if(z > maxZoom) return false;
    const int tilesPerEdge = pow(2,z);
    if(x >= tilesPerEdge || y >= tilesPerEdge) return false;
    return true;
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
        if(!validateTileUrl(tileInfo.x,tileInfo.y,tileInfo.zoom)) continue;
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

    auto camscp = lonlat2scenePoint(cam);
    scene()->setSceneRect(camscp.x,camscp.y,1,1);

    #ifdef MAPVIEW_DEBUG 
        // scene center
        scene()->addRect(-5,-5,10,10,QPen(Qt::red))->setZValue(101); // center of scene
        scene()->addLine(0,0,256,0,QPen(Qt::blue))->setZValue(100); // x axis
        scene()->addLine(0,0,0,256,QPen(Qt::green))->setZValue(100); // y axis

        // cam
        camHLine = scene()->addLine(camscp.x-256,camscp.y,camscp.x+256,camscp.y,QPen(Qt::red));
        camVLine = scene()->addLine(camscp.x,camscp.y-256,camscp.x,camscp.y+256,QPen(Qt::red));
        camHLine->setZValue(105);
        camVLine->setZValue(105);
    #endif
}

void MapGraphicsView::resizeEvent(QResizeEvent *event){
    QGraphicsView::resizeEvent(event);

    renderTiles();
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
    qDebug() << "Current scene rect: " << scene()->sceneRect();
    previousP = scenePos;

    auto newCam = scenePoint2lonLat(Point(newSceneRect.x(),newSceneRect.y()),cam.zoom);
    cam.lat = newCam.lat;
    cam.lon = newCam.lon;

    #ifdef MAPVIEW_DEBUG
        auto camscp = lonlat2scenePoint(cam);
        camHLine->setLine(camscp.x-256,camscp.y,camscp.x+256,camscp.y);
        camVLine->setLine(camscp.x,camscp.y-256,camscp.x,camscp.y+256);
    #endif

    qDebug() << "camera: " << cam.lat << cam.lon;

    renderTiles();

    QGraphicsView::mousePressEvent(event);
}

void MapGraphicsView::onZoomChanged(){
    auto camscp = lonlat2scenePoint(cam);
    scene()->setSceneRect(camscp.x,camscp.y,1,1);
    clearTiles();
    renderTiles();
}

QVector<TileInfo> MapGraphicsView::getVisibleTiles(){
	const int incrementX = TILE_SIZE, incrementY = TILE_SIZE;
	
	const int clientWidth = width() + incrementX; 
	const int clientHeight = height() + incrementY;
	
	Point centerpx = lonlat2scenePoint(cam);
	
	BBox bbox(
		floor((centerpx.x - clientWidth / 2) / TILE_SIZE),
		floor((centerpx.y - clientHeight / 2 ) / TILE_SIZE),
		ceil((centerpx.x + clientWidth / 2) / TILE_SIZE),
		ceil((centerpx.y + clientHeight / 2) / TILE_SIZE)
	);
 	
	QVector<TileInfo> tileInfos;
	
	for(int x = bbox.xmin; x < bbox.xmax; ++x){
		for(int y = bbox.ymin; y < bbox.ymax; ++y){
            auto scp = lonlat2scenePoint(tile2lonlat({(double)x,(double)y,cam.zoom}));
			tileInfos.push_back(TileInfo(
				x,y,cam.zoom,
                scp.x,scp.y
			));
		}
	}
	
	return tileInfos;
}

void MapGraphicsView::addTileLayer(TileLayer *layer){
    layer->setParent(this);
    layers.push_back(layer);
}

void MapGraphicsView::renderTiles(){
    TileGrid newGrid = getVisibleTiles();

    // clearing unused tiles from scene / TODO: better implementation
    QVector<QPair<int,int>> newGridCoords;
    for(auto info: newGrid){
        newGridCoords.push_back({info.px,info.py});
    }
    for(auto key: tileStack.keys()){
        if(!newGridCoords.contains(key)){
            tileStack.take(key)->deleteLater();
        }
    }


    // remove already existing tiles from grid to render
    for(int i=0;i<newGrid.size();i++){
        if(tileStack.contains({newGrid[i].px,newGrid[i].py})){
            newGrid.remove(i);
            i--;
        }
    }

    for(TileLayer *layer: layers){
        TileMap tiles = layer->renderTiles(newGrid);
        for(Tile *tile: tiles){
            tileStack[{tile->px,tile->py}] = tile;
            tile->connect(tile,&Tile::created,this,&MapGraphicsView::addItem);
        }
    }
}

void MapGraphicsView::clearTiles(){
    for(Tile *tile: tileStack){
        tile->deleteLater(); // also delete from scene
    }
    tileStack.clear();
}

void MapGraphicsView::addItem(QGraphicsItem *item){
    scene()->addItem(item);
}

MapGraphicsView::~MapGraphicsView(){

}