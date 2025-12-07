/*#ifndef DIAGRAMAWIDGET_H
#define DIAGRAMAWIDGET_H

#include <QWidget>
#include <QPixmap>
#include "structuras.h"

class DiagramaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiagramaWidget(MBR mbr, QWidget *parent = nullptr);

private:
    QPixmap crearImagenDiagrama(MBR mbr);
};

#endif // DIAGRAMAWIDGET_H
*/

#ifndef DIAGRAMAWIDGET_H
#define DIAGRAMAWIDGET_H

#include <QWidget>
#include "structuras.h"

class DiagramaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiagramaWidget(MBR mbr, QWidget *parent = nullptr);

private:
    QPixmap crearImagenDiagrama(MBR mbr);
};

#endif // DIAGRAMAWIDGET_H
