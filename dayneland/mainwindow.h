#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private slots:
        /*
      void OnFileNew();
      void OnFileOpen();
      void OnFileSave();
      void OnDayneland();*/
      void handleShut();
      void handleRes();
      void handleCan();
};


#endif // MAINWINDOW_H
