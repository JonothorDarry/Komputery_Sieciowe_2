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

#define SERVER_PORT 1234
#define QUEUE_SIZE 5
#define C 2048

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t{
	int csd;
};

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

int parse_wisdom(char a[]){
	int uid=0, gid=0, uid1, gid1, uid2, gid2, i=0, v;
	char scall[5024], sc1[4024], sc2[4024], outer[5024], tmp[1024], perms1[1024], perms2[1024];
	FILE *fk, *fk2, *fk3;
	
	uid=int_from_pos(&i, a);
	gid=int_from_pos(&i, a);

	strcpy(scall, "echo '");
	strcat(scall, a);
	strcpy(sc1, scall);
	strcpy(sc2, scall);
	strcat(scall, "' | tail -3 | head -2 | tr -s ' ' | cut -d ' ' -f 3,4");
	strcat(sc1, "' | tail -3 | head -1 | cut -c 4,7,10");
	strcat(sc2, "' | tail -2 | head -1 | cut -c 4,7,10");
	fk=popen(scall, "r");
	fk2=popen(sc1, "r");
	fk3=popen(sc2, "r");
	
	while (fgets(tmp, sizeof(tmp), fk)!=NULL) strcat(outer, tmp);
	while (fgets(tmp, sizeof(tmp), fk2)!=NULL) strcat(perms1, tmp);	
	while (fgets(tmp, sizeof(tmp), fk3)!=NULL) strcat(perms2, tmp);
	
	i=0;
	uid1=int_from_pos(&i, outer);
	gid1=int_from_pos(&i, outer);
	uid2=int_from_pos(&i, outer);
	gid2=int_from_pos(&i, outer);	
	
	if (uid==0) return 1;
	if (uid==uid1 && perms1[0]=='x') return 1;
	if (gid==gid1 && perms1[1]=='x') return 1;
	if (perms1[2]=='x') return 1;
	if (uid==uid2 && perms2[0]=='x') return 2;
	if (gid==gid2 && perms2[1]=='x') return 2;
	if (perms2[2]=='x') return 2;
	
	return 0;
	//printf("%d %d %d %d %d %d %c %c\n", uid, gid, uid1, gid1, uid2, gid2, perms2[0], perms2[1]);
}

void grant_wisdom(char dest[], int res, int purp){
	if (res==0) strcpy(dest, "false\n");
	else if (res==1&&purp==0) strcpy(dest, "shutdown -P\n");
	else if (res==1&&purp==1) strcpy(dest, "shutdown -r\n");
	else if (res==2&&purp==0) strcpy(dest, "init 0\n");
	else if (res==2&&purp==1) strcpy(dest, "init 6\n");
}


//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data){
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    char buffer[C], bf2[C];
    //dostęp do pól struktury: (*th_data).pole
    //TODO (przy zadaniu 1) klawiatura -> wysyłanie albo odbieranie -> wyświetlanie
    recv((*th_data).csd, buffer, C, 0);
    int ret=parse_wisdom(buffer);
    grant_wisdom(bf2, ret, 1);    
    send((*th_data).csd, bf2, C, 0);
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor) {
    //wynik funkcji tworzącej wątek
    char buffer[2024], bufferf[2024];
    int create_result = 0;

    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
    //TODO dynamiczne utworzenie instancji struktury thread_data_t o nazwie t_data (+ w odpowiednim miejscu zwolnienie pamięci)
    //TODO wypełnienie pól struktury
    struct thread_data_t *t_data;
    t_data = malloc(sizeof t_data);
    t_data->csd = connection_socket_descriptor;

    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
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

   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   while(1)
   {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0)
       {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }

       handleConnection(connection_socket_descriptor);
   }

   close(server_socket_descriptor);
   return(0);
}
