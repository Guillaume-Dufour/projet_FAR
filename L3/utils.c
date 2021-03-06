#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <dirent.h>

char* saisie(int maxInput) {

    char* input = malloc(maxInput);

    if (fgets(input, maxInput, stdin) == NULL) {
        perror("Erreur lors de la saisie");
    }

    input[strlen(input)-1] = '\0';

    return input;
}

int sendMessage(int destinataire, char* message) {

    int tailleMessage = (int) strlen(message)+1;
    int res = send(destinataire, &tailleMessage, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de l'envoi de la taille du message");
    }

    int nbTotalSent = 0;

    // Envoi du message par packet si le message est trop grand pour le buffer
    while(tailleMessage > nbTotalSent) {

        res = send(destinataire, message+nbTotalSent, tailleMessage, 0);

        if (res == -1) {
            perror("Erreur lors de l'envoi du message");
        }
        else {
            nbTotalSent += res;
        }
    }

    return nbTotalSent;
}

char* recvMessage(int expediteur, int tailleMax) {

    char* message = malloc(tailleMax);

    // Réception de la taille du message permettant de savoir quand tout le message sera reçu
    int tailleMessage;

    int res = recv(expediteur, &tailleMessage, sizeof(int), 0);

    if (res == -1) {
        perror("Erreur lors de la réception de la taille du message");
        exit(1);
    }

    int nbTotalRecv = 0;

    // Réception du message par packet si le message est trop grand pour le buffer
    while (nbTotalRecv < tailleMessage) {
        res = recv(expediteur, message+nbTotalRecv, tailleMessage, 0);
        if (res == -1) {
            perror("Erreur lors de la réception du message");
            free(message);
            return NULL;
        }
        else {
            nbTotalRecv += res;
        }
    }

    return message;
}

int contientFichier(char* loc, char* fichier) {

    int res = 0;
    struct dirent *dir;
    DIR *directory = opendir(loc);

    if (directory) {
        while ((dir = readdir(directory)) != NULL) {
            if (strcmp(dir->d_name, fichier) == 0) {
                res = 1;
                break;
            }
        }

        closedir(directory);
    }

    return res;
}