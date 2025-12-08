#ifndef DIAGRAMAWIDGET_H
#define DIAGRAMAWIDGET_H

#include <QWidget>
#include "structuras.h"

class DiagramaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiagramaWidget(MBR mbr, QWidget *parent = nullptr);
    QPixmap crearImagenDiagrama(MBR mbr);
    void dibujarLogicasEnExtendida(QPainter* painter, int x, int y,
                                   int anchoExtendida, int altura,
                                   Particion* logicas[], int numLogicas,
                                   Particion* extendida);
};

#endif // DIAGRAMAWIDGET_H
