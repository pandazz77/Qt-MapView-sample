#include "mapview.h"

#include <QResizeEvent>
#include <QMouseEvent>
#include <QLayout>

#include "MapViewCore.h"

// =============================

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

Point _lonlat2scenePoint(LonLat pos, int mapWidth, int mapHeight){
    return Point(
       (pos.lon+180) * mapWidth/360,
       (1-(log(tan(M_PI/4+(rad(pos.lat))/2)) /M_PI)) /2  * mapHeight
    );
}

LonLat _scenePoint2lonLat(Point scenePoint, int mapWidth, int mapHeight){
    return LonLat(
        (scenePoint.x*(360/(double)mapWidth))-180,
        deg(atan(sinh((1-scenePoint.y*(2/(double)mapHeight))*M_PI)))
    );
}

Point lonlat2scenePoint(LonLatZoom pos, int tileSize){
    const double n = pow(2,pos.zoom);
    const double size = n * tileSize;
    return _lonlat2scenePoint(pos,size,size);
}

LonLatZoom scenePoint2lonLat(Point scenePoint, int zoom, int tileSize){
    const double n = pow(2,zoom);
    const int size = n * tileSize;
    auto pre = _scenePoint2lonLat(scenePoint,size,size);
    return LonLatZoom(
        pre.lon,
        pre.lat,
        (double)zoom
    );
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

void MapGraphicsView::setCamera(Camera cam){
    auto previousCam = this->cam;
    this->cam = cam;
    if(previousCam.zoom != cam.zoom){
        onZoomChanged();
    }
    if(previousCam.lon != cam.lon && previousCam.lat != cam.lat){
        onLonLatChanged();
    }
}

void MapGraphicsView::setCamera(double lon, double lat, double zoom){
    MapGraphicsView::setCamera(Camera(lon,lat,zoom));
}

Camera MapGraphicsView::getCamera(){
    return cam;
}


void MapGraphicsView::resizeEvent(QResizeEvent *event){
    QGraphicsView::resizeEvent(event);

    emit sizeChanged(width(),height());
}

void MapGraphicsView::mousePressEvent(QMouseEvent *event){
    previousP = event->scenePosition();
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent *event){
    auto scenePos = event->scenePosition();

    QPointF delta = previousP - scenePos;
    QRectF oldSceneRect = scene()->sceneRect();
    cam = scenePoint2lonLat(Point(oldSceneRect.x()+delta.x(),oldSceneRect.y()+delta.y()),cam.zoom);
    onLonLatChanged();

    previousP = scenePos;

    QGraphicsView::mousePressEvent(event);
}

void MapGraphicsView::wheelEvent(QWheelEvent *event){
    bool wheelUp = event->angleDelta().y() > 0;
    wheelUp ? cam.zoom += 1 : cam.zoom -=1;
    onZoomChanged();
}

void MapGraphicsView::onLonLatChanged(){
    auto camscp = lonlat2scenePoint(cam);
    auto previousRect = scene()->sceneRect();
    scene()->setSceneRect(camscp.x,camscp.y,previousRect.width(),previousRect.height());
    qDebug() << "camera: " << cam.lat << cam.lon << "|" << camscp.x << camscp.y;

    #ifdef MAPVIEW_DEBUG
        camHLine->setLine(camscp.x-256,camscp.y,camscp.x+256,camscp.y);
        camVLine->setLine(camscp.x,camscp.y-256,camscp.x,camscp.y+256);
    #endif

    emit lonLatChanged(cam.lon,cam.lat);
}

void MapGraphicsView::onZoomChanged(){
    auto camscp = lonlat2scenePoint(cam);
    #ifdef MAPVIEW_DEBUG
        camHLine->setLine(camscp.x-256,camscp.y,camscp.x+256,camscp.y);
        camVLine->setLine(camscp.x,camscp.y-256,camscp.x,camscp.y+256);
    #endif
    scene()->setSceneRect(camscp.x,camscp.y,1,1);
    emit zoomChanged(cam.zoom);
}

void MapGraphicsView::addLayer(ILayer *layer){
    layer->setParent(this);
    layer->setZValue(layers.size());

    connect(layer,&ILayer::itemCreated,this,&MapGraphicsView::addItem);

    // TODO: better implementation
    if(dynamic_cast<Layer*>(layer)){
        Layer *standalone = dynamic_cast<Layer*>(layer);
        if(standalone->getItem()) addItem(standalone->getItem());
    } else {
        LayerGroup *group = dynamic_cast<LayerGroup*>(layer);
        if(!group->getItems().isEmpty()){
            for(auto *item: group->getItems()){
                addItem(item);
            }
        }
    }

    connect(this,&MapGraphicsView::lonLatChanged,layer,&ILayer::onViewLonLatChanged);
    connect(this,&MapGraphicsView::zoomChanged,layer,&ILayer::onViewZoomChanged);
    connect(this,&MapGraphicsView::sizeChanged,layer,&ILayer::onViewSizeChanged);

    layers.push_back(layer);
}

void MapGraphicsView::addItem(QGraphicsItem *item){
    scene()->addItem(item);
}

MapGraphicsView::~MapGraphicsView(){

}