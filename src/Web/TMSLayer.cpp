#include "TMSLayer.h"

QNetworkAccessManager *mgr = new QNetworkAccessManager();

// ======================

Tile::Tile(QString url, int px, int py, int zValue, QObject *parent) : Layer(px,py,zValue,parent), url(url){
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


    this->item = new QGraphicsPixmapItem(temppix);
    this->item->setPos(this->px,this->py);
    this->item->setZValue(this->zValue);

    emit this->itemCreated(this->item);
    reply->deleteLater();
}

Tile::~Tile(){
    
}

// ======================

TileLayer::TileLayer(QString baseUrl, MapGraphicsView *parent) : LayerGroup(zValue,parent), baseUrl(baseUrl){

}

MapGraphicsView *TileLayer::parentView(){
    return static_cast<MapGraphicsView*>(parent());
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

QVector<TileInfo> TileLayer::getVisibleTiles(){
    MapGraphicsView *view = parentView();

    const int incrementX = TILE_SIZE, incrementY = TILE_SIZE;
	
	const int clientWidth = view->width() + incrementX; 
	const int clientHeight = view->height() + incrementY;

    auto cam = view->getCamera();
	
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

void TileLayer::onViewLonLatChanged(double lon, double lat){
    renderTiles();
}

void TileLayer::onViewZoomChanged(double zoom){
    clearTiles();
    renderTiles();
}

void TileLayer::onViewSizeChanged(int width, int height){
    renderTiles();
}

void TileLayer::renderTiles(){
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

    for(auto tileInfo: newGrid){
        if(!validateTileUrl(tileInfo.x,tileInfo.y,tileInfo.zoom)) continue;
        Tile *tile = new Tile(getTileUrl(tileInfo.x,tileInfo.y,tileInfo.zoom),tileInfo.px,tileInfo.py,this->zValue);
        tileStack[{tileInfo.px,tileInfo.py}] = tile;
        connect(tile,&Layer::itemCreated,this,&LayerGroup::itemCreated);
    }
}

void TileLayer::clearTiles(){
    for(Tile *tile: tileStack){
        tile->deleteLater(); // also delete from scene
    }
    tileStack.clear();
}

TileLayer::~TileLayer(){

}

// ======================