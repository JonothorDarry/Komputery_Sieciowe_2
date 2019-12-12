#include "mainwindow.h"
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QPushButton>
#include <QLine>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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
    txt -> setGeometry(QRect(275, ydis-100, 300, 100));
    txt -> setText("What action shall be performed, master?");

    QPushButton * shut= new QPushButton(this);
    shut -> setGeometry(QRect(QPoint(100, ydis), QSize(200, 50)));
    shut -> setText("Shutdown");
    connect(shut, SIGNAL (released()), this, SLOT (handleShut()));

    QPushButton * res= new QPushButton(this);
    res -> setGeometry(QRect(QPoint(300, ydis), QSize(200, 50)));
    res -> setText("Reset");
    connect(res, SIGNAL (released()), this, SLOT (handleRes()));

    QPushButton * can= new QPushButton(this);
    can -> setGeometry(QRect(QPoint(500, ydis), QSize(200, 50)));
    can -> setText("Cancel");
    connect(can, SIGNAL (released()), this, SLOT (handleCan()));
/*
    // New
    QAction * action = new QAction("&New", this);
    connect(action, &QAction::triggered, this, &MainWindow::OnFileNew);
    menu->addAction(action);
    // Open
    action = new QAction("&Logarithm", this);
    connect(action, &QAction::triggered, this, &MainWindow::OnFileOpen);
    menu->addAction(action);
    // Save
    action = new QAction("&Save", this);
    connect(action, &QAction::triggered, this, &MainWindow::OnFileSave);
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction("&Exit", this);
    connect(action, &QAction::triggered, this, &MainWindow::close);
    menu->addAction(action);
*/
    QStatusBar * statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
}

MainWindow::~MainWindow()
{

}

// == PRIVATE SLOTS ==
/*
void MainWindow::OnDayneland(){
    statusBar()->showMessage("HAHAHANAHA");
}

void MainWindow::OnFileNew()
{
    statusBar()->showMessage("File -> New");
}

void MainWindow::OnFileOpen()
{
    statusBar()->showMessage("File -> Open");
}

void MainWindow::OnFileSave()
{
    statusBar()->showMessage("File -> Penissus Maximus");
}
*/
void MainWindow::handleShut(){
    statusBar()->showMessage("Shutdown");
}
void MainWindow::handleRes(){
    statusBar()->showMessage("Reset");
}
void MainWindow::handleCan(){
    statusBar()->showMessage("Cancel");
}
