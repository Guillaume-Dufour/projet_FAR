#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define TAILLE_MAX 1000 // Taille maximum d'un message

// Variables globales
int dS; // Socket du serveur
int users[2]; // Sockets des clients
int tailleMessage; // Taille du message à envoyer
char message[TAILLE_MAX];
int fin = 0; // Variable qui va servir à arrêter les threads

// Fonction de création de la socket
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

// Fonction permettant que les clients se connectent
void connexionClients(int nbClients) {

	int nbUsers = 0;
    int dSC;
    int res;

    // On attend que tous les clients soient connectés
    while(nbUsers < nbClients) {

    	listen(dS, nbClients);
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);

        dSC = accept(dS, (struct sockaddr*) &aC, &lg);

        if (dSC == -1) { 
        	perror("L'utilisateur n'a pas été accepté");
        }
        else {
        	// On met la socket du client dans le tableau
        	users[nbUsers] = dSC;
	       	nbUsers++;
	        printf("L'utilisateur %d a été accepté\n", nbUsers);
        }
    }
}

// Envoi aux clients de leur numéro
void envoyerNumeroClient() {

    int numeroClient;
    int res;

    int nbUsers = sizeof(users)/sizeof(int);

    for(int i = 0; i < nbUsers; i++) {
        numeroClient = i+1;

        res = send(users[i], &numeroClient, sizeof(int), 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du numéro de client");
        }
    }
}

// Envoi du message au destinataire voulu
int envoyerMessage(int destinaire) {

    // Envoi de la taille du message
    int tailleMessage = strlen(message)+1;
    int res = send(destinaire, &tailleMessage, sizeof(int), 0);

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
            printf("%d octets ont été envoyés\n", res);
        }            
    }

    return nbTotalSent;
}

// Réception du message
int recevoirMessage(int expediteur) {

	// Réception de la taille du message permettant de savoir quand tout le message sera reçu
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

    // Si "fin" est reçu
    if (strcmp(message, "fin") == 0) {
        return 0;
    }

    return nbTotalRecv;
}

// Fermeture des sockets si on fait Ctrl+C
void fini() {

	int res;

    int nbUsers = sizeof(users)/sizeof(int);

	for(int i=0; i < nbUsers; i++) {
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

// Fonction pour le thread qui reçoit de l'utilisateur 0 et qui envoie à l'utilisateur 1
void *user1 (void * arg) {

    int res;

    while(1) {
        if (fin == 1) {
        	break;
        }

        // Réception du message de l'utilisateur 0
        res = recevoirMessage(users[0]);

        // Envoi du message à l'utilisateur 1
        envoyerMessage(users[1]);

        if (res == 0 || fin == 1) {
            fin = 1;
            break;
        }

        // On efface le message
        memset(message, 0, sizeof(message));
    }

    pthread_exit(0);
}

// Fonction pour le thread qui reçoit de l'utilisateur 1 et qui envoie à l'utilisateur 0
void *user2 (void * arg) {

    int res;

    while(1) {
        if (fin == 1) {
        	break;
        }
        
        // Réception du message de l'utilisateur 1
        res = recevoirMessage(users[1]);

        // Envoi du message à l'utilisateur 0
        envoyerMessage(users[0]);

        if (res == 0 || fin ==1) {
            fin =1;
            break;
        }

        // On efface le message
        memset(message, 0, sizeof(message));
    }

    pthread_exit(0);
}

int main(int argc, char* argv[]) {

	// Vérification du nombre d'arguments
	if (argc != 2) {
		printf("Le nombre de paramètres doit être de 1 (PORT)\n");
		exit(1);
	}

	int port = atoi(argv[1]);

    creerSocket(port);
    
    // Signal pour le Ctrl+C
    signal(SIGINT, fini);

    do {
        fin = 0;
        printf("En attente de clients\n");

        connexionClients(2);
        envoyerNumeroClient();

        printf("Attente de la réception d'un message\n");

        // Création des 2 threads
        pthread_t th1, th2;
        void *ret;

        if (pthread_create(&th1, NULL, user1, "1") < 0) {
            perror("Erreur lors de la création du thread pour le user1");
       		exit(1);
        }

        if (pthread_create(&th2, NULL, user2, "2") < 0) {
            perror("Erreur lors de la création du thread pour le user2");
       		exit(1);
        }

        // On attend qu'ils soient terminés avant de relancer le serveur et l'attente de clients
        if (pthread_join(th1, &ret) != 0){
            perror("Erreur dans l'attente du thread");
            exit(1);
        }

        if (pthread_join(th2, &ret) != 0){
            perror("Erreur dans l'attente du thread");
            exit(1);
        }

    } while(1);    	

    return 0;
}