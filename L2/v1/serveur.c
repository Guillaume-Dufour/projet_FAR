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

#define TAILLE_MAX 1000


int dS;
int users[2];
int tailleMessage;
char message[TAILLE_MAX];
int fin=0;//variable qui va servir à arrêter les threads

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

void connexionClients(int nbClients) {

	int nbUsers = 0;
    int dSC;
    int res;
    while(nbUsers < nbClients) {

    	listen(dS,7);
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);

        dSC = accept(dS, (struct sockaddr*) &aC, &lg);

        if (dSC == -1) { 
        	perror("L'utilisateur n'a pas été accepté");
        }
        else {
        	users[nbUsers] = dSC;
	       	nbUsers++;
	        printf("L'utilisateur %d a été accepté\n", nbUsers);
        }
    }
}


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


int envoyerMessage(int destinaire) {

    // Envoi de la taille du message
    int tailleMessage = strlen(message)+1;
    int res = send(destinaire, &tailleMessage, sizeof(int), 0);

    int nbTotalSent = 0;

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

int recevoirMessage(int expediteur) {

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

    if (strcmp(message, "fin") == 0) {
        return 0;
    }

    return nbTotalRecv;
}

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

void *user1 (void * arg)//pour le thread qui reçoit de l'utilisateur 0 et qui envoi à l'utilisateur 1
{
    int res;
    while (1) {
        if (fin == 1)
            {
                break;
            }

        res = recevoirMessage(users[0]);
        envoyerMessage(users[1]);
        if (res == 0 || fin == 1) {
            fin =1;
            break;
        }
       memset(message, 0, sizeof(message));
    }

    pthread_exit (0);
}

void *user2 (void * arg)//pour le thread qui reçoit de l'utilisateur 1 et qui envoi à l'utilisateur 0
{   
    int res;
    while(1){
        if (fin == 1)
            {
                break;
            }
        res = recevoirMessage(users[1]);
        envoyerMessage(users[0]);
        if (res == 0 || fin ==1) {
            fin =1;
            break;
        }

        memset(message, 0, sizeof(message));
    }
    pthread_exit (0);
}
int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Le nombre de paramètres doit être de 1 (PORT)\n");
		exit(1);
	}

    

    creerSocket(atoi(argv[1]));
    
    signal(SIGINT, fini);

    do {
        fin = 0;
        printf("En attente de clients\n");

        connexionClients(2);
        envoyerNumeroClient();

        printf("Attente de la réception d'un message\n");

        pthread_t th1, th2;//on créé les 2 threads
        void *ret;
        if (pthread_create (&th1, NULL, user1, "1") < 0) {
            fprintf (stderr, "pthread_create error for thread 1\n");
            exit (1);
        }
        if (pthread_create (&th2, NULL, user2, "2") < 0) {
            fprintf (stderr, "pthread_create error for thread 2\n");
            exit (1);
        }
        //on attend qu'ils soient terminés avant de relancer le serveur et l'attente de clients
        if(pthread_join (th1, &ret) != 0){
            perror("erreur dans l'attente du thread");
            exit(1);
        }
        if(pthread_join (th2, &ret) != 0){
            perror("erreur dans l'attente du thread");
            exit(1);
        }
            
        }while (1);

    	

    return 0;
}