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
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"


#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

// Informations sur le client
struct {
    int dS; // Socket pour l'échange de messages
    int dS2; // Socket pour l'échange de fichiers
    int numeroClient; // Numéro du client dans le serveur
    char* pseudo; // Pseudo du client
    char pseudoExpediteur[21];
} infos;

// Variables globales
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
		printf("Connexion réussie à la socket %d\n", idSocket);

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


// Fonction d'envoi de fichier
void sendFile(int destinataire, char* name) {

    char loc[200] = "./files/";
    strcat(loc, name);
    FILE* file = NULL;
    file = fopen(loc, "r");
    int fsize = open(loc,O_RDONLY);
    char chaine[1000] = "";

    int res =send(destinataire, name, strlen(name),0);
    if(res == -1) {
        perror("erreur lors de l'envoi du nom du fichier");
        exit(1);
    }

    struct stat file_stat;
    if(fstat(fsize,&file_stat)<0){
        perror("erreur obtention des stats");
        exit(1);
    }
    int taille = file_stat.st_size;

    int part = send(destinataire,&taille,sizeof(int),0);
    if(part==0||part==-1){
        perror("Erreur envoi taille");
        exit(1);
    }
    int envoye=0;
    char buff[200];
    while(envoye<taille){
        int part = fread(buff,1,200,file);
        part = send(destinataire,buff,sizeof(buff),0);
        if(part<0){
            perror("Erreur");
            exit(1);
        }
        if (part < 200){
            if (feof(file)){
                printf("fin de fichier\n");
                break;
            }
            if (ferror(file)){
                printf("Erreur lors de la lecture du fichier\n");
                break;
            }

        }
        memset(buff,0,sizeof(buff));
        envoye=envoye+part;
    }
    printf("Fichier envoyé avec succès !\n");
    fclose(file);
}


// Fonction pour récupérer un fichier
void getFile(int expediteur) {

    char name[200];
    char loc[200]="./target/";
    FILE *fichierRecu;
    int res=recv(expediteur,name,sizeof(name),0);
    if(res==-1){
        perror("erreur lors de la reception de la taille");
        exit(1);
    }

    char newfile[200];
    strcpy(newfile,infos.pseudo);
    strcat(newfile,"_");

    strcat(newfile,name);
    strcat(loc,newfile);

    fichierRecu = fopen(loc, "w+");
    if (fichierRecu == NULL) {
        perror("Erreur lors de la création du fichier");
        exit(1);
    }
    int taille;


    res=recv(expediteur,&taille,sizeof(int),0);
    if(res==-1){
        perror("erreur lors de la reception de la taille");
        exit(1);
    }

    int recu=0;
    char part[200];
    int byte=0;
    while(recu<taille)
    {
        byte=recv(expediteur,part,sizeof(part),0);
        if(byte<0){
            perror("Erreur de reception");
            exit(1);
        }
        recu=recu+byte;
        fwrite(part, 1, 200, fichierRecu);

        if (byte < 200){
            if (feof(fichierRecu)){
                printf("Fin de fichier\n");
                break;

            }
            if (ferror(fichierRecu)){
                printf("Erreur de lecture\n");
                break;

            }

        }
        memset(part,0,sizeof(part));
    }

    printf("Fichier recu avec succès !\n");
    fclose(fichierRecu);
}

// Thread pour l'envoi de fichier
void *envoyerFichier(void * arg) {
    char* name = (char*) arg;
    sendFile(infos.dS2, name);
    pthread_exit(0);
}

void *recevoirFichier(void * arg) {

    int res;

    while(1){

        res = recv(infos.dS2, infos.pseudoExpediteur, sizeof(infos.pseudoExpediteur), 0);

        if (res == -1) {
            perror("Erreur lors de la réception du pseudo de l'expéditeur");
        }

        getFile(infos.dS2);
        printf("Fichier reçu de %s\n",infos.pseudoExpediteur);

        // On efface le pseudo de l'expéditeur
        memset(infos.pseudoExpediteur, 0, sizeof(infos.pseudoExpediteur));
    }

    pthread_exit(0);
}

// Fonction permettant de choisir le fichier à envoyer et de lancer le thread d'envoi
void debutEnvoiFile() {

    printf("Quel fichier voulez-vous envoyer ?\n");
    struct dirent *dir;
    char fichier[200];
    char loc[200] = "./files/";

    // opendir() renvoie un pointeur de type DIR.
    //on affiche les fichiers disponibles
    DIR *d = opendir(loc);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] != '.') {
                printf("\t%s\n", dir->d_name);
            }
        }

        closedir(d);
    }

    do {
        memset(fichier, 0, sizeof(fichier));
        printf("Entrez le nom du fichier : ");
        char* fic = saisie(200);
        strcpy(fichier, fic);
    } while(strlen(fichier)>199 || contientFichier(loc, fichier) == 0);


    pthread_t th;
    // lancement du thread qui va envoyer le fichier
    if (pthread_create(&th, NULL, envoyerFichier, (void *) fichier) < 0) {
        perror("Erreur lors de la création du thread envoi fichier");
        exit(1);
    }
}


// Fonction pour le thread qui envoie un message
void * envoyerMessage (void * arg) {

    int res;

    sendMessage(infos.dS, "begin238867");

    while(1) {
		if (fin == 1) {
			break;
		}

        char* message = malloc(1000);

        // Saisie du message
        do {
            if (strlen(message) >= 1000) {
                printf("Votre message est trop long (1000 caractères maximum)\n");
            }

            message = saisie(1000);

        } while (strlen(message) >= 1000);

        // Si "file" est envoyé cela signifie que l'on veut envoyer un fichier, on lance donc le thread d'envoi de fichier
		//si on demande l'envoi d'un fichier
        if (strcmp(message, "file") == 0) {
            debutEnvoiFile();
        }
        else {
            sendMessage(infos.dS, message);
        }

        // On stoppe le thread quand on envoie "fin"
		if (strcmp(message, "fin") == 0 || fin == 1) {
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

    while(1){
    	if (fin == 1) {
    		break;
    	}

    	res = recv(infos.dS, infos.pseudoExpediteur, sizeof(infos.pseudoExpediteur), 0);

    	if (res == -1) {
    		perror("Erreur lors de la réception du pseudo de l'expéditeur");
    		exit(1);
    	}

    	char* message = recvMessage(infos.dS, TAILLE_MAX);

        if (strcmp(message, "fin") == 0) {
            printf("%s a quitté la conversation\n", infos.pseudoExpediteur);
            nbUsers--;
        }
        else if(strcmp(message, "begin238867") == 0) {
            printf("%s a rejoint la conversation\n", infos.pseudoExpediteur);
            nbUsers++;
        }
        else {
            printf("Message reçu de %s : %s\n", infos.pseudoExpediteur, message);
        }


		// On efface le pseudo de l'expéditeur
		memset(infos.pseudoExpediteur, 0, sizeof(infos.pseudoExpediteur));
	}
	
	pthread_exit(0);
}

int main(int argc, char* argv[]) {

    infos.pseudo = malloc(20);

	// Vérification du nombre d'arguments
	if (argc != 3) {
		printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
		exit(1);
	}

	// Saisie du pseudo
	do {
		if (strlen(infos.pseudo) >= 20) {
			printf("Votre pseudo est trop long (20 caractères maximum)\n");
		}
		printf("(Taille max: 20, min : 1)\n" );
		printf("Entrez votre pseudo : ");

		infos.pseudo = saisie(20);

	} while (strlen(infos.pseudo) >= 20 || strlen(infos.pseudo) < 1);

	int port = atoi(argv[1]);
	char* ip = argv[2];

	infos.dS = connexionSocket(port, ip);
	infos.dS2 = connexionSocket(32566, ip);

	int res;

	// Envoi du pseudo au serveur
	res = send(infos.dS, infos.pseudo, strlen(infos.pseudo), 0);

	if (res == -1) {
		perror("Erreur lors de l'envoi du pseudo");
		exit(1);
	}

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
	if (pthread_join(th2, &ret) != 0){
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