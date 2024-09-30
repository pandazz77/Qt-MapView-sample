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

    auto camscp = lonlat2scenePoint(cam);
    scene()->setSceneRect(camscp.x,camscp.y,1,1);

    #ifdef MAPVIEW_DEBUG // scene center
        scene()->addRect(-5,-5,10,10,QPen(Qt::red))->setZValue(101); // center of scene
        scene()->addLine(0,0,256,0,QPen(Qt::blue))->setZValue(100); // x axis
        scene()->addLine(0,0,0,256,QPen(Qt::green))->setZValue(100); // y axis
    #endif
}

void MapGraphicsView::resizeEvent(QResizeEvent *event){
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
    qDebug() << "Current scene rect: " << scene()->sceneRect();
    previousP = scenePos;

    auto newCam = scenePoint2lonLat(Point(newSceneRect.x(),newSceneRect.y()),cam.zoom);
    cam.lat = newCam.lat;
    cam.lon = newCam.lon;

    qDebug() << "camera: " << cam.lat << cam.lon;

    QGraphicsView::mousePressEvent(event);
}

void MapGraphicsView::onZoomChanged(){
    auto camscp = lonlat2scenePoint(cam);
    scene()->setSceneRect(camscp.x,camscp.y,1,1);
    // TODO: CLEAR TILELAYER!
}


MapGraphicsView::~MapGraphicsView(){

}


MapView::MapView(QWidget *parent) : QWidget(parent), view(new MapGraphicsView()){
    setLayout(new QGridLayout());
    qDebug() << layout();
    layout()->addWidget(view);
};

QVector<TileInfo> MapView::getVisibleTiles(){
	const int incrementX = TILE_SIZE*2, incrementY = TILE_SIZE*2; 
	
	const int clientWidth = view->width() + incrementX; 
	const int clientHeight = view->height() + incrementY;
	
	const int startX = view->scene()->sceneRect().topLeft().x();
	const int startY = view->scene()->sceneRect().topLeft().y();
	
	Point centerpx = lonlat2scenePoint(view->cam);
	
	BBox bbox(
		floor((centerpx.x - clientWidth / 2) / TILE_SIZE),
		floor((centerpx.y - clientHeight / 2 ) / TILE_SIZE),
		floor((centerpx.x + clientWidth / 2) / TILE_SIZE),
		floor((centerpx.y + clientHeight / 2) / TILE_SIZE)
	);
	
	QVector<TileInfo> tileInfos;
	
	for(int x = bbox.xmin; x < bbox.xmax; ++x){
		for(int y = bbox.ymin; y < bbox.ymax; ++y){
            auto scp = lonlat2scenePoint(tile2lonlat({(double)x,(double)y,view->cam.zoom}));
			tileInfos.push_back(TileInfo(
				x,y,view->cam.zoom,
                scp.x,scp.y
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