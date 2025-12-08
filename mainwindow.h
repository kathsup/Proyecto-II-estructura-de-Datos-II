#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "diskmanager.h"
#include <QTextEdit>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool eventFilter(QObject *obj, QEvent *event) override;


private:
    Ui::MainWindow *ui;
    QString currentPath;
    DiskManager *diskManager;

    void processCommand(const QString &cmd);
    bool applyCd(QString arg);

    //metodo para parsear: sirve para dividir el comando que ponga
    map<QString, QString> extraerParametros(QString Linea);
    QString pendingDiskToDelete;
    void procesarParametro(QString parametro, map<QString, QString> &resultado);


    map<QString, QString> pendingPartitionDelete;
};

#endif // MAINWINDOW_H
