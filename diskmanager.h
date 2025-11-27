#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QString>
#include "structuras.h"
#include <map>

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

private:
    bool crearParticion(MBR &mbr, map<QString, QString> parametros);
};

#endif // DISKMANAGER_H
