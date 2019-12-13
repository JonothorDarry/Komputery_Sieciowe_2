#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
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
        QPushButton * shut;
        QPushButton * res;

    private slots:
      void handleShut();
      void handleRes();
      void handleCan();
};


#endif // MAINWINDOW_H
