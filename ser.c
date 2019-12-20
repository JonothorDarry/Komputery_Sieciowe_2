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
#define QUEUE_SIZE 5
#define C 1024
//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t{
	int csd;
};
//Struktura do przekazania sparsowanej wiedzy
struct wisdom{
	int res;
	int sores;
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

//To samo co poprzednia funkcja, ale wywala błąd po podaniu nie-cyfry
int superint_from_pos(int *p, char a[]){
	int i=*p, id=0;
	while (1){
		if (a[i]=='\n'||a[i]==' '||a[i]=='\0') break;
		if (a[i]>57||a[i]<48) return -1;
		id=id*10+(a[i]-48);
		i+=1;
	}
	*p=i+1;

	return id;
}

//Dostaję tablicę znaków w formacie uid \n guid \n ls -lnH shutdown \n ls -lnH init \n, gdzie uid, guid to id wykonawcy procesu klienta.
//Zwracam: 1 - mogę wykonać shutdowna; 2 - mogę wykonać inita; 0 wpp.
struct wisdom parse_wisdom(char *a){
	int uid=0, gid=0, uid1, gid1, uid2, gid2, i=0, times;
	char scall[C], sc1[C], sc2[C], sc3[C], outer[C], tmp[C], perms1[C], perms2[C];
	FILE *fk, *fk2, *fk3, *fk4;
	struct wisdom xx;

	//Zerowanie tablic
	strcpy(scall, "");
	strcpy(sc1, "");
	strcpy(sc2, "");
	strcpy(sc3, "");
	strcpy(outer, "");
	strcpy(tmp, "");
	strcpy(perms1, "");
	strcpy(perms2, "");
	
	//Wzięcie uida i gida z danych na wejściu
	xx.sores=int_from_pos(&i, a);
	uid=int_from_pos(&i, a);
	gid=int_from_pos(&i, a);

	//Parsowanie tekstu w bashu - wyjmuje do pliku informację o id, gid od shutdowna(inita), a także czas podany przez użytkownika
	strcpy(scall, "echo '");
	strcat(scall, a);
	strcpy(sc1, scall);
	strcpy(sc2, scall);
	strcpy(sc3, scall);
	strcat(scall, "' | tail -4 | head -2 | tr -s ' ' | cut -d ' ' -f 3,4");
	strcat(sc1, "' | tail -4 | head -1 | cut -c 4,7,10");
	strcat(sc2, "' | tail -3 | head -1 | cut -c 4,7,10");
	strcat(sc3, "' | tail -2 | head -1");
	fk=popen(scall, "r");
	fk2=popen(sc1, "r");
	fk3=popen(sc2, "r");
	fk4=popen(sc3, "r");
	
	//Przepis z pliku do tablic
	while (fgets(tmp, sizeof(tmp), fk)!=NULL) strcat(outer, tmp);
	while (fgets(tmp, sizeof(tmp), fk2)!=NULL) strcat(perms1, tmp);	
	while (fgets(tmp, sizeof(tmp), fk3)!=NULL) strcat(perms2, tmp);
	while (fgets(tmp, sizeof(tmp), fk4)!=NULL) strcat(outer, tmp);
	//Domknięcie pliku
	pclose(fk), pclose(fk2), pclose(fk3), pclose(fk4);
	//Znajdowanie kolejnych liczb w tablicy outer
	i=0;
	uid1=int_from_pos(&i, outer);
	gid1=int_from_pos(&i, outer);
	uid2=int_from_pos(&i, outer);
	gid2=int_from_pos(&i, outer);
	times=superint_from_pos(&i, outer);
	//Zwracanie liczby:
	//-1 - podano zły czas
	//>=3 - klient ma prawo do shutdowna
	//2 - klient ma prawo do inita
	//0 - klient ma niedostateczne prawa do zamknięcia systemu tymi poleceniami.
	if (times==-1) 			 	 		  xx.res=-1;
	else if (uid==0) 		 	 		  xx.res=3+times;
	else if (uid==uid1 && (perms1[0]=='x' || perms1[0]=='s')) xx.res=3+times;
	else if (gid==gid1 && (perms1[1]=='x' || perms1[1]=='s')) xx.res=3+times;
	else if (perms1[2]=='x' || perms1[2]=='t') 		  xx.res=3+times;
	else if (uid==uid2 && (perms2[0]=='x' || perms2[0]=='s')) xx.res=2;
	else if (gid==gid2 && (perms2[1]=='x' || perms2[1]=='s')) xx.res=2;
	else if (perms2[2]=='x' || perms2[2]=='t')	 	  xx.res=2;
	
	return xx;
}

//formulacja polecenia dla klienta
void grant_wisdom(char dest[], int res, int purp){
	char vv[10];
	//W dest polecenie dla klienta, vv - pomocnicza tablica to kopiowania inta
	if (res==0||res==2) strcpy(dest, "#Operacja się nie powiodła: Niedostateczne uprawnienia\n");
	else if (res==-1) strcpy(dest, "#Operacja się nie powiodła: Błędny czas\n");
	else if (res>=3&&purp==0) {
		strcpy(dest, "shutdown -P ");
		sprintf(vv, "%d", res-3);
		strcat(dest, vv);
	}
	else if (res>=3&&purp==1) {
		strcpy(dest, "shutdown -r ");
		sprintf(vv, "%d", res-3);
		strcat(dest, vv);
	}
	//else if (res==2&&purp==0) strcpy(dest, "init 0");
	//else if (res==2&&purp==1) strcpy(dest, "init 6");
}


//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data){
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    char buffer[C], bf2[C];
    //dostęp do pól struktury: (*th_data).pole
    int x, y, s1=0;
    while(s1<C){
        //Odbiór całości danych
        y=recv((*th_data).csd, buffer+s1, C-s1, 0);
        if (y<0){
            fprintf(stderr, "Błąd w odbiorze danych.\n");
            exit(-1);
        }
        s1+=y;
    }
    s1=0;
    
    struct wisdom ret=parse_wisdom(buffer);
    printf("%d %d\n", ret.res, ret.sores);
    grant_wisdom(bf2, ret.res, ret.sores);
    
    while(s1<C){
        //Wysyłka całości danych
        x=send((*th_data).csd, bf2+s1, C-s1, 0);
        if (x<0){
            fprintf(stderr, "Błąd w wysyłce danych.\n");
            exit(-1);
        }
        s1+=x;
    }
    
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor) {
    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
    struct thread_data_t *t_data;
    t_data = malloc(sizeof t_data);
    t_data->csd = connection_socket_descriptor;

    int create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       fprintf(stderr, "Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
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
