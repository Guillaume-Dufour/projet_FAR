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

#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

// Variables globales
int dS; // Socket du serveur pour l'échange des messages
int dS1; // Socket du serveur pour l'échange d'informations avec les clients
int users[NB_CLIENT_MAX]; // Sockets des clients (100 maximum)
int usersInfos[NB_CLIENT_MAX];
char pseudos[NB_CLIENT_MAX][21]; // Pseudos des clients
int nbUsers; // Nombre de clients
int tailleMessage; // Taille du message à envoyer
char message[TAILLE_MAX];
int fin = 0; // Variable qui va servir à arrêter les threads
pthread_t th[NB_CLIENT_MAX];//tableau de threads pour chaque client

// Fonction de création de la socket
int creerSocket(int port) {

    int idSocket = socket(PF_INET,SOCK_STREAM,0);
    
    if(idSocket == -1) {
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

    int res = bind(idSocket, (struct sockaddr*)&ad, sizeof(struct sockaddr_in));
    
    if(res == -1){
        perror("Erreur lors du bind");
        exit(1);
    }
    else {
        printf("bind OK\n");
        printf("Serveur en écoute\n");

        return idSocket;
    }
}



// Envoi aux clients de leur numéro
void envoyerNumeroClient() {

    int numeroClient;
    int res;

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
    printf("taille message :%d\n",tailleMessage );
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
        else {
            printf("envoye: %s\n",message );
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
    
    return nbTotalRecv;
}

// Fermeture des sockets si on fait Ctrl+C
void fini() {

    int res;

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

// Fonction qui reçoit le message de l'expéditeur et qui l'envoie au autres clients
void *user (void* arg) {
    int res;
    while (1) {
        if (fin == 1) {
            break;
        }

        int expediteur = (int) arg;
        res = recevoirMessage(users[expediteur]);

        int envoye=0;//correspond au nombre de messages envoyés
        int i = 0;//correspond à l'itération dans le tableau des utilisateurs pour le parcourir
        // Envoi du pseudo de l'expéditeur et du message à tous les autres clients
        while (envoye < nbUsers-1){
            if (i != expediteur && users[i]!=-1) {
                
                //Envoi du pseudo de l'expéditeur
                    int res2 = send(users[i], pseudos[expediteur], sizeof(pseudos[expediteur]), 0);
                    if (res2 == -1) {
                        perror("Erreur dans le l'envoi du pseudo");
                    }
                    else {
                        //Envoi du message
                        envoyerMessage(users[i]);
                        envoye++;
                    }
                    
                             
            }
            
            i++;
        }            
            
        if (strcmp(message, "fin") == 0) {
            users[expediteur]=-1;
            // On efface le pseudo du client précédent (utile dans le cas où une conversation est relancée)
            memset(pseudos[expediteur], 0, sizeof(pseudos[expediteur]));
            nbUsers--;
            memset(message, 0, sizeof(message));
            break;
        }        

        // On efface le message
        memset(message, 0, sizeof(message));
    }

    pthread_exit(0);
}
int indiceCaseLibre(){
    for (int i = 0; i < nbUsers; i++) {
        if(users[i] == -1){
            return i;
        }
    }

    return nbUsers;
}

void *recevoirDemandeUsers(void* arg) {

    while (1) {
        printf("Traitement de la demande de users\n");
        char message[15];
        int expediteur = (int) arg;

        int res = recv(usersInfos[expediteur], message, sizeof(message), 0);

        if (res == -1) {
            perror("Erreur lors de la réception du message");
        }

        if (strcmp(message, "user342107") == 0) {
            printf("nb d'users %d\n",nbUsers );
            int nb_envoi_file = nbUsers-1;
            int res3 = send(usersInfos[expediteur], &nb_envoi_file, sizeof(int), 0);

            if (res3 == -1) {
                perror("Erreur dans le l'envoi du nb de users");
            }

            for (int i = 0; i < nbUsers; i++) {
                if (i != expediteur){
                    int res3 = send(usersInfos[expediteur],  &users[i], sizeof(int), 0);
                    if (res3 == -1) {
                        perror("Erreur dans le l'envoi de la liste des users");
                    }
                }
                
            }        
        }
    }
    
}

// Fonction permettant que les clients se connectent
void *connexionClients(void* arg) {
    
    nbUsers = 0;
    int dSC;
    int dSC1;
    int res;
    int position;
    pthread_t infos[NB_CLIENT_MAX];

    while(1){
        printf("aaaaa\n");
        position = indiceCaseLibre();
        
        listen(dS,1);
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);

        printf("bbbbbbb\n");
        dSC = accept(dS, (struct sockaddr*) &aC, &lg);

        listen(dS1, 1);
        printf("ccccc\n");
        dSC1 = accept(dS1, (struct sockaddr*) &aC, &lg);


        if (dSC == -1 || dSC1 == -1) { 
            perror("L'utilisateur n'a pas été accepté");
        }
        else {
            position = indiceCaseLibre();
            // On met la socket du client dans le tableau
            users[position] = dSC;
            usersInfos[position] = dSC1;

            // Réception du pseudo du client
            res = recv(users[position], pseudos[position], 20, 0);

            if (res == -1) {
                perror("Erreur lors de la réception du pseudo");
            }
            else {
                res = send(users[position], &position, sizeof(int), 0);
                if (res == -1) {
                    perror("Erreur lors de l'envoi du numéro de client");
                }
                if (pthread_create (&th[position], NULL, user, (void *) position) < 0) {
                    perror("Erreur à la création des threads");
                }
                if (pthread_create (&infos[position], NULL, recevoirDemandeUsers, (void *) position) < 0) {
                    perror("Erreur à la création des threads recevoirDemandeUsers");
                    exit(1);
                }
                nbUsers++;
                printf("L'utilisateur n°%d (%s) a été accepté\n", position, pseudos[position]);
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
    int nbClients=1; // on connecte un client à la fois

    // Création de la socket pour l'échange de messages
    dS = creerSocket(port);

    // Création de la socket pour la demande d'informations au serveur
    dS1 = creerSocket(32565);

    
    // Signal pour le Ctrl+C
    signal(SIGINT, fini);

    do {
        fin = 0;
        pthread_t connexion;

        if (pthread_create (&connexion, NULL, connexionClients, "1")<0) {
            perror("Erreur à la création des threads");
            exit(1);
        }

        void* ret;

        // On attend que tous les clients soient déconnectés avant de relancer le serveur et l'attente de clients
        int numUser = 0;
        while(nbUsers>=0){
              
            if (numUser <nbUsers && nbUsers !=0)
            {
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