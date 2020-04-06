#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>


int sendTCP(int sock, char *msg, int sizeOctet, int option){
	int nbtotalsent = 0;
	int res;
	while(sizeOctet>nbtotalsent){
		res = send(sock,msg+nbtotalsent,strlen(msg),0);
		printf("%d\n", res);
		if(res==-1){
			return res;
		}else{
			nbtotalsent = nbtotalsent + res;
		}
		
	}
	return nbtotalsent;
	
}



int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
		exit(0);
	}

	int dS = socket(PF_INET, SOCK_STREAM, 0);

	if (dS == -1) {
		perror("Erreur dans la création de la socket");
		exit(0);
	}

	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(atoi(argv[1]));

	int res = inet_pton(AF_INET,argv[2],&(aS.sin_addr));
	if (res == -1) {
		perror("Erreur dans la création de la structure d'adresse réseau");
		exit(0);
	}
	else if (res == 0) {
		perror("Adresse réseau invalide");
		exit(0);
	}

	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *) &aS, lgA);

	if (res == -1) {
		perror("Erreur à la demande de connexion");
		exit(0);
	}
	else {
		printf("Connexion réussie\n");
	}


	char message[100];

	printf("Message : ");
	scanf("%s",message);
	res = sendTCP(dS, message, strlen(message)+1, 0);
		
	if (res == -1) {
		perror("Erreur lors de l'envoi");
	}
	else {
		printf("%d caractères ont été envoyés\n", res);
	}		
	

	return 0;
}