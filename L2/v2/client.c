#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#define TAILLE_MAX 1001 // Taille maximum d'un message

// Variables globales
char pseudo[21]; // Pseudo du client
char pseudoExpediteur[21]; // Pseudo de l'expéditeur du message
int dS;
int fin = 0; // Variable pour savoir si le client doit s'arrêter

// Fonction de création de la socket et connexion à la socket
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

// Fonction permettant de recevoir son numéro de client
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

// Fonction permettant d'envoyer un message
int envoyerMessage(int destinaire) {

	char message[TAILLE_MAX];
    int res;

	// Saisie du message
    do {
    	if (strlen(message) >= 1000) {
    		printf("Votre message est trop long (1000 caractères maximum)\n");
    	}

		gets(message);

    } while (strlen(message) >= 1000);

	// Envoi de la taille du message
	int tailleMessage = strlen(message)+1;
	res = send(destinaire, &tailleMessage, sizeof(int), 0);

	if (res == -1) {
		perror("Erreur lors de l'envoi de la taille du message");
	}

    int nbTotalSent = 0;

    // Envoi du message par packet si le message est trop grand pour le buffer
    while(tailleMessage > nbTotalSent) {

        res = send(destinaire, message+nbTotalSent, tailleMessage, 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du message");
        }
        else{
            nbTotalSent += res;
        }            
    }

    // Si "fin" est envoyé, le client est arrêté
    if (strcmp(message, "fin") == 0) {
    	return 0;
    }

    return nbTotalSent;
}

// Fonction permettant de recevoir un message
int recevoirMessage(int expediteur) {

	char message[TAILLE_MAX];
	// Réception de la taille du message permettant de savoir quand tout le message sera reçu
	int tailleMessage;
    int res = recv(expediteur, &tailleMessage, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception de la taille du message");
    }

    int nbTotalRecv = 0;

    // Réception du message par packet si le message est trop grand pour le buffer
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
    if (strcmp(message, "fin") == 0) {
    	printf("%s a quitté la conversation\n",pseudoExpediteur  );
    }else if(strcmp(message, "begin238867") == 0){
    	printf("%s a rejoint la conversation\n",pseudoExpediteur );
	}else{
	    printf("Message reçu de %s : %s\n", pseudoExpediteur, message);

	    return nbTotalRecv;
	}
}

// Fonction pour le thread qui envoie un message
void *envoyer (void * arg) {   

    int res;
    char message[15] = "begin238867";
	int tailleMessage = strlen(message)+1;
    //envoi du signal d'arrivée du client
    res = send(dS, &tailleMessage, sizeof(int), 0);
    if (res == -1) {
		perror("Erreur lors de l'envoi de la taille du message");
	}
    res = send(dS, message, tailleMessage, 0);
    if (res == -1) {
		perror("Erreur lors de l'envoi du message");
	}
    while(1) {
		if (fin == 1) {
			break;
		}

		res = envoyerMessage(dS);

		// On stoppe le thread quand on envoie "fin" 
		if (res == 0 || fin ==1) {
			fin = 1;
			break;
		}
	}
	printf("fin de la communication\n");
	pthread_exit(0);
}

//Fonction pour le thread qui reçoit un message
void *recevoir (void * arg) {

    int res;

    while(1){
    	if (fin == 1) {
    		break;
    	}

    	res = recv(dS, &pseudoExpediteur, sizeof(pseudoExpediteur), 0);

    	if (res == -1) {
    		perror("Erreur lors de la réception du pseudo de l'expéditeur");
    	}

		res = recevoirMessage(dS);

		// On efface le pseudo de l'expéditeur
		memset(pseudoExpediteur, 0, sizeof(pseudoExpediteur));
	}
	
	pthread_exit(0);
}

int main(int argc, char* argv[]) {

	// Vérification du nombre d'arguments
	if (argc != 3) {
		printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
		exit(1);
	}

	// Saisie du pseudo
	do {
		if (strlen(pseudo) >= 20 ) {
			printf("Votre pseudo est trop long (20 caractères maximum)\n");
		}
		printf("(taille max: 20, min : 1)\n" );
		printf("Entrez votre pseudo : ");
		gets(pseudo);

	} while (strlen(pseudo) >= 20 || strlen(pseudo) < 1 );

	int port = atoi(argv[1]);
	char* ip = argv[2];

	connexionSocket(port, ip);

	int res;

	// Envoi du pseudo au serveur
	res = send(dS, pseudo, strlen(pseudo), 0);

	if (res == -1) {
		perror("Erreur lors de l'envoi du pseudo");
	}

	int numeroClient = receptionNumeroClient();

	

	// Création des 2 threads
	pthread_t th1, th2;
	void *ret;

	if (pthread_create(&th1, NULL, recevoir, "1") < 0) {
        perror("Erreur lors de la création du thread 1");
        exit(1);
    }
    
    if (pthread_create(&th2, NULL, envoyer, "2") < 0) {
        perror("Erreur lors de la création du thread 2");
        exit(1);
    }

	
	// On attend que les deux threads soient terminés
	if (pthread_join(th2, &ret) != 0){
  		perror("Erreur dans l'attente du thread");
  		exit(1);
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