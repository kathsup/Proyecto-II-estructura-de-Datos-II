#include "diskmanager.h"
#include <QDir>

DiskManager::DiskManager() {}

bool DiskManager:: crearDisco(int size, QString unit, QString path, QString fit){
    long long tamanio_bytes = size;

    if(unit.toLower() == "m"){
        tamanio_bytes = size * 1024 * 1024;
    }else if(unit.toLower() == "k"){
        tamanio_bytes = size *1024;
    }

    if(!crearCarpeta(path)){
        return false; //si falla no se puede crear
    }

    if(!crearArchivoVacio(path, tamanio_bytes)){//si falla detiene
        return false;
    }

    //crear raid - la copia
    QString pathRaid = path +"_Raid";
    if(!crearArchivoVacio(pathRaid, tamanio_bytes)){return false;}

    return true;
}



bool DiskManager:: crearCarpeta(QString ruta){
    int ultimaBarra = ruta.lastIndexOf("/");
    if(ultimaBarra == -1){
        return true;
    }
    QString carpeta = ruta.left(ultimaBarra);//obtiene lo que esta a la izquierda de la barra - la carpeta
    QDir dir; //herramienta para trabajar con carpetas

    if(!dir.exists(carpeta)){
        //si la carpeta no existe se crea
        if(!dir.mkpath(carpeta)){
            //si no existen crea todas las carpetas del camino
            return false;
        }
    }
    return true;
}




bool DiskManager::crearArchivoVacio(QString path, long long tamanio_bytes){
    QFile archivo(path);//esta pero aun no existe

    if(!archivo.open(QIODevice::WriteOnly)){//falla al abrir
        return false;
    }

    char cero = '\0'; //caracter nulo, representa byte que su valor es 0

    for (long long i = 0; i<tamanio_bytes; i++){
        archivo.write(&cero, 1);
    }
    archivo.close();
    return true;
}


bool DiskManager::eliminarDisco(QString path){

    QFile archivo(path);
    QFile archivoRaid(path+"_Raid");

    if(!archivo.exists()){
        return false;
    }else{
        archivo.remove();
        archivoRaid.remove();
        return true;
    }


}



