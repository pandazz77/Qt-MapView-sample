#include "MapViewCore.h"

// ======================

Point::Point(double x, double y): x(x), y(y){

}

Point &Point::operator=(const Point & other){
    x = other.x;
    y = other.y;
    return *this;
}

// ======================

Point3D::Point3D(double x, double y, double z): Point(x,y), z(z){

}

Point3D &Point3D::operator=(const Point3D & other){
    Point::operator=(other);
    z = other.z;
    return *this;
}

// ======================

LonLat::LonLat(double lon, double lat): Point(lon,lat){

}

LonLat &LonLat::operator=(const LonLat & other){
    Point::operator=(other);
    return *this;
}

// ======================

LonLatZoom::LonLatZoom(double lon, double lat, double zoom): LonLat(lon, lat), zoom(zoom) {

};

LonLatZoom &LonLatZoom::operator=(const LonLatZoom & other){
    LonLat::operator=(other);
    zoom = other.zoom;
    return *this;
}

// ======================

ILayer::ILayer(int zValue, QObject *parent) : QObject(parent), zValue(zValue){

}

void ILayer::setZValue(int zValue){
    this->zValue = zValue;
}

int ILayer::getZValue(){
    return zValue;
}


ILayer::~ILayer(){

}

// ======================

Layer::Layer(int px, int py, int zValue, QObject *parent) : px(px), py(py), ILayer(zValue,parent){

}

QGraphicsItem *Layer::getItem(){
    return this->item;
}

void Layer::setPos(int px, int py, int zValue){
    this->px = px;
    this->py = py;
    this->zValue = zValue;
    if(item){
        item->setPos(px,py);
        item->setZValue(zValue);
    }
}

Point3D Layer::getPos(){
    return {(double)px, (double)py, (double)zValue};
}

Layer::~Layer(){
    if(item) delete item;
}

// ======================

LayerGroup::LayerGroup(int zValue, QObject *parent) : ILayer(zValue,parent){

}

void LayerGroup::setZValue(int zValue){
    ILayer::setZValue(zValue);
    for(ILayer *layer: layers){
        if(dynamic_cast<LayerGroup*>(layer)){
            LayerGroup *group = dynamic_cast<LayerGroup*>(layer);
            group->setZValue(this->zValue);
        } else { // standalone layer
            Layer *standalone = dynamic_cast<Layer*>(layer);
            auto pos = standalone->getPos();
            standalone->setPos(pos.x,pos.y,this->zValue);
        }
    }
}



void LayerGroup::addLayer(ILayer *layer){
    layers.insert(layer);
    if(dynamic_cast<LayerGroup*>(layer)){
        LayerGroup *group = dynamic_cast<LayerGroup*>(layer);
        connect(group,&LayerGroup::itemCreated,this,&LayerGroup::itemCreated);
    } else { // standalone layer
        Layer *standalone = dynamic_cast<Layer*>(layer);
        connect(standalone,&Layer::itemCreated,this,&LayerGroup::itemCreated);
    }
}

void LayerGroup::removeLayer(ILayer *layer){
    layers.remove(layer);
    if(dynamic_cast<LayerGroup*>(layer)){
        LayerGroup *group = dynamic_cast<LayerGroup*>(layer);
        disconnect(group,&LayerGroup::itemCreated,this,&LayerGroup::itemCreated);
    } else { // standalone layer
        Layer *standalone = dynamic_cast<Layer*>(layer);
        disconnect(standalone,&Layer::itemCreated,this,&LayerGroup::itemCreated);
    }
}

QSet<QGraphicsItem*> LayerGroup::getItems(){
    QSet<QGraphicsItem*> result;
    for(ILayer *layer: layers){
        if(dynamic_cast<LayerGroup*>(layer)){
            LayerGroup *group = dynamic_cast<LayerGroup*>(layer);
            result = result.unite(group->getItems());
        } else { // standalone layer
            Layer *standalone = dynamic_cast<Layer*>(layer);
            result << standalone->getItem();
        }
    }
    return result;
}

LayerGroup::~LayerGroup(){

}

// ======================