#include "diagramawidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

DiagramaWidget::DiagramaWidget(MBR mbr, QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: white;");
    setMinimumHeight(140);
    setMaximumHeight(140);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Crear la imagen del diagrama
    QPixmap pixmap = crearImagenDiagrama(mbr);

    QLabel *labelImagen = new QLabel();
    labelImagen->setPixmap(pixmap);
    labelImagen->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    layout->addWidget(labelImagen);
}


QPixmap DiagramaWidget::crearImagenDiagrama(MBR mbr)
{
    // Crear imagen con márgenes iguales
    int anchoTotal = 980;
    int altoTotal = 130;
    QPixmap pixmap(anchoTotal, altoTotal);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    //bordes
    QPen pen(QColor(74, 144, 226), 2);
    painter.setPen(pen);
    painter.setBrush(Qt::white);

    // Configurar fuente
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    // margenes
    int margenIzq = 25;
    int margenDer = 25;
    int anchoDisponible = anchoTotal - margenIzq - margenDer;

    int x = margenIzq;
    int y = 10;
    int altura = 110;

    //clasificar las particiones
    Particion* primarias[4] = {nullptr, nullptr, nullptr, nullptr};
    Particion* extendida = nullptr;
    Particion* logicas[2] = {nullptr, nullptr};
    int numPrimarias = 0;
    int numLogicas = 0;

    // Recolectar particiones por tipo
    for(int i = 0; i < mbr.num_particiones; i++) {
        if(mbr.particiones[i].estado == 'U') {
            if(mbr.particiones[i].tipo == 'P') {
                if(numPrimarias < 4) {
                    primarias[numPrimarias] = &mbr.particiones[i];
                    numPrimarias++;
                }
            } else if(mbr.particiones[i].tipo == 'E') {
                extendida = &mbr.particiones[i];
            } else if(mbr.particiones[i].tipo == 'L') {
                if(numLogicas < 2) {
                    logicas[numLogicas] = &mbr.particiones[i];
                    numLogicas++;
                }
            }
        }
    }

    // Ordenar primarias por posición de inicio use sort
    for(int i = 0; i < numPrimarias - 1; i++) {
        for(int j = 0; j < numPrimarias - i - 1; j++) {
            if(primarias[j]->inicio > primarias[j+1]->inicio) {
                Particion* temp = primarias[j];
                primarias[j] = primarias[j+1];
                primarias[j+1] = temp;
            }
        }
    }

    //porcentajes
    double porcentajeMBR = (sizeof(MBR) * 100.0) / mbr.tamanio_disco;

    // Calcular espacio usado total
    long long espacioUsado = sizeof(MBR);
    for(int i = 0; i < numPrimarias; i++) {
        if(primarias[i]) espacioUsado += primarias[i]->tamanio;
    }
    if(extendida) espacioUsado += extendida->tamanio;

    double porcentajeLibreFinal = ((mbr.tamanio_disco - espacioUsado) * 100.0) / mbr.tamanio_disco;

    //el diseño de las particiones segun cuantas sean
    if(extendida && numPrimarias == 0) {
        //solo extendida sin primarias

        int anchoSeccion = anchoDisponible / 15;
        double porcentajeExt = (extendida->tamanio * 100.0) / mbr.tamanio_disco;
        double porcentajeLibreInicio = ((mbr.tamanio_disco - sizeof(MBR) - extendida->tamanio) * 100.0) / mbr.tamanio_disco;

        // MBR
        painter.drawRect(x, y, anchoSeccion, altura);
        painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                         "MBR\n\n" + QString::number(porcentajeMBR, 'f', 2) + "%");
        x += anchoSeccion;

        // Libre inicial
        painter.drawRect(x, y, anchoSeccion, altura);
        painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                         "Libre\n\n" + QString::number(porcentajeLibreInicio, 'f', 1) + "%");
        x += anchoSeccion;

        // Extendida
        int anchoExtendida = anchoSeccion * 11;
        painter.drawRect(x, y, anchoExtendida, altura);
        painter.drawText(QRect(x, y + 5, anchoExtendida, 20), Qt::AlignCenter,
                         "Extendida: " + QString(extendida->nombre));
        painter.drawLine(x, y + 28, x + anchoExtendida, y + 28);

        // Subdividir extendida con lo de logicas
        dibujarLogicasEnExtendida(&painter, x, y, anchoExtendida, altura, logicas, numLogicas, extendida);

        x += anchoExtendida;

        // Libres finales
        painter.drawRect(x, y, anchoSeccion, altura);
        painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
        x += anchoSeccion;

        painter.drawRect(x, y, anchoSeccion, altura);
        if(porcentajeLibreFinal > 0.1) {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             "Libre\n\n" + QString::number(porcentajeLibreFinal, 'f', 1) + "%");
        } else {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
        }
    }
    else if(extendida && numPrimarias <= 2) {
       //extendida mas una o dos primarias
        int anchoSeccion = anchoDisponible / 15;

        double porcentajeP1 = primarias[0] ? (primarias[0]->tamanio * 100.0) / mbr.tamanio_disco : 0;
        double porcentajeExt = (extendida->tamanio * 100.0) / mbr.tamanio_disco;
        double porcentajeP2 = primarias[1] ? (primarias[1]->tamanio * 100.0) / mbr.tamanio_disco : 0;
        double porcentajeLibre1 = primarias[0] ? 0 : ((mbr.tamanio_disco - sizeof(MBR)) * 100.0) / mbr.tamanio_disco;

        // MBR
        painter.drawRect(x, y, anchoSeccion, altura);
        painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                         "MBR\n\n" + QString::number(porcentajeMBR, 'f', 2) + "%");
        x += anchoSeccion;

        // Primaria 1 o Libre
        painter.drawRect(x, y, anchoSeccion, altura);
        if(primarias[0]) {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             QString(primarias[0]->nombre) + "\n\n" + QString::number(porcentajeP1, 'f', 1) + "%");
        } else {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             "Libre\n\n" + QString::number(porcentajeLibre1, 'f', 1) + "%");
        }
        x += anchoSeccion;

        // Extendida
        int anchoExtendida = anchoSeccion * 11;
        painter.drawRect(x, y, anchoExtendida, altura);
        painter.drawText(QRect(x, y + 5, anchoExtendida, 20), Qt::AlignCenter,
                         "Extendida: " + QString(extendida->nombre));
        painter.drawLine(x, y + 28, x + anchoExtendida, y + 28);

        // Subdividir extendida
        dibujarLogicasEnExtendida(&painter, x, y, anchoExtendida, altura, logicas, numLogicas, extendida);

        x += anchoExtendida;

        // Primaria 2 o Libre
        painter.drawRect(x, y, anchoSeccion, altura);
        if(primarias[1]) {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             QString(primarias[1]->nombre) + "\n\n" + QString::number(porcentajeP2, 'f', 1) + "%");
        } else {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
        }
        x += anchoSeccion;

        // Libre final
        painter.drawRect(x, y, anchoSeccion, altura);
        if(porcentajeLibreFinal > 0.1) {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             "Libre\n\n" + QString::number(porcentajeLibreFinal, 'f', 1) + "%");
        } else {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
        }
    }
    else if(extendida && numPrimarias == 3) {
        //tres primarias y extendida

        int anchoSeccion = anchoDisponible / 7;

        double porcentajeExt = (extendida->tamanio * 100.0) / mbr.tamanio_disco;

        // MBR
        painter.drawRect(x, y, anchoSeccion, altura);
        painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                         "MBR\n\n" + QString::number(porcentajeMBR, 'f', 2) + "%");
        x += anchoSeccion;

        // 3 Primarias
        for(int i = 0; i < 3; i++) {
            painter.drawRect(x, y, anchoSeccion, altura);
            if(primarias[i]) {
                double porcentaje = (primarias[i]->tamanio * 100.0) / mbr.tamanio_disco;
                painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                                 QString(primarias[i]->nombre) + "\n\n" + QString::number(porcentaje, 'f', 1) + "%");
            } else {
                painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
            }
            x += anchoSeccion;
        }

        // Extendida (2 unidades)
        int anchoExtendida = anchoSeccion * 2;
        painter.drawRect(x, y, anchoExtendida, altura);

        // Mostrar porcentaje TOTAL de la extendida
        painter.drawText(QRect(x, y + 5, anchoExtendida, 20), Qt::AlignCenter,
                         "Ext: " + QString(extendida->nombre) + " - " + QString::number(porcentajeExt, 'f', 1) + "%");
        painter.drawLine(x, y + 28, x + anchoExtendida, y + 28);

        // Subdividir extendida mostrando NOMBRES de lógicas (sin porcentajes)
        int xLogica = x + 2;
        int yLogica = y + 30;
        int alturaLogica = altura - 32;
        int anchoTotalLogicas = anchoExtendida - 4;

        if(numLogicas == 2) {
            // Dividir en 2 partes iguales
            int anchoLogica = anchoTotalLogicas / 2;

            // Lógicas
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                              Qt::AlignCenter, QString(logicas[0]->nombre));
            xLogica += anchoLogica;


            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                              Qt::AlignCenter, QString(logicas[1]->nombre));
        }
        else if(numLogicas == 1) {
            // Una sola lógica ocupa todo el espacio
            painter.drawRect(xLogica, yLogica, anchoTotalLogicas, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoTotalLogicas, alturaLogica),
                              Qt::AlignCenter, QString(logicas[0]->nombre));
        }
        else {
            // Sin lógicas: todo libre dentro de la extendida
            painter.drawRect(xLogica, yLogica, anchoTotalLogicas, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoTotalLogicas, alturaLogica),
                              Qt::AlignCenter, "Libre\n100%");
        }

        x += anchoExtendida;

        // Libre final
        painter.drawRect(x, y, anchoSeccion, altura);
        if(porcentajeLibreFinal > 0.1) {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter,
                             "Libre\n\n" + QString::number(porcentajeLibreFinal, 'f', 1) + "%");
        } else {
            painter.drawText(QRect(x, y, anchoSeccion, altura), Qt::AlignCenter, "Libre");
        }
    }

    painter.end();
    return pixmap;
}

//dibujar logicas dentro las extendidas
void DiagramaWidget::dibujarLogicasEnExtendida(QPainter* painter, int x, int y,
                                               int anchoExtendida, int altura,
                                               Particion* logicas[], int numLogicas,
                                               Particion* extendida)
{
    int xLogica = x + 2;
    int yLogica = y + 30;
    int alturaLogica = altura - 32;

    if(numLogicas == 2) {
        // 2 lógica se divide  en 5 partes
        int anchoLogica = (anchoExtendida - 4) / 5;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica), Qt::AlignCenter, "Libre");
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                          Qt::AlignCenter, QString(logicas[0]->nombre));
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica), Qt::AlignCenter, "Libre");
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                          Qt::AlignCenter, QString(logicas[1]->nombre));
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica), Qt::AlignCenter, "Libre");
    }
    else if(numLogicas == 1) {
        // 1 lógic s divide  en 3 partes
        int anchoLogica = (anchoExtendida - 4) / 3;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica), Qt::AlignCenter, "Libre");
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                          Qt::AlignCenter, QString(logicas[0]->nombre));
        xLogica += anchoLogica;

        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica), Qt::AlignCenter, "Libre");
    }
    else {
        // Sin lógicas todo libre
        int anchoLogica = anchoExtendida - 4;
        painter->drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
        painter->drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                          Qt::AlignCenter, "Libre\n100%");
    }
}
