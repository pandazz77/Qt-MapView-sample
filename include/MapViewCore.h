#pragma once

#include <QObject>
#include <QGraphicsItem>

#define DEGREE_TO_METER(Y) (log(tan((90.0 + (Y)) * M_PI / 360.0)) / (M_PI / 180.0))*111319.490778
#define DEGREE_TO_METER_REVERSE(Y) (atan(pow(M_E, ((Y)/111319.490778)*M_PI/180.0))*360.0/M_PI-90.0)

inline double rad(double deg) { return deg * (M_PI/180); }
inline double deg(double rad) { return rad * (180/M_PI); }
inline double sec(double rad) { return 1/cos(rad); }

class Point{
    public:
        double x;
        double y;

        Point(double x, double y);
        Point &operator=(const Point & other);
};

class Point3D : public Point{
    public:
        double z;

        Point3D(double x, double y, double z);
        Point3D &operator=(const Point3D & other);
};

class LonLat : private Point{
    public:
        double &lon = Point::x;
        double &lat = Point::y;

        LonLat(double lon, double lat);
        LonLat &operator=(const LonLat & other);
};

class LonLatZoom : public LonLat{
    public:
        double zoom;

        LonLatZoom(double lon, double lat, double zoom);
        LonLatZoom &operator=(const LonLatZoom & other);
};

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