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
#include <fcntl.h>
#include <netinet/in.h>
#include "utils.h"

#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

struct {
    int dS;
    int dS2;
} infos;

struct {
    int dSMessage[NB_CLIENT_MAX];
    int dSFichier[NB_CLIENT_MAX];
    char pseudos[NB_CLIENT_MAX][21];
} users;

// Variables globales
int nbUsers;
int fin = 0; // Variable qui va servir à arrêter les threads
pthread_t th[NB_CLIENT_MAX]; // Tableau de threads pour chaque client (message)
pthread_t th2[NB_CLIENT_MAX]; // Tableau de threads pour chaque client (fichier)


// Fonction de création de la socket
int creerSocket(int port) {

    int idSocket = socket(PF_INET,SOCK_STREAM,0);
    
    if(idSocket == -1) {
        perror("Erreur lors de la création de la socket");
        exit(1);
    }
    else {
        printf("Création de la socket %d OK\n", idSocket);
    }

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(port);

    int res = bind(idSocket, (struct sockaddr*)&ad, sizeof(struct sockaddr_in));
    
    if(res == -1) {
        perror("Erreur lors du bind");
        exit(1);
    }
    else {
        printf("Bind OK\n");

        return idSocket;
    }
}


// Fermeture des sockets si on fait Ctrl+C
void fini() {

    int res;

    for(int i=0; i < nbUsers; i++) {
        res = close(users.dSMessage[i]);

        if (res == -1) {
            perror("Erreur lors du close socket message clients");
        }

        res = close(users.dSFichier[i]);

        if (res == -1) {
            perror("Erreur lors du close sockets fichier clients");
        }
    }

    res = close(infos.dS);

    if (res == -1) {
        perror("Erreur lors du close socket message serveur");
    }

    res = close(infos.dS2);

    if (res == -1) {
        perror("Erreur lors du close socket fichier serveur");
    }

    printf("Fin serveur\n");

    exit(0);
}

// Fonction qui reçoit le message de l'expéditeur et qui l'envoie au autres clients
void * userMessage (void *arg) {
    while (1) {
        if (fin == 1) {
            break;
        }

        int expediteur = (int) arg;
        char* message = recvMessage(users.dSMessage[expediteur], TAILLE_MAX);

        int envoye = 0; // Correspond au nombre de messages envoyés
        int i = 0;// Correspond à l'itération dans le tableau des utilisateurs pour le parcourir

        // Envoi du pseudo de l'expéditeur et du message à tous les autres clients
        while (envoye < nbUsers-1){
            if (i != expediteur && users.dSMessage[i]!=-1) {

                //Envoi du pseudo de l'expéditeur
                int res = send(users.dSMessage[i], users.pseudos[expediteur], sizeof(users.pseudos[expediteur]), 0);

                if (res == -1) {
                    perror("Erreur dans le l'envoi du pseudo");
                }
                else {
                    //Envoi du message
                    sendMessage(users.dSMessage[i], message);
                    envoye++;
                }
            }

            i++;
        }

        if (strcmp(message, "fin") == 0) {
            users.dSMessage[expediteur] = -1;
            users.dSFichier[expediteur] = -1;
            // On efface le pseudo du client précédent (utile dans le cas où une conversation est relancée)
            memset(users.pseudos[expediteur], 0, sizeof(users.pseudos[expediteur]));
            nbUsers--;
            break;
        }
    }

    pthread_exit(0);
}


void* userFichier(void* arg) {

    int res;

    while (1) {

        int expediteur = (int) arg;
        int tailleFichier;
        int nbOctetsEnvoye = 0;
        char name[200];

        int res = recv(users.dSFichier[expediteur], name, sizeof(name), 0);

        res = recv(users.dSFichier[expediteur], &tailleFichier, sizeof(int), 0);
        if (res == 0){//si un utilisateur est parti res renverra 0 car l'expéditeur n'existe plus donc on termine le thread

            break;
        }

        if (res == -1) {
            break;
        }

        char buffer[200];
        int envoye = 0;
        int i = 0;
        while (envoye < nbUsers-1){
            if (i != expediteur && users.dSFichier[i]!=-1) {
                res = send(users.dSFichier[i], users.pseudos[expediteur], sizeof(users.pseudos[expediteur]), 0);
                if (res == -1) {
                    pthread_exit(0);
                }

                res =send(users.dSFichier[i],name,sizeof(name),0);
                if(res == -1){
                    pthread_exit(0);
                }


                res = send(users.dSFichier[i], &tailleFichier, sizeof(int), 0);
                printf("envoi de taille\n");
                if (res == -1) {
                    pthread_exit(0);
                }
                envoye++;
            }

            i++;
        }
        printf("Envoi en cours...\n");
        envoye=0;
        i=0;

        while(nbOctetsEnvoye<tailleFichier){
            res = recv(users.dSFichier[expediteur], buffer, sizeof(buffer), 0);

            if(res<0){
                pthread_exit(0);
            }
            while (envoye < nbUsers-1){
                if (i != expediteur && users.dSFichier[i]!=-1) {

                    //Envoi du fichier
                    res = send(users.dSFichier[i], buffer, sizeof(buffer), 0);

                    if (res == -1) {
                        pthread_exit(0);
                    }

                    envoye++;
                }

                i++;
            }
            envoye=0;
            i=0;
            memset(buffer,0,sizeof(buffer));
            nbOctetsEnvoye=nbOctetsEnvoye+res;
        }
        memset(name,0,sizeof(name));

        printf("Fichier transféré\n");

    }
    pthread_exit(0);
}


int indiceCaseLibre(){
    for (int i = 0; i < nbUsers; i++) {
        if(users.dSMessage[i] == -1){
            return i;
        }
    }

    return nbUsers;
}


_Noreturn // Fonction permettant que les clients se connectent
void * connexionClients() {
    
    nbUsers = 0;
    int dSC;
    int dSC2;
    int res;
    int position;

    while(1){
        position = indiceCaseLibre();
        
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);

        listen(infos.dS, 1);
        dSC = accept(infos.dS, (struct sockaddr*) &aC, &lg);

        listen(infos.dS2, 1);
        dSC2 = accept(infos.dS2, (struct sockaddr*) &aC, &lg);

        if (dSC == -1 || dSC2 == -1) { 
            perror("L'utilisateur n'a pas été accepté");
        }
        else {
            position = indiceCaseLibre();
            // On met la socket du client dans le tableau
            users.dSMessage[position] = dSC;
            users.dSFichier[position] = dSC2;

            // Réception du pseudo du client
            res = recv(users.dSMessage[position], users.pseudos[position], 20, 0);

            if (res == -1) {
                perror("Erreur lors de la réception du pseudo");
            }
            else {
                res = send(users.dSMessage[position], &position, sizeof(int), 0);
                if (res == -1) {
                    perror("Erreur lors de l'envoi du numéro de client");
                }
                if (pthread_create (&th[position], NULL, userMessage, (void *) position) > 0) {
                    perror("Erreur à la création des threads pour l'envoi de messsages");
                }
                if (pthread_create (&th2[position], NULL, userFichier, (void *) position) > 0) {
                    perror("Erreur à la création des threads pour l'envoi de fichiers");
                }
                nbUsers++;
                printf("L'utilisateur n°%d (%s) a été accepté\n", position, users.pseudos[position]);
            }
        }
    }
}


int main(int argc, char* argv[]) {

    // Vérification du nombre d'arguments
    if (argc != 2) {
        printf("Le nombre de paramètres doit être de 1 (PORT)\n");
        exit(1);
    }
    
    int port = atoi(argv[1]);

    // Création de la socket pour l'échange de messages
    infos.dS = creerSocket(port);

    // Création de la socket pour l'envoi et la réception de fichier
    infos.dS2 = creerSocket(32566);

    printf("Serveur en écoute\n");

    
    // Signal pour le Ctrl+C
    signal(SIGINT, fini);

    do {
        fin = 0;
        pthread_t connexion;

        if (pthread_create (&connexion, NULL, connexionClients, NULL) > 0) {
            perror("Erreur à la création des threads");
            exit(1);
        }

        void* ret;

        // On attend que tous les clients soient déconnectés avant de relancer le serveur et l'attente de clients
        int numUser = 0;

        while (nbUsers >= 0) {
            if (numUser < nbUsers && nbUsers != 0) {
                if (pthread_join(th[numUser], &ret) != 0){
                    perror("Erreur dans l'attente du thread");
                    exit(1);
                }
                numUser++;
            }
        }
            
    } while (1);        

    return 0;
}