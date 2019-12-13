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

#define BUF_SIZE 1024
#define NUM_THREADS 5
#define C 2048
int is_process=0, dead=0, thread_complete=0;
pthread_t inner;

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    int csd;
    char *times;
};

struct outer_thread{
    char *server;
    char *port;
    char *times;
};
struct outer_thread thr;

//Komenda do wykonania i wypis na statusBar
char gbf[C], stat[C];
//Funkcja uzyskująca informację o uprawnieniach użytkownika i dostępie do funkcji init, shutdown; zwraca w tablicy a:
//Efektywne id usera, grupy, a także całe 2 wiersze(shutdown, init) zwracany przez długiego ls-a podążającego za symlinkiem z numerycznymi uid i gid
void attain_wisdom(char a[]){
    char bf[C], rn[C], df[C], dk[C];
    FILE *fp, *fp3;

    fp=popen("sh -c 'which shutdown; which init' | xargs -I{} ls -lnH {}", "r");
    if (fp==NULL){
        fprintf(stderr, "Failed running command");
        sprintf(stat, "Nie powiodło się wykonanie podstawowych poleceń: which i ls");
        pthread_exit(NULL);
    }

    fp3=popen("id -u; id -g", "r");
    fgets(dk, sizeof(dk), fp3);
    strcat(a, dk);
    fgets(dk, sizeof(dk), fp3);
    strcat(a,dk);

    while (fgets(bf, sizeof(bf), fp)!=NULL){
        strcat(a, bf);
    }
    pclose(fp);
    pclose(fp3);
}

//wskaźnik na funkcję opisującą zachowanie wątku
//Co robi wątek? zdobywa wiedzę, wysyła ją serwerowi, odbiera od serwera dane i wydaje zadaną komendę do tablicy globalnej, żeby główny wątek ją przetworzył
void *ThreadBehavior(void *t_data){
    char buffer[C], bf2[C];
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    //dostęp do pól struktury: (*th_data).pole
    strcpy(buffer, "");
    attain_wisdom(buffer);
    strcat(buffer, (*th_data).times);
    strcat(buffer, "\n");
    printf("%s", buffer);
    send((*th_data).csd, buffer, C, 0);
    recv((*th_data).csd, bf2, C, 0);
    strcpy(gbf, bf2);
    pthread_exit(NULL);
}


//funkcja obsługująca połączenie z serwerem
//Po prostu wysyła dane do wątku i czeka na jego zakończenie.
void handleConnection(int connection_socket_descriptor, char * times) {
    //wynik funkcji tworzącej wątek
    int create_result = 0;

    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
    struct thread_data_t t_data;
    t_data.csd=connection_socket_descriptor;
    t_data.times=times;

    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)&t_data);
    if (create_result){
       fprintf(stderr, "Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       sprintf(stat, "Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       pthread_exit(NULL);
    }

    pthread_join(thread1, NULL);
    //TODO (przy zadaniu 1) odbieranie -> wyświetlanie albo klawiatura -> wysyłanie
}

//Wszystkie aspekty związane z połączeniem i jego nawiązaniem, rozwiązaniem
void *parse_connection(void *td){
   struct outer_thread *outth = (struct outer_thread*)td;
   char *server=(*outth).server;
   char *port=(*outth).port;
   char *times=(*outth).times;

   int connection_socket_descriptor;
   int connect_result;
   struct sockaddr_in server_address;
   struct hostent* server_host_entity;


   server_host_entity = gethostbyname(server);
   if (! server_host_entity)
   {
      fprintf(stderr, "Nie można uzyskać adresu IP serwera.\n");
      sprintf(stat, "Nie można uzyskać adresu IP serwera.\n");
      thread_complete=1;
      pthread_exit(NULL);
   }
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

   connect_result = connect(connection_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (connect_result < 0)
   {
      fprintf(stderr, "Błąd przy próbie połączenia z serwerem (%s:%i).\n", server, atoi(port));
      sprintf(stat,"Błąd przy próbie połączenia z serwerem (%s:%i).\n", server, atoi(port));
      thread_complete=1;
      pthread_exit(NULL);
   }

   handleConnection(connection_socket_descriptor, times);
   close(connection_socket_descriptor);
   sprintf(stat,"Operacja się powiodła\n", server, atoi(port));
   thread_complete=1;
   pthread_exit(NULL);
}

void outer_processing(char server[], char port[], char times[]){
    is_process=1;
    //dane, które zostaną przekazane do wątku
    thr.port=port;
    thr.server=server;
    thr.times=times;
    int create_result = pthread_create(&inner, NULL, parse_connection, (void *)&thr);
    if (create_result){
       sprintf(stat,"Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
    }

    //pthread_join(inner, NULL);
}

#endif // WUNDABARNETWERKING_H
