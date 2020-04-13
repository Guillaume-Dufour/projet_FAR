#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#define TAILLE_MAX 250

int dS;

void connexionSocket(int port, char* ip) {

	dS = socket(PF_INET, SOCK_STREAM, 0);

	if (dS == -1) {
		perror("Erreur dans la création de la socket");
		exit(1);
	}

	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(port);

	int res = inet_pton(AF_INET, ip, &(aS.sin_addr));
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
}


int receptionNumeroClient() {

	int numeroClient;

	int res = recv(dS, &numeroClient, sizeof(int), 0);

	if (res == -1) {
		perror("Erreur lors de la réception du numéro de client");
		exit(1);
	}
	else {
		printf("Vous êtes le client numéro %d\n", numeroClient);
	}

	return numeroClient;
}


int envoyerMessage(int destinaire) {

	char message[TAILLE_MAX];
    int res;


	gets(message);

	// Envoi de la taille du message
	int tailleMessage = strlen(message)+1;
	res = send(destinaire, &tailleMessage, sizeof(int), 0);

    int nbTotalSent = 0;

    while(tailleMessage > nbTotalSent) {

        res = send(destinaire, message+nbTotalSent, tailleMessage, 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du message");
        }
        else{
            nbTotalSent += res;
        }            
    }

    if (strcmp(message, "fin") == 0) {
    	return 0;
    }

    return nbTotalSent;
}

int recevoirMessage(int expediteur) {

	char message[TAILLE_MAX];
	int tailleMessage;

    int res = recv(expediteur, &tailleMessage, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception de la taille du message");
    }

    int nbTotalRecv = 0;

    while (nbTotalRecv < tailleMessage) {
        res = recv(expediteur, message+nbTotalRecv, tailleMessage, 0);

        if (res == -1) {
            perror("Erreur lors de la réception du message");
            memset(message, 0, sizeof(message));
            break;
        }
        else {
            nbTotalRecv += res;
        }
    }

    printf("Message reçu : %s\n", message);

    if (strcmp(message, "fin") == 0) {
    	return 0;
    }

    return nbTotalRecv;
}


int main(int argc, char* argv[]) {

	if (argc != 3) {
		printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
		exit(1);
	}

	connexionSocket(atoi(argv[1]),argv[2]);

	int numeroClient = receptionNumeroClient();

	int res;

	

	int pid;
	//si je suis le fils
	if ((pid = fork()) == 0) {
		while(1){
			res = envoyerMessage(dS);
			if (res == 0) {
				break;
			}
		}
		

	}else{//si je suis le père

		while(1){
			res = recevoirMessage(dS);
			if (res == 0) {
				break;
			}
		}
		
	}
	

	res = close(dS);

	if (res == -1) {
		perror("Erreur close socket");
		exit(1);
	}
	else {
		printf("Fin client\n");
	}

	return 0;
}