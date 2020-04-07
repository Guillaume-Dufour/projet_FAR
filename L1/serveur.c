#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>


int dS;
int users[2];


void creerSocket(int port) {

	dS = socket(PF_INET,SOCK_STREAM,0);
    
    if(dS == -1) {
    	perror("Erreur lors de la création de la socket");
    	exit(1);
    }
    else {
    	printf("Création de la socket OK\n");
    }

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(port);

   	int res = bind(dS, (struct sockaddr*)&ad, sizeof(struct sockaddr_in));
    
    if(res == -1){
    	perror("Erreur lors du bind");
    	exit(1);
    }
    else {
    	printf("bind OK\n");
    	printf("Serveur en écoute\n");
    }
}

void connexionClients() {

	int nbUsers = 0;
    int dSC;
    int res;

    while(nbUsers < 2) {
    	listen(dS,7);
        struct sockaddr_in aC;
        socklen_t lg=sizeof(struct sockaddr_in);

        dSC = accept(dS, (struct sockaddr*) &aC,&lg);

        if(dSC == -1) { 
        	perror("L'utilisateur n'a pas été accepté");
        }
        else {
        	users[nbUsers] = dSC;

        	res = send(users[nbUsers], &nbUsers, sizeof(int), 0);

        	if (res == -1) {
        		perror("Erreur lors de l'envoi du numéro de client");
        	}

	       	nbUsers++;
	        printf("L'utilisateur %d a été accepté\n", nbUsers);
        }
    }
}


void recevoirMessage(int expediteur, char message[100]) {

	int res = recv(expediteur, message, 100*sizeof(char), 0);

	if (res == -1) {
		perror("Erreur lors de la réception du message");
	}
	else {
		printf("%d octets ont été reçus\n", res);
	}

}

void envoyerMessage(int destinaire, char message[], int tailleMessage) {
        int nbtotalsent = 0;
        int res;
        while(tailleMessage>nbtotalsent){
                res = send(destinaire,message+nbtotalsent,tailleMessage,0);
                if(res==-1){
                        perror("Erreur lors de l'envoi du message");
                        exit(1);
                }else{
                        nbtotalsent = nbtotalsent + res;
                        printf("%d octets ont été envoyés\n", res);
                }
                
        }
     

}

void fin() {

	int res;

	for(int i=0; i<sizeof(users); i++) {
		res = close(users[i]);

		if (res == -1) {
			perror("Erreur lors du close socket clients");
		}
	}

	res = close(dS);

	if (res == -1) {
		perror("Erreur lors du close socket serveur");
	}

	printf("Fin serveur\n");

	exit(0);
}

int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Le nombre de paramètres doit être de 1 (PORT)\n");
		exit(1);
	}

	creerSocket(atoi(argv[1]));

	connexionClients();

    printf("Attente de la réception d'un message\n");

    char message[100];

    signal(SIGINT, fin);

    while (1) {

    	recevoirMessage(users[0], message);
    	envoyerMessage(users[1], message, strlen(message)+1);

    	memset(message, 0, sizeof(message));

    	recevoirMessage(users[1], message);
    	envoyerMessage(users[0], message, strlen(message)+1);

    	memset(message, 0, sizeof(message));
    }

    return 0;
}