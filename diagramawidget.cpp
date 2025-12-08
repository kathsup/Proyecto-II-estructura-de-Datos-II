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

    // Configurar pen (borde azul)
    QPen pen(QColor(74, 144, 226), 2);
    painter.setPen(pen);
    painter.setBrush(Qt::white);

    // Configurar fuente
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    // MÁRGENES IGUALES
    int margenIzq = 25;
    int margenDer = 25;
    int anchoDisponible = anchoTotal - margenIzq - margenDer;

    int x = margenIzq;
    int y = 10;
    int altura = 110;

    // ANÁLISIS DEL MBR - Buscar qué particiones existen
    Particion* primaria1 = nullptr;
    Particion* extendida = nullptr;
    Particion* primaria2 = nullptr;
    Particion* logica1 = nullptr;
    Particion* logica2 = nullptr;

    // Ordenar particiones por inicio y tipo
    for(int i = 0; i < mbr.num_particiones; i++) {
        if(mbr.particiones[i].estado == 'U') {
            if(mbr.particiones[i].tipo == 'P') {
                if(!primaria1) {
                    primaria1 = &mbr.particiones[i];
                } else if(!primaria2) {
                    primaria2 = &mbr.particiones[i];
                }
            } else if(mbr.particiones[i].tipo == 'E') {
                extendida = &mbr.particiones[i];
            } else if(mbr.particiones[i].tipo == 'L') {
                if(!logica1) {
                    logica1 = &mbr.particiones[i];
                } else if(!logica2) {
                    logica2 = &mbr.particiones[i];
                }
            }
        }
    }

    // Calcular porcentajes
    double porcentajeMBR = (sizeof(MBR) * 100.0) / mbr.tamanio_disco;
    double porcentajeP1 = primaria1 ? (primaria1->tamanio * 100.0) / mbr.tamanio_disco : 0;
    double porcentajeExt = extendida ? (extendida->tamanio * 100.0) / mbr.tamanio_disco : 0;
    double porcentajeP2 = primaria2 ? (primaria2->tamanio * 100.0) / mbr.tamanio_disco : 0;

    // Calcular espacios libres
    long long espacioUsado = sizeof(MBR);
    if(primaria1) espacioUsado += primaria1->tamanio;
    if(extendida) espacioUsado += extendida->tamanio;
    if(primaria2) espacioUsado += primaria2->tamanio;

    double porcentajeLibre1 = primaria1 ? 0 : ((mbr.tamanio_disco - sizeof(MBR)) * 100.0) / mbr.tamanio_disco;
    double porcentajeLibreFinal = ((mbr.tamanio_disco - espacioUsado) * 100.0) / mbr.tamanio_disco;

    // Porcentajes dentro de la extendida
    double porcentajeL1 = 0, porcentajeL2 = 0, porcentajeLibreExt = 0;
    if(extendida) {
        long long espacioLogicas = 0;
        if(logica1) {
            espacioLogicas += logica1->tamanio;
            porcentajeL1 = (logica1->tamanio * 100.0) / extendida->tamanio;
        }
        if(logica2) {
            espacioLogicas += logica2->tamanio;
            porcentajeL2 = (logica2->tamanio * 100.0) / extendida->tamanio;
        }
        porcentajeLibreExt = ((extendida->tamanio - espacioLogicas) * 100.0) / extendida->tamanio;
    }

    // DIVIDIR EN SECCIONES (estructura fija del diseño original)
    int anchoSeccion = anchoDisponible / 15;

    // ===== CUADRO 1: MBR (1 unidad) =====
    int anchoMBR = anchoSeccion;
    painter.drawRect(x, y, anchoMBR, altura);

    QString textoMBR = "MBR\n\n" + QString::number(porcentajeMBR, 'f', 2) + "%";
    painter.drawText(QRect(x, y, anchoMBR, altura), Qt::AlignCenter, textoMBR);
    x += anchoMBR;

    // ===== CUADRO 2: PRIMARIA 1 o LIBRE (1 unidad) =====
    int anchoP1 = anchoSeccion;
    painter.drawRect(x, y, anchoP1, altura);

    if(primaria1) {
        QString textoP1 = QString(primaria1->nombre) + "\n\n" +
                          QString::number(porcentajeP1, 'f', 1) + "%";
        painter.drawText(QRect(x, y, anchoP1, altura), Qt::AlignCenter, textoP1);
    } else {
        QString textoLibre = "Libre\n\n" + QString::number(porcentajeLibre1, 'f', 1) + "%";
        painter.drawText(QRect(x, y, anchoP1, altura), Qt::AlignCenter, textoLibre);
    }
    x += anchoP1;

    // ===== CUADRO 3: EXTENDIDA (11 unidades) =====
    int anchoExtendida = anchoSeccion * 11;

    if(extendida) {
        // Rectángulo exterior de la extendida
        painter.drawRect(x, y, anchoExtendida, altura);

        // Texto "Extendida" arriba
        QString textoExt = "Extendida: " + QString(extendida->nombre);
        painter.drawText(QRect(x, y + 5, anchoExtendida, 20), Qt::AlignCenter, textoExt);

        // Línea divisoria debajo de "Extendida"
        painter.drawLine(x, y + 28, x + anchoExtendida, y + 28);

        // SUBDIVIDIR LA EXTENDIDA
        int xLogica = x + 2;
        int yLogica = y + 30;
        int alturaLogica = altura - 32;

        // Si hay 2 lógicas: dividir en 5 partes
        if(logica1 && logica2) {
            int anchoLogica = (anchoExtendida - 4) / 5;

            // Espacio libre inicial
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre");
            xLogica += anchoLogica;

            // Lógica 1
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, QString(logica1->nombre));
            xLogica += anchoLogica;

            // Libre centro
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre");
            xLogica += anchoLogica;

            // Lógica 2
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, QString(logica2->nombre));
            xLogica += anchoLogica;

            // Libre final
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre");
        }
        // Si hay 1 lógica: dividir en 3 partes
        else if(logica1) {
            int anchoLogica = (anchoExtendida - 4) / 3;

            // Libre inicial
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre");
            xLogica += anchoLogica;

            // Lógica 1
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, QString(logica1->nombre));
            xLogica += anchoLogica;

            // Libre final
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre");
        }
        // Si no hay lógicas: solo un bloque libre
        else {
            int anchoLogica = anchoExtendida - 4;
            painter.drawRect(xLogica, yLogica, anchoLogica, alturaLogica);
            painter.drawText(QRect(xLogica, yLogica, anchoLogica, alturaLogica),
                             Qt::AlignCenter, "Libre\n100%");
        }
    } else {
        // Si no hay extendida, mostrar como libre
        painter.drawRect(x, y, anchoExtendida, altura);
        painter.drawText(QRect(x, y, anchoExtendida, altura), Qt::AlignCenter, "Libre");
    }
    x += anchoExtendida;

    // ===== CUADRO 4: PRIMARIA 2 o LIBRE (1 unidad) =====
    int anchoP2 = anchoSeccion;
    painter.drawRect(x, y, anchoP2, altura);

    if(primaria2) {
        QString textoP2 = QString(primaria2->nombre) + "\n\n" +
                          QString::number(porcentajeP2, 'f', 1) + "%";
        painter.drawText(QRect(x, y, anchoP2, altura), Qt::AlignCenter, textoP2);
    } else {
        painter.drawText(QRect(x, y, anchoP2, altura), Qt::AlignCenter, "Libre");
    }
    x += anchoP2;

    // ===== CUADRO 5: LIBRE FINAL (1 unidad) =====
    int anchoLibreFin = anchoSeccion;
    painter.drawRect(x, y, anchoLibreFin, altura);

    if(porcentajeLibreFinal > 0.1) {
        QString textoLibreFin = "Libre\n\n" + QString::number(porcentajeLibreFinal, 'f', 1) + "%";
        painter.drawText(QRect(x, y, anchoLibreFin, altura), Qt::AlignCenter, textoLibreFin);
    } else {
        painter.drawText(QRect(x, y, anchoLibreFin, altura), Qt::AlignCenter, "Libre");
    }

    painter.end();
    return pixmap;
}
