#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFont>
#include <QKeyEvent>
#include <QStringList>
#include <QTextCursor>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Apariencia CMD
    QFont font("Consolas", 11);
    font.setFixedPitch(true);
    ui->base->setFont(font);
    ui->base->setStyleSheet("background-color: black; color: white; border: none;");

    currentPath = "C:/";

    QString header = "-------------------------------------------------- Sistema de archivos -------------------------------------------------\n";
    header += "                                              Katherine Carvallo - 22441130\n";
    header += "------------------------------------------------------------------------------------------------------------------------\n\n";
    header += currentPath + "> ";

    ui->base->setPlainText(header);

    ui->base->installEventFilter(this);


    QTextCursor cursor = ui->base->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->base->setTextCursor(cursor);
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::processCommand(const QString &cmd)
{
    QString trimmedCmd = cmd.trimmed();

    if (trimmedCmd.isEmpty())
    {
        ui->base->appendPlainText(currentPath + "> ");
        return;
    }

    QStringList parts = trimmedCmd.split(" ", Qt::SkipEmptyParts);
    QString command = parts[0].toLower();

    if (command == "cd")
    {
        if (parts.size() > 1)
        {
            applyCd(parts[1]);
        }
        else
        {
            ui->base->appendPlainText("Se requiere un parámetro de ruta.");
        }

        ui->base->appendPlainText(currentPath + "> ");
        return;
    }

    if (command == "clear")
    {
        ui->base->clear();
        QString header = "-------------------------------------------------- Sistema de archivos -------------------------------------------------\n";
        header += "                                              Katherine Carvallo - 22441130\n";
        header += "------------------------------------------------------------------------------------------------------------------------\n\n";
        header += currentPath + "> ";
        ui->base->setPlainText(header);
        return;
    }

    // Comando no reconocido
    ui->base->appendPlainText("'" + command + "' no es reconocido como un comando interno o externo.");
    ui->base->appendPlainText(currentPath + "> ");
}

bool MainWindow::applyCd(QString arg)
{
    arg = arg.trimmed();
    arg.replace("\\", "/");   // normalizar barras

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

        if (newPath == "C:/")
            return true;


        newPath.chop(1);
        int idx = newPath.lastIndexOf('/');

        if (idx != -1)
        {
            newPath = newPath.left(idx + 1);
        }
        else
        {
            newPath = "C:/";
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
                if (newPath != "C:/")
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

