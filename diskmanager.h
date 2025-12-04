#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QString>
#include "structuras.h"
#include <map>
#include <QVector>

using namespace std;

class DiskManager
{
public:
    DiskManager();

    bool crearDisco(int size, QString unit, QString path, QString fit);
    bool crearCarpeta(QString ruta);
    bool crearArchivoVacio(QString path, long long tamanio_bytes);
    bool eliminarDisco(QString path);
    MBR leerMBR(QString path);
    bool guardarMBR(QString path, MBR mbr);
    bool fdisk(map<QString, QString> parametros);
    bool mount(QString path, QString name);
    QString generarID(QString path);  // Helper para generar vdXN
    bool estaMontada(QString path, QString name);
    bool unmount(QString id);
    QString obtenerTablaParticionesMontadas();

private:
    bool crearParticion(MBR &mbr, map<QString, QString> parametros);
    bool eliminarParticion(MBR &mbr, QString name, QString deleteType);
    bool modificarTamanioParticion(MBR &mbr, map<QString, QString> parametros);
    QVector<ParticionMontada> particiones_montadas;

};

#endif // DISKMANAGER_H
