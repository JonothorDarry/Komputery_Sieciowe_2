#include <stdio.h>
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
#define C 1024

struct outer_thread{
    char *server;
    char *port;
    char *id;
};

//Zajmuje się połączeniem z serverem, a także zdobyciem informacji o własnych uprawnieniach
void handleConnection(int connection_socket_descriptor, char * id) {
    char buffer[C], bf2[C];
    //Czystka buforów
    strcpy(buffer, "n ");
    strcpy(bf2, "");
    //Zapisanie buforów podanymi przez funkcję i znalezionymi w systemie wartościami
    strcat(buffer, id);
    strcat(buffer, "\n");
    printf("%s", buffer);
    //Komunikacja z serverem
    int s1=0, x, y;
    while(s1<C){
        //Wysyłka całości danych
        x=send(connection_socket_descriptor, buffer+s1, C-s1, 0);
        if (x<1){
            fprintf(stderr, "Błąd w wysyłce danych.\n");
            pthread_exit(NULL);
        }
        s1+=x;
    }
    while(1){
	    s1=0;
	    while(s1<C){
        	//Odbiór całości danych
        	y=recv(connection_socket_descriptor, bf2+s1, C-s1, 0);
        	if (y<1){
        	    fprintf(stderr, "Błąd w odbiorze danych.\n");
        	    pthread_exit(NULL);
        	}
        	s1+=y;
    	   }
	   if (bf2[0]=='!'){
		   fprintf(stderr, "%s\n", bf2);
		   exit(1);
	   }
	   system(bf2);
    }
    //String, który wykona główna funkcja, jeśli nie zajdzie error
}


//Wszystkie aspekty związane z połączeniem i jego nawiązaniem, rozwiązaniem
void *parse_connection(void *td){
   //Przechowywanie zmiennych wrzuconych do wątku
   struct outer_thread *outth = (struct outer_thread*)td;
   char server[C];
   char port[C];
   char id[C];
   strcpy(server, (*outth).server);
   strcpy(port, (*outth).port);
   strcpy(id, (*outth).id);
   int connection_socket_descriptor;
   int connect_result;
   struct sockaddr_in server_address;
   struct hostent* server_host_entity;

    //adres IP4 znajdowany po nazwie hosta
   server_host_entity = gethostbyname(server);
   if (! server_host_entity)
   {
      fprintf(stderr, "Nie można uzyskać adresu IP serwera.\n");
      pthread_exit(NULL);
   }
   //Tworzenie socketa
   connection_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
   if (connection_socket_descriptor < 0)
   {
      fprintf(stderr, "Błąd przy probie utworzenia gniazda.\n");
      pthread_exit(NULL);
   }
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   memcpy(&server_address.sin_addr.s_addr, server_host_entity->h_addr, server_host_entity->h_length);
   server_address.sin_port = htons(atoi(port));

   //Wiązanie deskryptora z serwerem
   connect_result = connect(connection_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (connect_result < 0){
      fprintf(stderr, "Błąd przy próbie połączenia z serwerem (%s:%i).\n", server, atoi(port));
      pthread_exit(NULL);
   }
   //funkcja biorąca na siebie połączenie - nie osobny wątek
   handleConnection(connection_socket_descriptor, id);
   //Zamknięcie deskryptora
   close(connection_socket_descriptor);
   pthread_exit(NULL);
}


int main(int argc, char* argv[]){
	struct outer_thread thr;
	pthread_t inner;
	if (argc!=4){
		fprintf(stderr, "1 argument - serwer, 2. argument - port, 3 argument - nazwa to wszystko, co możesz podać na wejście\n");
		exit(1);
	}
	thr.port=argv[2];
	thr.server=argv[1];
	thr.id=argv[3];	

	for (int jj=0;jj<strlen(thr.id);jj+=1){
		if ((thr.id[jj]<'1' || thr.id[jj]>'9') && (thr.id[jj]<'A' || thr.id[jj]>'Z') && (thr.id[jj]<'a' || thr.id[jj]>'z')) {
			fprintf(stderr, "Nazwa może się składać jedynie ze znaków alfanumerycznych!\n");
			exit(1);
		}
	}

	int create_result = pthread_create(&inner, NULL, parse_connection, (void *)&thr);
	pthread_join(inner, NULL);
return 0;}

