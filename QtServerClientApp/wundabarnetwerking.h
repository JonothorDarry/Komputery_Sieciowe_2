#ifndef WUNDABARNETWERKING_H
#define WUNDABARNETWERKING_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <chrono>
#include <thread>

//Rozmiar bufora, Czy zaczął się proces, czy wątek skończył się
#define C 1024
int is_process=0, thread_complete=0;
//Wątek - komunikacja z serwerem, mutexy na nowego klienta i zapis danych do wątku
pthread_t inner;
pthread_mutex_t newcli = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t data_written = PTHREAD_MUTEX_INITIALIZER;

//struktura zawierająca dane, które zostaną przekazane do wątku
struct outer_thread{
    char *server;
    char *port;
    char *times;
    int op;
};
struct outer_thread thr;

//Komenda do wykonania i wypis na statusBar
char gbf[C], stat[C];
//Funkcja uzyskująca informację o uprawnieniach użytkownika i dostępie do funkcji init, shutdown; zwraca w tablicy a:
//Efektywne id usera, grupy, a także całe 2 wiersze(shutdown, init) zwracany przez długiego ls-a podążającego za symlinkiem z numerycznymi uid i gid
void attain_wisdom(char a[]){
    char bf[C], dk[C];
    FILE *fp, *fp3;
    //Znalezienie permisji
    fp=popen("sh -c 'which shutdown; which init' | xargs -I{} ls -lnH {}", "r");
    if (fp==NULL){
        fprintf(stderr, "Nie powiodło się wykonanie podstawowych poleceń: which i ls");
        sprintf(stat, "Nie powiodło się wykonanie podstawowych poleceń: which i ls");
        thread_complete=1;
        pthread_exit(NULL);
    }
    //id usera i grupy, w której jest user - dodane do tablicy na output
    fp3=popen("id -u; id -g", "r");
    fgets(dk, sizeof(dk), fp3);
    strcat(a, dk);
    fgets(dk, sizeof(dk), fp3);
    strcat(a,dk);

    //Dodanie do stringa na wyjście permisji
    while (fgets(bf, sizeof(bf), fp)!=NULL){
        strcat(a, bf);
    }
    //Zamknięcie plików
    pclose(fp); pclose(fp3);
}


//Zajmuje się połączeniem z serverem, a także zdobyciem informacji o własnych uprawnieniach
void handleConnection(int connection_socket_descriptor, char * times, int wal) {
    char buffer[C], bf2[C];
    //Czystka buforów
    strcpy(buffer, "");
    strcpy(bf2, "");
    //Zapisanie buforów podanymi przez funkcję i znalezionymi w systemie wartościami
    sprintf(buffer, "%d\n", wal);
    strcat(buffer, times);
    strcat(buffer, "\n");
    printf("%s", buffer);
    //Komunikacja z serverem
    int s1=0, x, y;
    while(s1<C){
        //Wysyłka całości danych
        x=send(connection_socket_descriptor, buffer+s1, C-s1, 0);
        if (x<1){
            fprintf(stderr, "Błąd w wysyłce danych.\n");
            sprintf(stat, "Błąd w wysyłce danych.\n");
            thread_complete=1;
            pthread_exit(NULL);
        }
        s1+=x;
    }
    s1=0;
    while(s1<C){
        //Odbiór całości danych
        y=recv(connection_socket_descriptor, bf2+s1, C-s1, 0);
        if (y<1){
            fprintf(stderr, "Błąd w odbiorze danych.\n");
            sprintf(stat, "Błąd w odbiorze danych.\n");
            thread_complete=1;
            pthread_exit(NULL);
        }
        s1+=y;
    }
    //String, który wykona główna funkcja, jeśli nie zajdzie error
    strcpy(gbf, bf2);
}

//Wszystkie aspekty związane z połączeniem i jego nawiązaniem, rozwiązaniem
void *parse_connection(void *td){
   //Przechowywanie zmiennych wrzuconych do wątku
   struct outer_thread *outth = (struct outer_thread*)td;
   char server[C];
   char port[C];
   char times[C];
   int v=(*outth).op;
   strcpy(server, (*outth).server);
   strcpy(port, (*outth).port);
   strcpy(times, (*outth).times);
   pthread_mutex_unlock(&data_written);
   int connection_socket_descriptor;
   int connect_result;
   struct sockaddr_in server_address;
   struct hostent* server_host_entity;

    //adres IP4 znajdowany po nazwie hosta
   server_host_entity = gethostbyname(server);
   if (! server_host_entity)
   {
      fprintf(stderr, "Nie można uzyskać adresu IP serwera.\n");
      sprintf(stat, "Nie można uzyskać adresu IP serwera.\n");
      thread_complete=1;
      pthread_exit(NULL);
   }
   //Tworzenie socketa
   connection_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
   if (connection_socket_descriptor < 0)
   {
      fprintf(stderr, "Błąd przy probie utworzenia gniazda.\n");
      sprintf(stat, "Błąd przy probie utworzenia gniazda.\n");
      thread_complete=1;
      pthread_exit(NULL);
   }
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   memcpy(&server_address.sin_addr.s_addr, server_host_entity->h_addr, server_host_entity->h_length);
   server_address.sin_port = htons(atoi(port));

   //Wiązanie deskryptora z serwerem
   connect_result = connect(connection_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (connect_result < 0)
   {
      fprintf(stderr, "Błąd przy próbie połączenia z serwerem (%s:%i).\n", server, atoi(port));
      sprintf(stat,"Błąd przy próbie połączenia z serwerem (%s:%i).\n", server, atoi(port));
      thread_complete=1;
      pthread_exit(NULL);
   }

   //funkcja biorąca na siebie połączenie - nie osobny wątek
   handleConnection(connection_socket_descriptor, times, v);
   //Zamknięcie deskryptora
   close(connection_socket_descriptor);
   sprintf(stat,"!Serwer wysłał odpowiedź!\n");
   thread_complete=1;
   pthread_exit(NULL);
}

//Co się w ogóle stanie - najbardziej zewnętrzna funkcja robiąca passa do wątku
void outer_processing(char server[], char port[], char times[], int w){
    pthread_mutex_lock(&newcli);
    pthread_mutex_unlock(&data_written);
    is_process=1;
    //dane, które zostaną przekazane do wątku
    thr.port=port;
    thr.server=server;
    thr.times=times;
    thr.op=w;
    pthread_mutex_lock(&data_written);
    int create_result = pthread_create(&inner, NULL, parse_connection, (void *)&thr);
    if (create_result){
       sprintf(stat,"Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
    }
    pthread_mutex_lock(&data_written);
    //pthread_join(inner, NULL);
}

#endif // WUNDABARNETWERKING_H
