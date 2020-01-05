#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <iostream>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = 0);
        ~MainWindow();
    private:
        QLineEdit * tline;
        QLineEdit * tline2;
        QLineEdit * timeline;
        QLineEdit * sear;
        QPushButton * shut;
        QPushButton * res;
        QPushButton * searer;
        QListWidget * vis;
        QPushButton * rem;

    private slots:
      void handleShut();
      void handleRes();
      void handleCan();
      void handleSear();
      void handleRem();
};


#endif // MAINWINDOW_H
