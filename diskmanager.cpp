#include "diskmanager.h"
#include <QDir>
#include <QFile>

DiskManager::DiskManager() {}

//parte1
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

    MBR mbr;
    mbr.tamanio_disco = tamanio_bytes;
    mbr.num_particiones = 0;
    mbr.fit_disco = fit.toLower().toStdString()[0];

    // Inicializar particiones vacías
    for(int i = 0; i < 4; i++){
        mbr.particiones[i].estado = 'L'; // L = Libre
        strcpy(mbr.particiones[i].nombre, "");
        mbr.particiones[i].tamanio = 0;
        mbr.particiones[i].inicio = 0;
    }

    // GUARDAR el MBR en el disco
    if(!guardarMBR(path, mbr)){
        return false;
    }


    // crear raid - la copia
    QString pathRaid = path +"_Raid";
    if(!crearArchivoVacio(pathRaid, tamanio_bytes)){return false;}

    // tmb inicializar el MBR del RAID
    if(!guardarMBR(pathRaid, mbr)){
        return false;
    }
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

//parte 2

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

//parte 3

MBR DiskManager::leerMBR(QString path){
    MBR mbr;

    QFile archivo(path);//crearlo y ver si no hay error al abrirlo
    if(!archivo.open(QIODevice::ReadOnly)){
        return mbr;
    }

    archivo.seek(0);//mover el puntero al inicio
    archivo.read((char*)&mbr, sizeof(MBR));
    archivo.close();
    return mbr;

}

bool DiskManager::guardarMBR(QString path, MBR mbr){
    QFile archivo(path);//crearlo y ver si no hay error al abrirlo
    if(!archivo.open(QIODevice::ReadWrite)){//readwrite solo modificaria el writeoly  borraria lo anterior
        return false;
    }
    archivo.seek(0);
    archivo.write((char*)&mbr, sizeof(MBR));
    archivo.close();
    return true;
}

bool DiskManager::fdisk(map<QString, QString> parametros){

    if (parametros["-path"].isEmpty()) {
        qDebug() << "Error: -path es obligatorio";
        return false;
    }

    if (parametros["-name"].isEmpty()) {
        qDebug() << "Error: -name es obligatorio";
        return false;
    }

    QString path = parametros["-path"];
    QString name = parametros["-name"];

    //leer el mbr
    MBR mbr = leerMBR(path);


    if (!parametros["-delete"].isEmpty()) {
        // ELIMINAR
    }
    else if (!parametros["-add"].isEmpty()) {
        //MODIFICAR TAM
    }
    else if (!parametros["-size"].isEmpty()) {
        if(!crearParticion(mbr, parametros)){
            return false;
        }
    }
    else {
        qDebug() << "Error: Debe especificar -size, -delete o -add";
        return false;
    }

    // guardar cambiso
    return guardarMBR(path, mbr);
}


bool DiskManager::crearParticion(MBR &mbr, map<QString, QString> parametros){
    // EXTRAER parámetros
    int size = parametros["-size"].toInt();
    QString unit = parametros["-unit"].isEmpty() ? "k" : parametros["-unit"];
    QString type = parametros["-type"].isEmpty() ? "p" : parametros["-type"];
    QString fit = parametros["-fit"].isEmpty() ? "wf" : parametros["-fit"];
    QString name = parametros["-name"];

    // Convertir tamaño a bytes
    long long tamanio_bytes = size;
    if(unit.toLower() == "m"){
        tamanio_bytes = size * 1024 * 1024;
    }else if(unit.toLower() == "k"){
        tamanio_bytes = size * 1024;
    }
    // si es b ya está en bytes

    // nombre no repetido
    for(int i = 0; i < mbr.num_particiones; i++){
        if(QString(mbr.particiones[i].nombre) == name && mbr.particiones[i].estado == 'U'){
            qDebug() << "Error: Ya existe una partición con ese nombre";
            return false;
        }
    }

    //tipo de partición
    char tipo_char = type.toLower().toStdString()[0];
    if(tipo_char != 'p' && tipo_char != 'e' && tipo_char != 'l'){
        qDebug() << "Error: Tipo inválido. Use P, E o L";
        return false;
    }

    // reglas de particiones
    if(tipo_char == 'e'){
        // Solo puede haber UNA extendida
        for(int i = 0; i < mbr.num_particiones; i++){
            if(mbr.particiones[i].tipo == 'E' && mbr.particiones[i].estado == 'U'){
                qDebug() << "Error: Ya existe una partición extendida";
                return false;
            }
        }
    }

    if(tipo_char == 'l'){
        // Debe existir una extendida
        bool hay_extendida = false;
        for(int i = 0; i < mbr.num_particiones; i++){
            if(mbr.particiones[i].tipo == 'E' && mbr.particiones[i].estado == 'U'){
                hay_extendida = true;
                break;
            }
        }
        if(!hay_extendida){
            qDebug() << "Error: No existe partición extendida para crear lógica";
            return false;
        }
    }

    // máximo 4 primarias+extendidas
    if(tipo_char == 'p' || tipo_char == 'e'){
        int primarias_extendidas = 0;
        for(int i = 0; i < mbr.num_particiones; i++){
            if((mbr.particiones[i].tipo == 'P' || mbr.particiones[i].tipo == 'E')
                && mbr.particiones[i].estado == 'U'){
                primarias_extendidas++;
            }
        }
        if(primarias_extendidas >= 4){
            qDebug() << "Error: Ya hay 4 particiones primarias/extendidas";
            return false;
        }
    }

    //  espacio disponible
    int inicio_disponible = sizeof(MBR); // Empieza después del MBR

    // Ordenar particiones por inicio para encontrar huecos
    Particion particiones_ordenadas[4];
    for(int i = 0; i < 4; i++){
        particiones_ordenadas[i] = mbr.particiones[i];
    }

    // Ordenamiento burbuja
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3 - i; j++){
            if(particiones_ordenadas[j].inicio > particiones_ordenadas[j+1].inicio
                && particiones_ordenadas[j+1].estado == 'U'){
                Particion temp = particiones_ordenadas[j];
                particiones_ordenadas[j] = particiones_ordenadas[j+1];
                particiones_ordenadas[j+1] = temp;
            }
        }
    }

    // Buscar espacio según el fit
    int posicion_inicio = -1;

    if(tipo_char == 'l'){
        // busco dentro de la extendida
        for(int i = 0; i < 4; i++){
            if(particiones_ordenadas[i].tipo == 'E' && particiones_ordenadas[i].estado == 'U'){
                int inicio_ext = particiones_ordenadas[i].inicio;
                int fin_ext = inicio_ext + particiones_ordenadas[i].tamanio;

                // Buscar hueco dentro de la extendida
                int inicio_logica = inicio_ext;
                for(int j = 0; j < 4; j++){
                    if(particiones_ordenadas[j].tipo == 'L' && particiones_ordenadas[j].estado == 'U'){
                        if(particiones_ordenadas[j].inicio >= inicio_ext &&
                            particiones_ordenadas[j].inicio < fin_ext){
                            inicio_logica = particiones_ordenadas[j].inicio + particiones_ordenadas[j].tamanio;
                        }
                    }
                }

                if(inicio_logica + tamanio_bytes <= fin_ext){
                    posicion_inicio = inicio_logica;
                }
                break;
            }
        }

        if(posicion_inicio == -1){
            qDebug() << "Error: No hay espacio en la partición extendida";
            return false;
        }
    }
    else{
        // si es primaria o secundaria buscar en todo el disco
        inicio_disponible = sizeof(MBR);

        for(int i = 0; i < 4; i++){
            if(particiones_ordenadas[i].estado == 'U' &&
                (particiones_ordenadas[i].tipo == 'P' || particiones_ordenadas[i].tipo == 'E')){

                int espacio_libre = particiones_ordenadas[i].inicio - inicio_disponible;

                if(espacio_libre >= tamanio_bytes){
                    posicion_inicio = inicio_disponible;
                    break;
                }

                inicio_disponible = particiones_ordenadas[i].inicio + particiones_ordenadas[i].tamanio;
            }
        }

        // Revisar espacio después de la última partición
        if(posicion_inicio == -1){
            int espacio_final = mbr.tamanio_disco - inicio_disponible;
            if(espacio_final >= tamanio_bytes){
                posicion_inicio = inicio_disponible;
            }
        }

        if(posicion_inicio == -1){
            qDebug() << "Error: No hay espacio suficiente en el disco";
            return false;
        }
    }

    // crear la partición
    Particion nueva;
    strcpy(nueva.nombre, name.toStdString().c_str());
    nueva.tamanio = tamanio_bytes;
    nueva.inicio = posicion_inicio;
    nueva.tipo = tipo_char == 'p' ? 'P' : (tipo_char == 'e' ? 'E' : 'L');
    nueva.estado = 'U'; // Usada
    nueva.fit = fit.toLower().toStdString()[0];

    // guardar en el MBR
    mbr.particiones[mbr.num_particiones] = nueva;
    mbr.num_particiones++;

    qDebug() << "Partición creada exitosamente";
    return true;
}



