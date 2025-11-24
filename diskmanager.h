#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QString>

class DiskManager
{
public:
    DiskManager();

    bool crearDisco(int size, QString unit, QString path, QString fit);
    bool crearCarpeta(QString ruta);
    bool crearArchivoVacio(QString path, long long tamanio_bytes);
    bool eliminarDisco(QString path);
};

#endif // DISKMANAGER_H
