#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFont>
#include <QKeyEvent>
#include <QStringList>
#include <QTextCursor>
#include <QDir>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    diskManager = new DiskManager();

    // Apariencia CMD
    QFont font("Consolas", 11);
    font.setFixedPitch(true);
    ui->base->setFont(font);
    ui->base->setStyleSheet("background-color: black; color: white; border: none;");

    currentPath = "C:/";

    QString header = "-------------------------------------------------- Sistema de archivos -------------------------------------------------\n";
    header += "                                              Katherine Carvallo - 22441130\n";
    header += "------------------------------------------------------------------------------------------------------------------------\n\n";
    header += "Por favor escriba algún comando:\n";
    header += ">> ";

    ui->base->setPlainText(header);
    ui->base->installEventFilter(this);


    QTextCursor cursor = ui->base->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->base->setTextCursor(cursor);
}

MainWindow::~MainWindow()
{
    delete diskManager;
    delete ui;
}



void MainWindow::processCommand(const QString &cmd)
{
    QString trimmedCmd = cmd.trimmed();

    if (trimmedCmd.isEmpty())
    {
        ui->base->appendPlainText(currentPath + ">> ");
        return;
    }

    //verificar eliminar la partición
    if (!pendingPartitionDelete.empty()) {
        QString respuesta = trimmedCmd.toLower();

        if (respuesta == "s" || respuesta == "si" || respuesta == "y" || respuesta == "yes") {
            bool exito = diskManager->fdisk(pendingPartitionDelete);

            if(exito){
                ui->base->appendPlainText("Partición eliminada exitosamente: " +
                                          pendingPartitionDelete["-name"]);
            }else{
                ui->base->appendPlainText("No se pudo eliminar la partición");
            }

            pendingPartitionDelete.clear();
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        } else if (respuesta == "n" || respuesta == "no") {
            ui->base->appendPlainText("Operación cancelada.");
            pendingPartitionDelete.clear();
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        } else {
            ui->base->appendPlainText("Respuesta no válida. Escriba S o N.");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }
    }

    // verificación de si desea eliminar
    if (!pendingDiskToDelete.isEmpty()) {
        QString respuesta = trimmedCmd.toLower();

        if (respuesta == "s" || respuesta == "si" || respuesta == "y" || respuesta == "yes") {
            bool eliminar = diskManager->eliminarDisco(pendingDiskToDelete);

            if(!eliminar){
                ui->base->appendPlainText("No se pudo borrar el disco: " + pendingDiskToDelete);
            }else{
                ui->base->appendPlainText("Se borro exitosamente");
            }

            pendingDiskToDelete = "";  // Limpiar la variable
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        } else if (respuesta == "n" || respuesta == "no") {
            ui->base->appendPlainText("Operación cancelada.");
            pendingDiskToDelete = "";
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        } else {
            ui->base->appendPlainText("Respuesta no válida. Escriba S o N.");
            ui->base->appendPlainText(currentPath + ">> ");
            return;  // Vuelve a pedir sin limpiar pendingDiskToDelete
        }
    }

    //obtener el comando - la primera palabra
    QStringList partes = trimmedCmd.split(" ", Qt::SkipEmptyParts);
    QString command = partes[0].toLower();

    //comando 1. mkdisk
    if(command == "mkdisk"){
        //extraemos parametros
        map <QString, QString> parametros = extraerParametros(trimmedCmd);

        // VALIDAR -size
        if (parametros["-size"].isEmpty()) {
            ui->base->appendPlainText("Error: -size es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        bool ok;
        int size = parametros["-size"].toInt(&ok);
        if (!ok || size <= 0) {
            ui->base->appendPlainText("Error: -size debe ser un número mayor que 0");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // VALIDAR -path
        if (parametros["-path"].isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // VALIDAR -unit si existe
        QString unit = parametros["-unit"];
        if (unit.isEmpty()) {
            unit = "M";
        }
        if (unit != "K" && unit != "M" && unit != "k" && unit != "m") {
            ui->base->appendPlainText("Error: -unit debe ser K o M");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // VALIDAR -fit si existe
        QString fit = parametros["-fit"].toLower();
        if (fit.isEmpty()) {
            fit = "ff";  // el valor si no pone nada
        }
        if (fit != "bf" && fit != "ff" && fit != "wf") {
            ui->base->appendPlainText("Error: -fit debe ser BF, FF o WF");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        //crear disco luego de las validaciones
        int tam = parametros["-size"].toInt();
        QString path = parametros["-path"];

        bool exito = diskManager->crearDisco(tam, unit, path, fit);

        if(exito){
            ui->base->appendPlainText("Disco creado exitosamente: "+path);
        }else {
            ui->base->appendPlainText("No se pudo crear el disco");
        }
    }

    else if(command == "rmdisk"){
        map <QString, QString> parametros = extraerParametros(trimmedCmd);
        // VALIDAR -path
        QString path = parametros["-path"];
        if (path.isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        ui->base->appendPlainText("¿Está seguro que desea eliminar el disco: " + path + "? (S/N)");
        pendingDiskToDelete = path;
        ui->base->appendPlainText(currentPath + ">> ");



    }   else if (command == "fdisk") {
        map<QString, QString> parametros = extraerParametros(trimmedCmd);

        // VALIDAR -path - obli
        if (parametros["-path"].isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // VALIDAR -name obli
        if (parametros["-name"].isEmpty()) {
            ui->base->appendPlainText("Error: -name es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // elige que operación de fdisk va a hacer
        if (!parametros["-delete"].isEmpty()) {
            QString deleteType = parametros["-delete"].toLower();

            if (deleteType != "fast" && deleteType != "full") {
                ui->base->appendPlainText("Error: -delete debe ser Fast o Full");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Mostrar confirmación
            ui->base->appendPlainText("¿Está seguro que desea eliminar la partición '" +
                                      parametros["-name"] + "'? (S/N)");

            // Guardar en una variable temporal para confirmar después
            // (Necesitarás agregar variables miembro en mainwindow.h)
            pendingPartitionDelete = parametros;
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }
        // ===== OPCIÓN 2: ADD (modificar tamaño) =====
        else if (!parametros["-add"].isEmpty()) {
            bool ok;
            int add = parametros["-add"].toInt(&ok);

            if (!ok || add == 0) {
                ui->base->appendPlainText("Error: -add debe ser un número diferente de 0");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Validar -unit (opcional)
            QString unit = parametros["-unit"];
            if (unit.isEmpty()) {
                unit = "k";
            }
            unit = unit.toLower();
            if (unit != "k" && unit != "m" && unit != "b") {
                ui->base->appendPlainText("Error: -unit debe ser B, K o M");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Ejecutar modificación
            bool exito = diskManager->fdisk(parametros);

            if(exito){
                if(add > 0){
                    ui->base->appendPlainText("Se agregó espacio a la partición: " + parametros["-name"]);
                }else{
                    ui->base->appendPlainText("Se quitó espacio de la partición: " + parametros["-name"]);
                }
            } else {
                ui->base->appendPlainText("No se pudo modificar el tamaño de la partición");

            }
        }else if (!parametros["-size"].isEmpty()) {
            // CREAR PARTICIÓN

            // Validar -size
            bool ok;
            int size = parametros["-size"].toInt(&ok);
            if (!ok || size <= 0) {
                ui->base->appendPlainText("Error: -size debe ser un número mayor que 0");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Validar -unit
            QString unit = parametros["-unit"];
            if (unit.isEmpty()) {
                unit = "k";
            }
            if (unit != "k" && unit != "m" && unit != "b") {
                ui->base->appendPlainText("Error: -unit debe ser B, K o M");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Validar -type
            QString type = parametros["-type"];
            if (type.isEmpty()) {
                type = "p";
            }
            if (type != "p" && type != "e" && type != "l") {
                ui->base->appendPlainText("Error: -type debe ser P, E o L");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // Validar -fit
            QString fit = parametros["-fit"];
            if (fit.isEmpty()) {
                fit = "wf";
            }
            if (fit != "bf" && fit != "ff" && fit != "wf") {
                ui->base->appendPlainText("Error: -fit debe ser BF, FF o WF");
                ui->base->appendPlainText(currentPath + ">> ");
                return;
            }

            // LLAMAR a fdisk
            bool exito = diskManager->fdisk(parametros);

            if(exito){
                ui->base->appendPlainText("Partición creada exitosamente: " + parametros["-name"]);
            } else {
                ui->base->appendPlainText("No se pudo crear la partición");
            }
        }
        else {
            ui->base->appendPlainText("Error: Debe especificar -size, -delete o -add");
        }
    }else if (command == "mount") {
        map<QString, QString> parametros = extraerParametros(trimmedCmd);

        // VALIDAR -path (obligatorio)
        if (parametros["-path"].isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        // VALIDAR -name (obligatorio)
        if (parametros["-name"].isEmpty()) {
            ui->base->appendPlainText("Error: -name es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        QString path = parametros["-path"];
        QString name = parametros["-name"];

        // EJECUTAR mount
        bool exito = diskManager->mount(path, name);

        if (exito) {
            ui->base->appendPlainText("Partición montada exitosamente");
            QString tabla = diskManager->obtenerTablaParticionesMontadas();
            ui->base->appendPlainText(tabla);
        } else {
            ui->base->appendPlainText("No se pudo montar la partición");
        }
    }
    else if (command == "unmount") {
        map<QString, QString> parametros = extraerParametros(trimmedCmd);

        // VALIDAR -id (obligatorio)
        if (parametros["-id"].isEmpty()) {
            ui->base->appendPlainText("Error: -id es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        QString id = parametros["-id"];

        // EJECUTAR unmount
        bool exito = diskManager->unmount(id);

        if (exito) {
            ui->base->appendPlainText("Partición desmontada exitosamente: " + id);
        } else {
            ui->base->appendPlainText("Error: No existe una partición con ID: " + id);
        }
    }
    else if (command == "cd"){
        if (partes.size() > 1){
            applyCd(partes[1]);
        }
        else{
            ui->base->appendPlainText("Se requiere un parámetro de ruta.");
        }
    }else if (command == "clear") {
        ui->base->clear();
        QString header = "-------------------------------------------------- Sistema de archivos -------------------------------------------------\n";
        header += "                                              Katherine Carvallo - 22441130\n";
        header += "------------------------------------------------------------------------------------------------------------------------\n\n";
        header += currentPath + ">> ";
        ui->base->setPlainText(header);
        return;
    }
    /*else if (command == "rep") {
        map<QString, QString> parametros = extraerParametros(trimmedCmd);

        if (parametros["-path"].isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        QString path = parametros["-path"];

        ui->base->appendPlainText("\n═══════════════════ REPORTE DE DISCO ═══════════════════\n");
        diskManager->mostrarReporteEnConsola(path, ui->base);
        ui->base->appendPlainText("\n════════════════════════════════════════════════════════\n");
    }*/
    else if (command == "rep") {
        map<QString, QString> parametros = extraerParametros(trimmedCmd);

        if (parametros["-path"].isEmpty()) {
            ui->base->appendPlainText("Error: -path es obligatorio");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        QString path = parametros["-path"];

        QFile archivo(path);
        if(!archivo.exists()) {
            ui->base->appendPlainText("Error: El disco no existe en la ruta especificada");
            ui->base->appendPlainText(currentPath + ">> ");
            return;
        }

        ui->base->appendPlainText("\n╔═══════════════════════════════════════════════════════════════════════════╗");
        ui->base->appendPlainText("║                         REPORTE DE DISCO (MBR)                            ║");
        ui->base->appendPlainText("╚═══════════════════════════════════════════════════════════════════════════╝\n");

        diskManager->mostrarReporteEnConsola(path, ui->base);
    }
    else {
        ui->base->appendPlainText("'" + command + "' no es reconocido como un comando interno o externo.");
    }

    ui->base->appendPlainText(currentPath + "> ");
}

bool MainWindow::applyCd(QString arg)
{
    arg = arg.trimmed();
    arg.replace("\\", "/");  //cambiar barras

    QString newPath = currentPath;

    //ruta absoluuta
    if (arg.contains(":"))
    {
        newPath = arg;

        // Asegurar barra final
        if (!newPath.endsWith("/"))
            newPath += "/";

        currentPath = newPath;
        return true;
    }


    if (arg == "..")
    {

        if (newPath == "/")
            return true;


        newPath.chop(1);
        int idx = newPath.lastIndexOf('/');

        if (idx != -1)
        {
            newPath = newPath.left(idx + 1);
        }
        else
        {
            newPath = "/";
        }

        currentPath = newPath;
        return true;
    }


    if (arg.startsWith("../"))
    {
        QStringList parts = arg.split("/", Qt::SkipEmptyParts);

        for (const QString &p : parts)
        {
            if (p == "..")
            {
                if (newPath != "/")
                {
                    newPath.chop(1);
                    int idx = newPath.lastIndexOf('/');
                    newPath = newPath.left(idx + 1);
                }
            }
        }

        currentPath = newPath;
        return true;
    }


    if (!newPath.endsWith("/"))
        newPath += "/";

    newPath += arg + "/";

    currentPath = newPath;
    return true;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->base && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        //  procesar comando
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            QString text = ui->base->toPlainText();
            QString lastLine = text.split("\n").last();
            QString cmd = lastLine.mid(lastLine.indexOf("> ") + 2);

            processCommand(cmd);


            QTextCursor cursor = ui->base->textCursor();
            cursor.movePosition(QTextCursor::End);
            ui->base->setTextCursor(cursor);

            return true;
        }

        // evitar borrar el prompt
        if (keyEvent->key() == Qt::Key_Backspace)
        {
            QString lastLine = ui->base->toPlainText().split("\n").last();
            if (lastLine.length() <= currentPath.length() + 2)
                return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

/*map<QString, QString> MainWindow:: extraerParametros(QString linea){
     map <QString, QString> resultado; //guardaremos lo de las parejas

     linea.replace("–", "-");
     linea.replace("—", "-");

     QStringList partes = linea.split(" "); //separamos por espacios



     //como nos devuelven un tipo de "arreglo" lo vamos a recorrer con un for para guardar los resultados
     //sintaxis for each tipo de dato nombre var: arreglo
     for(QString parte : partes){
          parte = parte.trimmed();
         //en cada pedazo revisamos si comienza con - ya que es un parametro
         if(parte.startsWith("-")){
             //dividir el parametro en dos para saber categoria y el argumento
             QStringList dividido = parte.split("=");
             if(dividido.size() == 2){
                 //si tiene dos pedazos sacamos clave y valor y lo agregamos al mapa
                 //QString clave = dividido[0].trimmed();
                 QString valor = dividido[1].trimmed();
                 resultado[clave] =  valor;
                 QString clave = dividido[0].trimmed().toLower();
                 QString valor = dividido[1].trimmed().toLower();
                 resultado[clave] = valor;

             }
         }
     }
     return resultado;
 }

*/

map<QString, QString> MainWindow::extraerParametros(QString linea)
{
    map<QString, QString> resultado;

    linea.replace("–", "-");
    linea.replace("—", "-");

    // Procesar manualmente respetando comillas
    QString parametroActual;
    bool dentroComillas = false;

    for (int i = 0; i < linea.length(); i++)
    {
        QChar c = linea[i];

        if (c == '"')
        {
            dentroComillas = !dentroComillas;
        }
        else if (c == ' ' && !dentroComillas)
        {
            if (!parametroActual.isEmpty())
            {
                procesarParametro(parametroActual, resultado);
                parametroActual.clear();
            }
        }
        else
        {
            parametroActual += c;
        }
    }

    // Procesar el último parámetro
    if (!parametroActual.isEmpty())
    {
        procesarParametro(parametroActual, resultado);
    }

    return resultado;
}

void MainWindow::procesarParametro(QString parametro, map<QString, QString> &resultado)
{
    if (parametro.startsWith("-"))
    {
        QStringList dividido = parametro.split("=");
        if (dividido.size() == 2)
        {
            QString clave = dividido[0].trimmed().toLower();
            //QString valor = dividido[1].trimmed().toLower();
            QString valor = dividido[1].trimmed();

            // Quitar comillas si existen
            if (valor.startsWith("\"") && valor.endsWith("\""))
            {
                valor = valor.mid(1, valor.length() - 2);
            }

            resultado[clave] = valor;
        }
    }
}
