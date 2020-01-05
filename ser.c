#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

//stałe - port, kolejka, rozmiar tablic
#define SERVER_PORT 1234
#define QUEUE_SIZE 20 
#define C 1024
//struktura zawierająca dane, które zostaną przekazane do wątku

//dead - czy indeks i-ty przechowuje identyfikator?, valid[i] - i-ty identyfikator, prep_command - komenda dla i-tego identyfikatora
int dead[QUEUE_SIZE];
char valid[QUEUE_SIZE][1024], prep_command[QUEUE_SIZE][1024];
//mx - mutex na tablicę identyfikatorów, arr - mutex na kolejne identyfikatory, nv - zmienna warunkowa, na której czekają identyfikatory
pthread_mutex_t mx=PTHREAD_MUTEX_INITIALIZER, arr[QUEUE_SIZE] = {[0 ... QUEUE_SIZE-1] = PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t nv[QUEUE_SIZE]={[0 ... QUEUE_SIZE-1]=PTHREAD_COND_INITIALIZER};


struct thread_data_t{
	int csd; 
};

//jestem w pozycji p tablicy znaków a: znajduję inta skrytego od tej pozycji do końca inta(endline or space), zwracam go, zmieniając przy okacji pozycję na następnego niezbadanego chara w tablicy.
int int_from_pos(int *p, char a[]){	
	int i=*p, id=0;
	while (1){
		if (a[i]=='\n'||a[i]==' ') break;
		id=id*10+(a[i]-48);
		i+=1;
	}
	*p=i+1;

	return id;
}

//formulacja polecenia dla klienta
void grant_wisdom(char dest[], int res, int purp){
	char vv[10];
	//W dest polecenie dla klienta, vv - pomocnicza tablica to kopiowania inta - purp=0 oznacza shutdowna, purp=1 oznacza reset
	if (purp==0) {
		strcpy(dest, "shutdown -P ");
		sprintf(vv, "%d", res);
		strcat(dest, vv);
	}
	else if (purp==1) {
		strcpy(dest, "shutdown -r ");
		sprintf(vv, "%d", res);
		strcat(dest, vv);
	}
}

//Wysyłka całości danych - zwraca 1 w przypadku błędu, 0 przy poprawnym wykonaniu
int sendall(int csd, char* bf2){
	int s1=0, x;
	while(s1<C){
	  	x=send(csd, bf2+s1, C-s1, MSG_NOSIGNAL);
	    	if (x<1)  return -1;
    		s1+=x;
  	}
	return 0;
}

void finitorec(){
	fprintf(stderr, "Błąd w odbiorze danych.\n");
	pthread_exit(NULL);
}

void finitosen(){
	fprintf(stderr, "Błąd w wysyłce danych.\n");
	pthread_exit(NULL);
}

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data){
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    struct thread_data_t th2;
    th2.csd=(*th_data).csd;
    //Uwolnienie zaalokowanych uprzednio mallociem danych
    free(th_data);
    char buffer[C], bf2[C], tmpb[C];
    //dostęp do pól struktury: (*th_data).pole
    int x, y, s1=0;
    //Odbiór całości danych
    while(s1<C){
        y=recv(th2.csd, buffer+s1, C-s1, 0);
        if (y<1) finitorec();
        s1+=y;
    }
    s1=0;
    
    //komunikat został nadany przez node'a
    if (buffer[0]=='n'){
	    int i=0, jj=0;
	    //Kopiowanie danych od 2 znaku - sam identyfikator
	    for (jj=2;jj<strlen(buffer)-1;jj+=1) bf2[jj-2]=buffer[jj];
	    bf2[jj-2]='\0';
	    //Blokada na muteksie odpowiedzialnym za tablicę z identyfikatorami
	    pthread_mutex_lock(&mx);
	    //Sprawdzenie, czy istnieją 2 takie identyfikatory, których nazwy się powtarzają
	    while (i<QUEUE_SIZE){
		    if (dead[i]==1){
			    if (strcmp(valid[i], bf2)==0){
				    x=sendall(th2.csd, "!Nazwa się powtarza!");
				    if (x<0) finitosen();
				    pthread_mutex_unlock(&mx);
				    pthread_exit(NULL);
			    }
		    }
		    i+=1;
	    }

	    //Dodanie identyfikatora na wolnej pozycji
	    i=0;
	    while (i<QUEUE_SIZE){
		    if (dead[i]==0){
			    dead[i]=1;
			    break;
		    }
		    i+=1;
	    }

	    //Błąd - za dużo identyfikatorów zajętych
	    if (i==QUEUE_SIZE){
		    x=sendall(th2.csd, "!Kolejka identyfikatorów serwera jest pełna!");
		    if (x<0) finitosen();
		    fprintf(stderr, "Kolejka identyfikatorów jest pełna!");
		    pthread_exit(NULL);
	    }

	    //Przepis identyfikatora na wolne pole
	    for (jj=0;jj<strlen(bf2);jj+=1) valid[i][jj]=bf2[jj];
	    valid[i][jj]='\0';
	    //Czekanie na komunikaty
	    pthread_mutex_lock(&arr[i]);
	    //Odblokowanie dostępu do tablicy identyfikatorów
	    pthread_mutex_unlock(&mx);

	    while (1){
		    //Oczekiwanie na zmiennej warunkowej
		    pthread_cond_wait(&nv[i], &arr[i]);
		    x=sendall(th2.csd, prep_command[i]);
		    //Jeśli send się nie powiódł: zwolnienie miejsca w tablicy identyfikatorów, uwolnienie muteksa
		    if (x<0){
			    fprintf(stderr, "Błąd w wysyłce danych.\n");
			    dead[i]=0;
			    pthread_mutex_unlock(&arr[i]);
			    pthread_exit(NULL);
		  }
	    }
	    pthread_mutex_unlock(&arr[i]);
    }

    //komunikat od klienta - poszukiwanie identyfikatorów
    else if (buffer[0]=='2'){
	strcpy(bf2, buffer+2);
	strcat(bf2, "\n");
	char tosend[C];
	//do wysłania do klienta
	strcpy(tosend, "2 ");
	//Zablokowanie dostępu do tablicy identyfikatorów
	pthread_mutex_lock(&mx);
	int j, lst=0, jj, ij;
	//Poszukiwanie w tablicy identyfikatorów kolejnych identyfikatorów podanych przez klienta
	for (j=0;j<C-2;j++){
		if (bf2[j]=='\0')	break;
		if ((bf2[j]<'0' || bf2[j]>'9') && (bf2[j]<'A' || bf2[j]>'Z') && (bf2[j]<'a' || bf2[j]>'z')){
			for (jj=lst;jj<j;jj+=1) tmpb[jj-lst]=bf2[jj];
			tmpb[jj-lst]='\0';
			//Szukanie w tablicy identyfikatorów konkretnego identyfikatora
			for (ij=0;ij<QUEUE_SIZE;ij+=1){
				if (dead[ij]==1 && strcmp(tmpb, valid[ij])==0){
					strcat(tosend, tmpb);
					strcat(tosend, " ");
					break;
			       	}
			}
			lst=jj+1;
		}
	}
	//Odblokowanie dostępu do tablicy identyfikatorów
	pthread_mutex_unlock(&mx);
	//Wysłanie klientowi identyfikatorów, które on wymienił, a które istnieją
	x=sendall(th2.csd, tosend);
	if (x<0) finitosen();
    }

    //komunikat od klienta - shutdown albo reset na zbiorze identyfikatorów
    else if (buffer[0]=='0' || buffer[0]=='1'){
	char final[C];
	strcpy(final, "3 ");
	int it=0, vtime, ope, j, jj, ij, lst=0;
	//ope=0 - shutdown, ope=1 - reset
	if (buffer[0]=='0') ope=0;
	else ope=1;

	//vtime - czas podany przez klienta
	vtime=int_from_pos(&it, buffer+2);
	//bf2 - zbiór identyfikatorów
	strcpy(bf2, buffer+2+it);
	strcat(bf2, "\n");

	//Zablokowanie dostępu do tablicy identyfikatorów
	pthread_mutex_lock(&mx);

	//Poszukiwanie w tablicy identyfikatorów kolejnych identyfikatorów podanych przez klienta
	for (j=0;j<C-2;j++){
		if (bf2[j]=='\0')	break;
		if ((bf2[j]<'n' || bf2[j]>'9') && (bf2[j]<'A' || bf2[j]>'Z') && (bf2[j]<'a' || bf2[j]>'z')){
			for (jj=lst;jj<j;jj+=1) tmpb[jj-lst]=bf2[jj];
			tmpb[jj-lst]='\0';
			//Szukanie w tablicy identyfikatorów konkretnego identyfikatora
			for (ij=0;ij<QUEUE_SIZE;ij+=1){
				if (dead[ij]==1 && strcmp(tmpb, valid[ij])==0){
					//Wysłanie sygnału do wątku odpowiedzialnego za zmatchowany identyfikator - przy okazji odblokowuje i blokuje jego mutexa, tak, aby
					//on na pewno otrzymał tą komendę w trakcie waita
					pthread_mutex_lock(&arr[ij]);
		    			grant_wisdom(prep_command[ij], vtime, ope);
					pthread_cond_signal(&nv[ij]);
					pthread_mutex_unlock(&arr[ij]);
					break;
			       	}
			}
			//Jeśli identyfikatora nie ma - odesłać go w komunikacie do klienta
			if (ij==QUEUE_SIZE) strcat(final, tmpb), strcat(final, " ");
			lst=jj+1;
		}
	}
	//Wysyłka danych do klienta - tylko te identyfikatory, które już znikły (np. bo komputery zostały wyłączone - albo ctrl+c)
	x=sendall(th2.csd, final);
	if (x<0) finitosen();
    	//Odblokowanie dostępu do tablicy identyfikatorów
    	pthread_mutex_unlock(&mx);
    }
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor/*, int dead[], char valid[QUEUE_SIZE][], char prep_command[QUEUE_SIZE][], pthread_mutex_t mx, pthread_mutex_t arr[], pthread_cond_t nv[]*/) {
    //uchwyt na wątek
    pthread_t thread1;
    //dane, które zostaną przekazane do wątku
    struct thread_data_t *t_data;
    t_data = malloc(sizeof t_data);
    t_data->csd = connection_socket_descriptor;

    //Tworzenie wątku
    int create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       fprintf(stderr, "Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(1);
    }
}

int main(int argc, char* argv[]){
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;
   //inicjalizacja gniazda serwera 
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(SERVER_PORT);
   //Stworzenie socketa
   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0){
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
   //Wiązanie socketa z portem i adresem
   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0){
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }
   //Nasłuchiwanie z ograniczeniem do 5 klientów
   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   //Akceptacja klientów
   while(1){
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0){
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }
       handleConnection(connection_socket_descriptor);
   }
   
   close(server_socket_descriptor);
   return(0);
}
