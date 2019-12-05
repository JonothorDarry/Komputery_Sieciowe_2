#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void attain_wisdom(char a[]){	
	char bf[1024], rn[1024], df[1024], dk[1024];
	FILE *fp, *fp2, *fp3;

	fp=popen("sh -c 'which shutdown; which init' | xargs -I{} ls -lnH {}", "r");
	if (fp==NULL){
		printf("Failed running command");
		exit(1);
	}
	
	fp3=popen("id -u; id -g", "r");
	fgets(dk, sizeof(dk), fp3);
	strcat(a, dk);	
	fgets(dk, sizeof(dk), fp3);
	strcat(a,dk);

	while (fgets(bf, sizeof(bf), fp)!=NULL){
		strcat(a, bf);
	}
	//pclose(fp);
	//pclose(fp2);
	//pclose(fp3);
}

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
	if (perms1[2]=='x') return 2;
	if (uid==uid2 && perms2[0]=='x') return 2;
	if (gid==gid2 && perms2[1]=='x') return 2;
	if (perms2[2]=='x') return 2;
	
	return 0;
	//printf("%d %d %d %d %d %d %c %c\n", uid, gid, uid1, gid1, uid2, gid2, perms2[0], perms2[1]);
}

void grant_wisdom(char dest[], int res, int purp){
	if (res==0) strcpy(dest, "false");
	if (res==1&&purp==0) strcpy(dest, "shutdown -P");
	if (res==1&&purp==1) strcpy(dest, "shutdown -r");
	if (res==2&&purp==0) strcpy(dest, "init 0");
	if (res==2&&purp==1) strcpy(dest, "init 6");
}


int main(){
	char a[5024];
	attain_wisdom(a);
	//printf("%s", a);
	parse_wisdom(a);

return 0;}
