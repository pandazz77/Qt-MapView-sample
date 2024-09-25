#include <QApplication>

int main(int argc, char *argv[]){
    QApplication app(argc,argv);

    qDebug() << "Hello world!";

    return app.exec();
}