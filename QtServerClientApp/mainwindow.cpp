#include "mainwindow.h"
#include "wundabarnetwerking.h"
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QPushButton>
#include <QLineEdit>
#include <QLine>
#include <QLabel>
#include <QCoreApplication>
#include <QSignalMapper>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    setWindowTitle("Shutdown Application");
    setMinimumSize(800, 450);
    /*
    QMenuBar * menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    QMenu * menu = menuBar->addMenu("&File");
    */

    int ydis=260;

    //QLine * txt = new QLine(this);
    QLabel * txt = new QLabel(this);
    txt -> setGeometry(QRect(275, ydis-200, 300, 100));
    txt -> setText("What action shall be performed, master?");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(100, ydis-100, 100, 100));
    txt -> setText("Server Address:");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(350, ydis-100, 100, 100));
    txt -> setText("Server Port:");

    tline=new QLineEdit(this);
    tline -> setGeometry(QRect(200, ydis-100+40, 100, 20));
    tline->insert("200.23.1.2");

    tline2=new QLineEdit(this);
    tline2 -> setGeometry(QRect(450, ydis-100+40, 100, 20));
    tline2->insert("1234");

    shut= new QPushButton(this);
    shut -> setGeometry(QRect(QPoint(100, ydis), QSize(200, 50)));
    shut -> setText("Shutdown");
    connect(shut, SIGNAL (released()), this, SLOT (handleShut()));

    res= new QPushButton(this);
    res -> setGeometry(QRect(QPoint(300, ydis), QSize(200, 50)));
    res -> setText("Reset");
    connect(res, SIGNAL (released()), this, SLOT(handleRes()));

    //QTimer* timer = new QTimer();
    //timer->setInterval(1000);

    //timer->start();

    QPushButton * can= new QPushButton(this);
    can -> setGeometry(QRect(QPoint(500, ydis), QSize(200, 50)));
    can -> setText("Cancel");
    connect(can, SIGNAL (released()), this, SLOT (handleCan()));
    QStatusBar * statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
}

MainWindow::~MainWindow()
{

}

// == PRIVATE SLOTS ==
void MainWindow::handleShut(){
    shut->setDisabled(true);
    res->setDisabled(true);
    statusBar()->showMessage("Shutdown...");
}
void MainWindow::handleRes(){
    shut->setDisabled(true);
    res->setDisabled(true);
    statusBar()->showMessage("Reset...");
    QString vx=tline->text(), vy=tline2->text();
    QByteArray ba = vx.toLocal8Bit(), bb=vy.toLocal8Bit();
    char *serv = ba.data();
    char *port=bb.data();

    outer_processing(serv,  port);
    statusBar()->showMessage(stat);
    shut->setDisabled(false);
    res->setDisabled(false);
}

void MainWindow::handleCan(){
    statusBar()->showMessage("Cancel");
    QCoreApplication::quit();
    exit(0);
}
