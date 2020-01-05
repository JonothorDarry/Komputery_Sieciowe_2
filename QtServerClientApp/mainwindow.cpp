#include <QListWidget>
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
#include <QCheckBox>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    //Okno samo-w-sobie, stałe
    setWindowTitle("Shutdown Application");
    setMinimumSize(800, 450);
    int ydis=360;

    //Umiejscawianie tekstów stałych
    QLabel * txt = new QLabel(this);
    txt -> setGeometry(QRect(275, ydis-400, 300, 100));
    txt -> setText("What action shall be performed, master?");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(100, ydis-140, 100, 100));
    txt -> setText("Server Address:");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(350, ydis-140, 100, 100));
    txt -> setText("Server Port:");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(100, ydis-10, 150, 40));
    txt -> setText("Wait time (in minutes):");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(500, ydis-300+40, 200, 20));
    txt -> setText("Identificators to add:");

    txt = new QLabel(this);
    txt -> setGeometry(QRect(50, ydis-380+40, 200, 20));
    txt -> setText("List of known identificators:");

    //Lista identyfikatorów
    vis=new QListWidget(this);
    vis->setSelectionMode(QAbstractItemView::MultiSelection);
    vis -> setGeometry(QRect(50, ydis-320, 200, 200));

    //Linie do wpisania inputu - struktura kodu wymaga, aby były dostępne globalnie - także dla triggerów
    tline=new QLineEdit(this);
    tline -> setGeometry(QRect(200, ydis-140+40, 100, 20));
    tline->insert("localhost");

    sear=new QLineEdit(this);
    sear -> setGeometry(QRect(500, ydis-270+40, 200, 20));

    tline2=new QLineEdit(this);
    tline2 -> setGeometry(QRect(450, ydis-140+40, 100, 20));
    tline2->insert("1234");

    timeline=new QLineEdit(this);
    timeline -> setGeometry(QRect(300, ydis-10+10, 100, 20));
    timeline->insert("1");

    //Przyciski wyłączające aplikację
    shut= new QPushButton(this);
    shut -> setGeometry(QRect(QPoint(100, ydis-60), QSize(200, 50)));
    shut -> setText("Shutdown");
    connect(shut, SIGNAL (released()), this, SLOT (handleShut()));

    res= new QPushButton(this);
    res -> setGeometry(QRect(QPoint(300, ydis-60), QSize(200, 50)));
    res -> setText("Reset");
    connect(res, SIGNAL (released()), this, SLOT(handleRes()));

    searer=new QPushButton(this);
    searer -> setGeometry(QRect(QPoint(500, ydis-200), QSize(200, 50)));
    searer -> setText("Add Identificators");
    connect(searer, SIGNAL (released()), this, SLOT(handleSear()));


    QPushButton * can= new QPushButton(this);
    can -> setGeometry(QRect(QPoint(500, ydis-60), QSize(200, 50)));
    can -> setText("Cancel");
    connect(can, SIGNAL (released()), this, SLOT (handleCan()));

    rem= new QPushButton(this);
    rem -> setGeometry(QRect(QPoint(250, ydis-240), QSize(200, 50)));
    rem -> setText("Remove Identificators");
    connect(rem, SIGNAL (released()), this, SLOT (handleRem()));

    //Timer - jego celem jest bezpieczne odblokowywanie aplikacji po zakończeniu resetu
    QTimer* timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, [=](){
        //Jeśli wątek interakcji z serverem się skończył, a był, to:
        if (is_process==1 && thread_complete==1){
            //tworzę komunikat statusu, robię jego joina
            if (stat[0]=='!' && gbf[0]=='2') strcat(stat, " : Uzyskane identyfikatory: "), strcat(stat, gbf+2);
            if (stat[0]=='!' && gbf[0]=='3') strcat(stat, " : Komputery, na których na pewno nie wykonano polecenia: "), strcat(stat, gbf+2);
            statusBar()->showMessage(stat);
            pthread_join(inner, NULL);

            thread_complete=0;
            is_process=0;
            //Dodanie identyfikatora - dodanie do listy, jeśli jeszcze go nie ma
            if (gbf[0]=='2'){
                int j, lst=0, jj;
                char bf2[C], tmpb[C];
                strcpy(bf2, gbf+2);
                for (j=0;j<C-2;j++){
                        if (bf2[j]=='\0')       break;
                        if ((bf2[j]<'0' || bf2[j]>'9') && (bf2[j]<'A' || bf2[j]>'Z') && (bf2[j]<'a' || bf2[j]>'z')){
                                for (jj=lst;jj<j;jj+=1) tmpb[jj-lst]=bf2[jj];
                                tmpb[jj-lst]='\0';
                                QListWidgetItem *lambda=new QListWidgetItem();
                                lambda->setText(tmpb);
                                QString x=QString::fromUtf8(tmpb);
                                QList<QListWidgetItem *> ff=vis->findItems(x, Qt::MatchExactly);
                                if (ff.length()==0) vis->addItem(lambda);
                                lst=jj+1;
                        }
                }
            }

            //Usuwanie identyfikatora z listy, jeśli serwer na pewno stracił z nim połączenie
            if (gbf[0]=='3'){
                int j, lst=0, jj;
                char bf2[C], tmpb[C];
                strcpy(bf2, gbf+2);
                strcat(bf2, "\n");
                for (j=0;j<C-2;j++){
                        if (bf2[j]=='\0')       break;
                        if ((bf2[j]<'0' || bf2[j]>'9') && (bf2[j]<'A' || bf2[j]>'Z') && (bf2[j]<'a' || bf2[j]>'z')){
                                for (jj=lst;jj<j;jj+=1) tmpb[jj-lst]=bf2[jj];
                                tmpb[jj-lst]='\0';
                                QString x=QString::fromUtf8(tmpb);
                                QList<QListWidgetItem *> ff=vis->findItems(x, Qt::MatchExactly);
                                for (int jj=0;jj<ff.length();jj++){
                                    int y=vis->row(ff[jj]);
                                    vis->takeItem(y);
                                }
                                lst=jj+1;
                        }
                }
            }

            //Nowy klient już może wchodzić
            pthread_mutex_unlock(&newcli);
            shut->setDisabled(false);
            res->setDisabled(false);
            searer->setDisabled(false);
            rem->setDisabled(false);
        }
    });
    timer->start();

    //dodanie Statusu
    QStatusBar * statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
}

MainWindow::~MainWindow()
{

}

// == PRIVATE SLOTS ==
//Trigger przycisku shutdown
void MainWindow::handleShut(){
    //Mrożenie przycisków
    shut->setDisabled(true);
    res->setDisabled(true);
    searer->setDisabled(true);
    rem->setDisabled(true);
    statusBar()->showMessage("Shutdown...");

    //Wyciąganie informacji z pól użytkownika
    QString vx=tline->text(), vy=tline2->text(), zeit=timeline->text();
    QByteArray ba = vx.toLocal8Bit(), bb=vy.toLocal8Bit(), bc=zeit.toLocal8Bit();
    char *serv = ba.data();
    char *port=bb.data();
    char times[1024];
    strcpy(times, bc.data());
    strcat(times, " ");
    //Które pola na liście są zaznaczone? - nazwy idą do serwera
    QList<QListWidgetItem *> f=vis->selectedItems();
    for (int jj=0;jj<f.length();jj++){
        strcat(times, (f[jj]->text()).toLocal8Bit().data());
        strcat(times, " ");
    }
    //Przetwarzanie danych podanych przez usera
    outer_processing(serv, port, times, 0);
}
//Trigger przycisku reset
void MainWindow::handleRes(){
    //Unieruchamianie przycisków, Zmiana status bara
    shut->setDisabled(true);
    res->setDisabled(true);
    searer->setDisabled(true);
    rem->setDisabled(true);
    statusBar()->showMessage("Reset...");

    //Wyciąganie informacji z pól użytkownika
    QString vx=tline->text(), vy=tline2->text(), zeit=timeline->text();
    QByteArray ba = vx.toLocal8Bit(), bb=vy.toLocal8Bit(), bc=zeit.toLocal8Bit();
    char *serv = ba.data();
    char *port=bb.data();
    char times[1024];
    strcpy(times, bc.data());
    strcat(times, " ");
    //Które pola na liście są zaznaczone? - nazwy idą do serwera
    QList<QListWidgetItem *> f=vis->selectedItems();
    for (int jj=0;jj<f.length();jj++){
        strcat(times, (f[jj]->text()).toLocal8Bit().data());
        strcat(times, " ");
    }
    //Przetwarzanie danych podanych przez usera
    outer_processing(serv, port, times, 1);
}

void MainWindow::handleSear(){
    //Mrożenie przycisków
    shut->setDisabled(true);
    res->setDisabled(true);
    searer->setDisabled(true);
    rem->setDisabled(true);
    statusBar()->showMessage("Searching for Wisdom...");
    //Wzięcie serwera, portu, identyfikatorów i pass do outer_processing
    QString vx=tline->text(), vy=tline2->text(), vz=sear->text();
    QByteArray ba = vx.toLocal8Bit(), bb=vy.toLocal8Bit(), bc=vz.toLocal8Bit();
    char *serv = ba.data();
    char *port=bb.data();
    char *names=bc.data();
    outer_processing(serv, port, names, 2);
}

void MainWindow::handleRem(){
    QList<QListWidgetItem *> f=vis->selectedItems();
    for (int jj=0;jj<f.length();jj++){
        int y=vis->row(f[jj]);
        vis->takeItem(y);
    }

}

//Trigger przycisku cancel
void MainWindow::handleCan(){
    statusBar()->showMessage("Cancel");
    QCoreApplication::quit();
    exit(0);
}
