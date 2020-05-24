#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"

#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

// Informations sur le client
struct {
    int dS;
    int dS1; // Socket pour l'échange de messages
    int dS2; // Socket pour l'échange de fichiers
    int numeroClient; // Numéro du client dans le serveur
    char* pseudo; // Pseudo du client
    char pseudoExpediteur[21];
} infos;

// Variables globales
char* ip;
int fin = 0; // Variable pour savoir si le client doit s'arrêter
int nbUsers; // Nombre de clients

// Fonction de création de la socket et connexion à la socket
int connexionSocket(int port, char* ip) {

    int idSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (idSocket == -1) {
        perror("Erreur dans la création de la socket");
        exit(1);
    }

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    aS.sin_port = htons(port);

    int res = inet_pton(AF_INET, ip, &(aS.sin_addr));

    if (res == -1) {
        perror("Erreur dans la création de la structure d'adresse réseau");
        exit(1);
    }
    else if (res == 0) {
        perror("Adresse réseau invalide");
        exit(1);
    }

    socklen_t lgA = sizeof(aS);

    res = connect(idSocket, (struct sockaddr *) &aS, lgA);

    if (res == -1) {
        perror("Erreur à la demande de connexion");
        exit(1);
    }
    else {
        //printf("Connexion réussie à la socket %d\n", idSocket);

        return idSocket;
    }
}

// Fonction permettant de recevoir son numéro de client
int receptionNumeroClient(int expediteur) {

    int numeroClient;

    int res = recv(expediteur, &numeroClient, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception du numéro de client");
        exit(1);
    }
    else {
        printf("Vous êtes le client numéro %d\n", numeroClient);
    }

    return numeroClient;
}

void fexit(FILE* file, char* successMessage, int codeRetour) {

    int res = fclose(file);

    if (res == EOF) {
        fprintf(stderr, "Le fichier ne peut pas être fermé\n");
    }
    else {
        printf("%s\n", successMessage);
    }

    if (codeRetour != 0) {
        exit(codeRetour);
    }
}


// Fonction d'envoi de fichier
void sendFile(int destinataire, char* name) {

    char loc[200] = "./files/";
    strcat(loc, name);

    FILE* file = fopen(loc, "r");

    if (file == NULL) {
        printf("Erreur lors de l'ouverture du fichier");
        exit(1);
    }

    int fsize = open(loc,O_RDONLY);

    if (fsize == -1) {
        perror("Erreur open");
        exit(1);
    }

    int res = send(destinataire, name, strlen(name), 0);

    if (res == -1) {
        perror("Erreur lors de l'envoi du nom du fichier");
        fexit(file, "Erreur avant envoi", 1);
    }

    struct stat file_stat;

    if (fstat(fsize, &file_stat) == -1) {
        perror("Erreur lors de l'obtention des stats");
        fexit(file, "Erreur avant envoi", 1);
    }

    int taille = file_stat.st_size;

    int part = send(destinataire, &taille, sizeof(int),0);

    if (part == -1) {
        perror("Erreur lors de l'envoi de la taille duf fichier");
        fexit(file, "Erreur avant envoi", 1);
    }

    int envoye = 0;
    char buff[200];

    while (envoye < taille) {
        part = fread(buff,1,200, file);

        if (part != 200) {
            fprintf(stderr, "Erreur de la lecture du fichier\n");
        }

        res = send(destinataire, buff, sizeof(buff), 0);

        if (res < 0) {
            perror("Erreur lors de l'envoi du contenu du fichier");
            fexit(file, "Erreur pendant envoi", 1);
        }

        if (res < 200) {
            if (feof(file)){
                printf("Fin de fichier\n");
                break;
            }
            if (ferror(file)){
                printf("Erreur lors de la lecture du fichier\n");
                fexit(file, "Erreur pendant envoi", 1);
            }
        }

        memset(buff, 0, sizeof(buff));
        envoye = envoye + res;
    }

    fexit(file, "Fichier envoyé avec succès !", 0);
}


// Fonction pour récupérer un fichier
void getFile(int expediteur) {

    char loc[200] = "./target/";
    char name[200];

    int res = recv(expediteur, name, sizeof(name), 0);

    if (res == -1) {
        perror("Erreur lors de la réception du nom de fichier");
        exit(1);
    }

    char newfile[200];
    strcpy(newfile, infos.pseudo);
    strcat(newfile, "_");
    strcat(newfile, name);
    strcat(loc, newfile);

    FILE* fichierRecu = fopen(loc, "w+");

    if (fichierRecu == NULL) {
        perror("Erreur lors de la création du fichier");
        exit(1);
    }

    int taille;

    res = recv(expediteur, &taille, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception de la taille du fichier");
        fexit(fichierRecu, "Erreur avant réception", 1);
    }

    int recu = 0;
    int byte = 0;
    char part[200];

    while (recu <= taille) {

        byte = recv(expediteur, part, sizeof(part), 0);

        byte = fwrite(part, 1, 200, fichierRecu);

        if (byte != 200){
            if (feof(fichierRecu)){
                printf("Fin de fichier\n");
                break;

            }
            if (ferror(fichierRecu)){
                printf("Erreur de lecture\n");
                fexit(fichierRecu, "Erreur pendant réception", 1);
            }

        }

        memset(part, 0, sizeof(200));
        recu = recu + byte;
    }

    memset(name, 0, sizeof(name));

    fexit(fichierRecu, "Fichier reçu avec succès !", 0);
}


// Thread pour l'envoi de fichier
void * envoyerFichier(void * arg) {
    char* name = (char*) arg;

    int res = send(infos.dS2, &infos.numeroClient, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de l'envoi du numéro de client");
    }

    sendFile(infos.dS2, name);
    pthread_exit(0);
}

_Noreturn void * recevoirFichier(void * arg) {

    int res;

    while(1) {

        res = recv(infos.dS2, infos.pseudoExpediteur, sizeof(infos.pseudoExpediteur), 0);

        if (res == -1) {
            perror("Erreur lors de la réception du pseudo de l'expéditeur");
        }

        getFile(infos.dS2);
        printf("Fichier reçu de %s\n", infos.pseudoExpediteur);

        // On efface le pseudo de l'expéditeur
        memset(infos.pseudoExpediteur, 0, sizeof(infos.pseudoExpediteur));
    }

    pthread_exit(0);
}

// Fonction permettant de choisir le fichier à envoyer et de lancer le thread d'envoi
void debutEnvoiFile() {

    printf("Quel fichier voulez-vous envoyer ?\n");

    struct dirent *dir;
    char* fichier = malloc(200);
    char loc[200] = "./files/";

    // Affichage des fichiers disponibles
    DIR* directory = opendir(loc);

    if (directory) {
        while ((dir = readdir(directory)) != NULL) {
            if (dir->d_name[0] != '.') {
                printf("\t%s\n", dir->d_name);
            }
        }

        closedir(directory);
    }

    do {
        printf("Entrez le nom du fichier : ");
        fichier = saisie(200);

    } while(strlen(fichier)>199 || contientFichier(loc, fichier) == 0);

    pthread_t th;

    // Lancement du thread d'envoi de fichier
    if (pthread_create(&th, NULL, envoyerFichier, (void *) fichier) > 0) {
        perror("Erreur lors de la création du thread d'envoi de fichier");
        exit(1);
    }
}


// Fonction pour le thread qui envoie un message
void * envoyerMessage (void * arg) {

    int res;

    res = send(infos.dS1, &infos.numeroClient, sizeof(int), 0);

    sendTCP(infos.dS1, "begin238867");


    if (res == -1) {
        perror("Erreur lors de l'envoi du numéro de client");
    }

    while(1) {

        if (fin == 1) {
            break;
        }

        char* message = malloc(1000);

        // Saisie du message
        do {
            message = saisie(1000);

            if (strlen(message) >= 1000) {
                printf("Votre message est trop long (1000 caractères maximum)\n");
            }
        } while (strlen(message) >= 1000);

        if (fin == 1) {

            res = send(infos.dS1, &infos.numeroClient, sizeof(int), 0);

            if (res == -1) {
                perror("Erreur lors de l'envoi du numéro de client");
            }

            sendTCP(infos.dS1, "fin");

            break;
        }

        // Si "file" est envoyé cela signifie que l'on veut envoyer un fichier, on lance donc le thread d'envoi de fichier
        if (strcmp(message, "file") == 0){
            debutEnvoiFile();
        }
        else {

            res = send(infos.dS1, &infos.numeroClient, sizeof(int), 0);

            if (res == -1) {
                perror("Erreur lors de l'envoi du numéro de client");
            }

            sendTCP(infos.dS1, message);
        }

        // On stoppe le thread quand on envoie "fin"
        if (strcmp(message, "fin") == 0 || strcmp(message, "/suppSalon") == 0 || fin == 1) {
            fin = 1;
            break;
        }

        free(message);
    }

    printf("Fin de la communication\n");
    pthread_exit(0);
}

//Fonction pour le thread qui reçoit un message
void * recevoirMessage (void * arg) {

    int res;

    while(1) {

        if (fin == 1) {
            break;
        }

        res = recv(infos.dS1, infos.pseudoExpediteur, sizeof(infos.pseudoExpediteur), 0);

        if (res == -1) {
            perror("Erreur lors de la réception du pseudo de l'expéditeur");
            exit(1);
        }

        char* message = recvTCP(infos.dS1, TAILLE_MAX);

        if (strcmp(message, "fin") == 0) {
            printf("%s a quitté la conversation\n", infos.pseudoExpediteur);
        }
        else if(strcmp(message, "begin238867") == 0) {
            printf("%s a rejoint la conversation\n", infos.pseudoExpediteur);
        }
        else if (strcmp(message, "fin1523265") == 0) {
            printf("Le salon a été supprimé par %s\n", infos.pseudoExpediteur);
            fin = 1;
        }
        else {
            printf("Message reçu de %s : %s\n", infos.pseudoExpediteur, message);
        }

        // On efface le pseudo de l'expéditeur
        memset(infos.pseudoExpediteur, 0, sizeof(infos.pseudoExpediteur));
    }

    pthread_exit(0);
}


void choixAction(int expediteur) {

    int choix = 0;
    int nbSalons;

    int res = recv(expediteur, &nbSalons, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception du nombre de salons disponibles");
        exit(1);
    }
    else {
    }

    char* listeSalons = recvTCP(expediteur, 1000);

    printf("Que voulez-vous faire ?\n");
    printf("\tCréer un salon (tapez 1)\n");
    printf("\tRejoindre un salon (tapez 2)\n");

    do {
        printf("Choix : ");
        choix = saisieInt();
    } while (choix != 1 && choix != 2);

    char* nomSalon;

    if (choix == 1) {

        do {
            printf("\n(Taille max: 50, min : 1)\n" );
            printf("Entrez le nom du salon : ");
            nomSalon = saisie(50);

            if (strlen(nomSalon) >= 50) {
                printf("Le nom du salon que vous avez entré est trop long (50 caractères maximum)\n");
            }
        } while (strlen(nomSalon) >= 50 || strlen(nomSalon) < 1);

        choix = 0;

        res = send(expediteur, &choix, sizeof(int), 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du choix");
            exit(1);
        }

        res = send(expediteur, nomSalon, strlen(nomSalon), 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du nom du salon");
            exit(1);
        }
    }
    else {
        printf("\nListe des salons (%d) :\n\n%s\n", nbSalons, listeSalons);

        do {
            printf("Numéro du salon à rejoindre : ");
            choix = saisieInt();
        } while (choix <= 0 || choix > nbSalons);

        printf("Vous allez rejoindre le salon numéro %d\n", choix);

        res = send(expediteur, &choix, sizeof(int), 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du numéro de salon à rejoindre");
            exit(1);
        }
    }

    int port;

    res = recv(expediteur, &port, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception du numéro de port (socket fichier)");
        exit(1);
    }

    infos.dS1 = connexionSocket(port, ip);

    res = recv(expediteur, &port, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception du numéro de port (socket fichier)");
        exit(1);
    }

    infos.dS2 = connexionSocket(port, ip);
}



void fini() {

    int res = close(infos.dS);

    if (res == -1) {
        perror("Erreur close socket messages");
        exit(1);
    }

    res = close(infos.dS2);

    if (res == -1) {
        perror("Erreur close socket fichiers");
        exit(1);
    }
    else {
        printf("Fin client\n");
    }

    exit(1);
}

int main(int argc, char* argv[]) {

    // Vérification du nombre d'arguments
    if (argc != 3) {
        printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
        exit(1);
    }

    // Saisie du pseudo
    do {
        printf("(Taille max: 20, min : 1)\n" );
        printf("Entrez votre pseudo : ");
        infos.pseudo = saisie(20);

        if (strlen(infos.pseudo) >= 20) {
            printf("Votre pseudo est trop long (20 caractères maximum)\n");
        }
    } while (strlen(infos.pseudo) >= 20 || strlen(infos.pseudo) < 1);

    int port = atoi(argv[1]);
    ip = argv[2];

    infos.dS = connexionSocket(port, ip);

    signal(SIGINT, fini);

    int res;

    // Envoi du pseudo au serveur
    res = send(infos.dS, infos.pseudo, strlen(infos.pseudo), 0);

    if (res == -1) {
        perror("Erreur lors de l'envoi du pseudo");
        exit(1);
    }

    choixAction(infos.dS);

    infos.numeroClient = receptionNumeroClient(infos.dS);

    // Création des 2 threads
    pthread_t th1, th2, th3;
    void *ret;

    if (pthread_create(&th1, NULL, recevoirMessage, "1") > 0) {
        perror("Erreur lors de la création du thread 1");
        exit(1);
    }

    if (pthread_create(&th2, NULL, envoyerMessage, "2") > 0) {
        perror("Erreur lors de la création du thread 2");
        exit(1);
    }

    if (pthread_create(&th3, NULL, recevoirFichier, "3") > 0) {
        perror("Erreur lors de la création du thread 3");
        exit(1);
    }

    // On attend que les deux threads soient terminés
    if (pthread_join(th2, &ret) != 0) {
        perror("Erreur dans l'attente du thread");
        exit(1);
    }

    res = close(infos.dS);

    if (res == -1) {
        perror("Erreur close socket messages");
        exit(1);
    }

    res = close(infos.dS2);

    if (res == -1) {
        perror("Erreur close socket fichiers");
        exit(1);
    }
    else {
        printf("Fin client\n");
    }

    return 0;
}