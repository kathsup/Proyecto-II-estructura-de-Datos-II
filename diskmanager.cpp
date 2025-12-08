#include "diskmanager.h"
#include <QDir>
#include <QFile>
#include <QTextCursor>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QTextImageFormat>
#include <QDateTime>
#include <QBuffer>
#include <QScrollBar>
#include <QRandomGenerator>

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
    //mbr.fit_disco = fit.toLower().toStdString()[0];

    QString fitNormalizado = fit.toLower();
    if(fitNormalizado == "bf") {
        mbr.fit_disco = 'B';
    } else if(fitNormalizado == "ff") {
        mbr.fit_disco = 'F';
    } else { // wf
        mbr.fit_disco = 'W';
    }

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


    if(!archivo.exists()){
        return false;
    }

    return archivo.remove();


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


    QFile archivo(path);
    if(!archivo.exists()){
        qDebug() << "El disco no existe en la ruta especificada";
        return false;
    }

    //leer el mbr
    MBR mbr = leerMBR(path);


    if (!parametros["-delete"].isEmpty()) {
        // ELIMINAR
        QString deleteType = parametros["-delete"].toLower();
        if(deleteType != "fast" && deleteType != "full"){
            qDebug() << "Error: -delete debe ser Fast o Full";
            return false;
        }

        if(!eliminarParticion(mbr, name, deleteType)){
            return false;
        }
    }
    else if (!parametros["-add"].isEmpty()) {
        // MODIFICAR
        if(!modificarTamanioParticion(mbr, parametros)){
            return false;
        }
    }
    else if (!parametros["-size"].isEmpty()) {
        // CREAR
        if(!crearParticion(mbr, parametros)){
            return false;
        }
    }
    else {
        qDebug() << "Debe especificar -size, -delete o -add";
        return false;
    }


    // guardar cambiso
    if(!guardarMBR(path, mbr)){
        qDebug() << "Error: No se pudieron guardar los cambios";
        return false;
    }

    QString pathRaid = path + "_Raid";
    if(QFile(pathRaid).exists()){
        guardarMBR(pathRaid, mbr);
    }

    return true;
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
    Particion particiones_ordenadas[20];
    for(int i = 0; i < 20; i++){
        particiones_ordenadas[i] = mbr.particiones[i];
    }

    // Ordenamiento burbuja
    for(int i = 0; i < 19; i++){
        for(int j = 0; j < 19 - i; j++){
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
        for(int i = 0; i < 20; i++){
            if(particiones_ordenadas[i].tipo == 'E' && particiones_ordenadas[i].estado == 'U'){
                int inicio_ext = particiones_ordenadas[i].inicio;
                int fin_ext = inicio_ext + particiones_ordenadas[i].tamanio;

                // Buscar hueco dentro de la extendida
                int inicio_logica = inicio_ext;
                for(int j = 0; j < 20; j++){
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

        for(int i = 0; i < 20; i++){
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
    //nueva.fit = fit.toLower().toStdString()[0];

    QString fitNormalizado = fit.toLower();
    if(fitNormalizado == "bf") {
        nueva.fit = 'B';
    } else if(fitNormalizado == "ff") {
        nueva.fit = 'F';
    } else { // wf
        nueva.fit = 'W';
    }

    // guardar en el MBR
    mbr.particiones[mbr.num_particiones] = nueva;
    mbr.num_particiones++;

    qDebug() << "Partición creada exitosamente";
    return true;
}


//eliminar
bool DiskManager::eliminarParticion(MBR &mbr, QString name, QString deleteType){
    int indice_eliminar = -1;

    // buscar la partición por nombre
    for(int i = 0; i < mbr.num_particiones; i++){
        if(QString(mbr.particiones[i].nombre) == name && mbr.particiones[i].estado == 'U'){
            indice_eliminar = i;
            break;
        }
    }

    if(indice_eliminar == -1){
        qDebug() << "Error: No existe una partición con ese nombre";
        return false;
    }

    Particion particion_eliminar = mbr.particiones[indice_eliminar];

    // Si es extendida se eliminan todas las logicas dentro
    if(particion_eliminar.tipo == 'E'){
        qDebug() << "Eliminando partición extendida y sus particiones lógicas";

        // Marcar todas las logicas como libres
        for(int i = 0; i < mbr.num_particiones; i++){
            if(mbr.particiones[i].tipo == 'L' && mbr.particiones[i].estado == 'U'){
                mbr.particiones[i].estado = 'L';
                strcpy(mbr.particiones[i].nombre, "");
                mbr.particiones[i].tamanio = 0;
                mbr.particiones[i].inicio = 0;
            }
        }
    }

    // Marcar la particion como libre
    mbr.particiones[indice_eliminar].estado = 'L';
    strcpy(mbr.particiones[indice_eliminar].nombre, "");
    mbr.particiones[indice_eliminar].tamanio = 0;
    mbr.particiones[indice_eliminar].inicio = 0;


    // Reorganizar el array de particiones
    int escritura = 0;
    for(int lectura = 0; lectura < 4; lectura++){
        if(mbr.particiones[lectura].estado == 'U'){
            if(escritura != lectura){
                mbr.particiones[escritura] = mbr.particiones[lectura];
            }
            escritura++;
        }
    }

    // Limpiar las posiciones sobrantes
    for(int i = escritura; i < 4; i++){
        mbr.particiones[i].estado = 'L';
        strcpy(mbr.particiones[i].nombre, "");
        mbr.particiones[i].tamanio = 0;
        mbr.particiones[i].inicio = 0;
    }

    mbr.num_particiones = escritura;

    qDebug() << "Partición eliminada exitosamente";
    return true;
}

// modificar
bool DiskManager::modificarTamanioParticion(MBR &mbr, map<QString, QString> parametros){
    QString name = parametros["-name"];
    int add = parametros["-add"].toInt();
    QString unit = parametros["-unit"].isEmpty() ? "k" : parametros["-unit"];

    // Convertir el tamaño a bytes
    long long bytes_modificar = add;
    if(unit.toLower() == "m"){
        bytes_modificar = add * 1024 * 1024;
    }else if(unit.toLower() == "k"){
        bytes_modificar = add * 1024;
    }

    // Buscar la partición
    int indice = -1;
    for(int i = 0; i < mbr.num_particiones; i++){
        if(QString(mbr.particiones[i].nombre) == name && mbr.particiones[i].estado == 'U'){
            indice = i;
            break;
        }
    }

    if(indice == -1){
        qDebug() << "Error: No existe una partición con ese nombre";
        return false;
    }

    Particion &particion = mbr.particiones[indice];

    // CASO 1: AGREGAR espacio (add positivo)
    if(bytes_modificar > 0){
        // Encontrar el final de esta partición
        long long fin_particion = particion.inicio + particion.tamanio;

        // Buscar la siguiente partición
        long long inicio_siguiente = mbr.tamanio_disco; // Por defecto es el fin del disco

        for(int i = 0; i < mbr.num_particiones; i++){
            if(mbr.particiones[i].estado == 'U' &&
                mbr.particiones[i].inicio > fin_particion &&
                mbr.particiones[i].inicio < inicio_siguiente){
                inicio_siguiente = mbr.particiones[i].inicio;
            }
        }

        long long espacio_disponible = inicio_siguiente - fin_particion;

        if(bytes_modificar > espacio_disponible){
            qDebug() << "Error: No hay suficiente espacio libre después de la partición";
            qDebug() << "Espacio disponible:" << espacio_disponible << "bytes";
            qDebug() << "Espacio solicitado:" << bytes_modificar << "bytes";
            return false;
        }

        particion.tamanio += bytes_modificar;
        qDebug() << "Se agregaron" << bytes_modificar << "bytes a la partición";
    }
    // CASO 2: quitar espacio (add negativo)
    else if(bytes_modificar < 0){
        long long bytes_quitar = -bytes_modificar; // Convertir a positivo

        if(bytes_quitar >= particion.tamanio){
            qDebug() << "Error: No se puede quitar más espacio del que tiene la partición";
            qDebug() << "Tamaño actual:" << particion.tamanio << "bytes";
            qDebug() << "Intentando quitar:" << bytes_quitar << "bytes";
            return false;
        }

        particion.tamanio -= bytes_quitar;
        qDebug() << "Se quitaron" << bytes_quitar << "bytes de la partición";
    }
    else{
        qDebug() << "Error: El parámetro -add debe ser diferente de 0";
        return false;
    }

    qDebug() << "Partición modificada exitosamente. Nuevo tamaño:" << particion.tamanio << "bytes";
    return true;
}


bool DiskManager::mount(QString path, QString name) {
    QFile archivo(path);
    if(!archivo.exists()) {
        qDebug() << "Error: El disco no existe";
        return false;
    }

    MBR mbr = leerMBR(path);

    // revsar que la partición exista en el mbr
    bool encontrada = false;
    for(int i = 0; i < mbr.num_particiones; i++) {
        if(QString(mbr.particiones[i].nombre) == name &&
            mbr.particiones[i].estado == 'U') {
            encontrada = true;
            break;
        }
    }

    if(!encontrada) {
        qDebug() << "Error: La partición no existe en el disco";
        return false;
    }

    // ver que no este mount ya
    if(estaMontada(path, name)) {
        qDebug() << "Error: La partición ya está montada";
        return false;
    }

    // hacer id
    QString id = generarID(path);

    //guardar en la lista de montajes
    ParticionMontada montaje;
    strcpy(montaje.id, id.toStdString().c_str());
    strcpy(montaje.path_disco, path.toStdString().c_str());
    strcpy(montaje.nombre_particion, name.toStdString().c_str());
    montaje.letra = id[2].toLatin1();  // Extraer la letra de "vda1"
    montaje.numero = id.mid(3).toInt(); // Extraer el número

    particiones_montadas.append(montaje);

    qDebug() << "Partición montada con ID:" << id;
    return true;
}


QString DiskManager::generarID(QString path) {
    char letra = 'a';

    // PASO 1: Buscar si este DISCO ya tiene montajes para obtener su letra
    bool discoEncontrado = false;
    for(const ParticionMontada& montaje : particiones_montadas) {
        if(QString(montaje.path_disco) == path) {
            letra = montaje.letra;
            discoEncontrado = true;
            break;
        }
    }

    // PASO 2: Si es un disco nuevo, asignar una letra libre
    if(!discoEncontrado && particiones_montadas.size() > 0) {
        QSet<char> letrasUsadas;
        for(const ParticionMontada& montaje : particiones_montadas) {
            letrasUsadas.insert(montaje.letra);
        }

        letra = 'a';
        while(letrasUsadas.contains(letra)) {
            letra++;
        }
    }

    // PASO 3: Buscar el PRIMER número disponible para esta letra
    QSet<int> numerosUsados;
    for(const ParticionMontada& montaje : particiones_montadas) {
        if(montaje.letra == letra) {
            numerosUsados.insert(montaje.numero);
        }
    }

    int numero = 1;
    while(numerosUsados.contains(numero)) {
        numero++;
    }

    return QString("vd") + QString(letra) + QString::number(numero);
}

bool DiskManager::estaMontada(QString path, QString name) {
    // Recorrer todas las particiones montadas
    for(const ParticionMontada& montaje : particiones_montadas) {
        // Comparar si coinciden AMBOS: disco Y nombre de partición
        if(QString(montaje.path_disco) == path &&
            QString(montaje.nombre_particion) == name) {
            return true;  // ¡Ya está montada!
        }
    }

    return false;  // No está montada
}

bool DiskManager::unmount(QString id) {
    // Buscar el montaje con ese ID
    for(int i = 0; i < particiones_montadas.size(); i++) {
        if(QString(particiones_montadas[i].id) == id) {
            // ¡Encontrado! Eliminarlo de la lista
            particiones_montadas.removeAt(i);
            qDebug() << "Partición desmontada:" << id;
            return true;
        }
    }

    // No se encontró el ID
    qDebug() << "Error: No existe una partición montada con ese ID";
    return false;
}

QString DiskManager::obtenerTablaParticionesMontadas() {
    QString tabla = "";

    // Línea superior
    tabla += "    ┌────────────────────────────────────────┐\n";
    tabla += "    │            Particiones montadas        │\n";
    tabla += "    ├──────────────────┬─────────────────────┤\n";
    tabla += "    │     Nombre       │         ID          │\n";
    tabla += "    ├──────────────────┼─────────────────────┤\n";

    // Contenido de particiones
    for(const ParticionMontada& m : particiones_montadas) {
        QString nombre = QString(m.nombre_particion);
        QString id = QString(m.id);

        // Centrar el nombre
        int espaciosNombre = (16 - nombre.length()) / 2;
        QString nombreCentrado = QString(" ").repeated(espaciosNombre) + nombre;
        nombreCentrado = nombreCentrado.leftJustified(16);

        // Centrar el ID
        int espaciosId = (19 - id.length()) / 2;
        QString idCentrado = QString(" ").repeated(espaciosId) + id;
        idCentrado = idCentrado.leftJustified(19);

        tabla += "    │" + nombreCentrado + "  │" + idCentrado + "  │\n";
    }

    // Línea inferior
    tabla += "    └──────────────────┴─────────────────────┘\n";

    return tabla;
}


void DiskManager::mostrarReporteEnConsola(QString path, QPlainTextEdit *consola)
{
    QFile archivo(path);
    if(!archivo.exists()) {
        consola->appendPlainText("Error: El disco no existe");
        return;
    }

    MBR mbr = leerMBR(path);

    // Crear el widget del diagrama
    DiagramaWidget *diagrama = new DiagramaWidget(mbr, consola);
    diagrama->setFixedSize(975, 140);

    // Calcular posición
    QFontMetrics fm(consola->font());
    int lineHeight = fm.lineSpacing();
    int numLineas = consola->document()->lineCount();
    int posY = (numLineas * lineHeight) + 5;

    // Centrar horizontalmente
    int posX = (consola->width() - 1100) / 2;
    if(posX < 10) posX = 10;

    diagrama->move(posX, posY);
    diagrama->show();

    // Reservar espacio
    consola->appendPlainText("\n\n\n\n\n\n\n\n");
}





void DiskManager::guardarImagenDisco(MBR mbr, QString pathDestino)
{
    // Crear el directorio si no existe
    crearCarpeta(pathDestino);

    // Usar DiagramaWidget para generar la imagen
    DiagramaWidget *temp = new DiagramaWidget(mbr, nullptr);
    QPixmap imagen = temp->crearImagenDiagrama(mbr);

    // Guardar la imagen
    if(imagen.save(pathDestino)) {
        qDebug() << "Imagen guardada exitosamente en:" << pathDestino;
    } else {
        qDebug() << "Error al guardar la imagen";
    }

    delete temp;
}

bool DiskManager::generarReporteDisco(map<QString, QString> parametros, QPlainTextEdit *consola)
{
    QString pathDisco;
    QString pathDestino = parametros["-path"]; // Donde se guarda la imagen
    QString name = parametros["-name"];

    // VALIDAR que sea reporte de disco/mbr
    if(name.toLower() != "disk" && name.toLower() != "mbr") {
        consola->appendPlainText("Error: Este comando solo soporta -name=disk o -name=mbr");
        return false;
    }

    // CASO 1: Usar -id (partición montada)
    if(!parametros["-id"].isEmpty()) {
        QString id = parametros["-id"];

        // Buscar en particiones montadas
        bool encontrado = false;
        for(const ParticionMontada& montaje : particiones_montadas) {
            if(QString(montaje.id) == id) {
                pathDisco = QString(montaje.path_disco);
                encontrado = true;
                break;
            }
        }

        if(!encontrado) {
            consola->appendPlainText("Error: No existe una partición montada con ese ID");
            return false;
        }
    }
    // CASO 2: Usar -path_disco (path directo al disco)
    else if(!parametros["-path_disco"].isEmpty()) {
        pathDisco = parametros["-path_disco"];
    }
    else {
        consola->appendPlainText("Error: Debe especificar -id o -path_disco");
        return false;
    }

    // Verificar que el disco existe
    QFile archivo(pathDisco);
    if(!archivo.exists()) {
        consola->appendPlainText("Error: El disco no existe en la ruta: " + pathDisco);
        return false;
    }

    // Leer el MBR
    MBR mbr = leerMBR(pathDisco);

    // MOSTRAR en consola
    consola->appendPlainText("\n╔═══════════════════════════════════════════════════════════════════════════╗");
    consola->appendPlainText("║                         REPORTE DE DISCO (MBR)                            ║");
    consola->appendPlainText("╚═══════════════════════════════════════════════════════════════════════════╝\n");

    mostrarReporteEnConsola(pathDisco, consola);

    // GUARDAR como imagen
    guardarImagenDisco(mbr, pathDestino);

    consola->appendPlainText("\nReporte generado con éxito");
    consola->appendPlainText("Imagen guardada en: " + pathDestino);

    return true;
}
