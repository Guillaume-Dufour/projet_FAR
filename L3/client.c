#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <string.h>


#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

// Variables globales
char pseudo[21]; // Pseudo du client
char pseudoExpediteur[21]; // Pseudo de l'expéditeur du message
int dS; // Socket pour envoyer des messages
int dS1; // Socket pour échanger des informations avec le serveur
int fin = 0; // Variable pour savoir si le client doit s'arrêter
int users[NB_CLIENT_MAX];
int nbUser; // nb de clients

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
		exit(0);
	}
	else if (res == 0) {
		perror("Adresse réseau invalide");
		exit(0);
	}

	socklen_t lgA = sizeof(aS);
	res = connect(idSocket, (struct sockaddr *) &aS, lgA);

	if (res == -1) {
		perror("Erreur à la demande de connexion");
		exit(0);
	}
	else {
		printf("Connexion réussie à la socket %d\n", idSocket);

		return idSocket;
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

//demande la liste des users au serveur
void demandeUsersServeur(){

	printf("Demande de la liste des clients connectés\n");

	char message[15]= "user342107";

    // Envoi du message de demande des users
    int res = send(dS1, message, strlen(message)+1, 0);

    if (res == -1) {
        perror("Erreur lors de l'envoi du message");
    }

    int nbUser;

    res = recv(dS1, &nbUser, sizeof(nbUser), 0);

    if (res == -1) {
		perror("Erreur lors de la réception de nbuser");
	}

	printf("il y'a %d users connecté\n",nbUser );

    for (int i = 0; i < nbUser; i++) {
    	res = recv(dS1, &users[i], sizeof(int), 0);
	    if (res == -1) {
			perror("Erreur lors de la réception de nbuser");
		}
		printf("user %d :%d\n",i,users[i] );
    }

}

//thread qui va s'occuper de l'envoi d'un fichier

void *envoyerFichier (void * arg) { 
	char *loc = (char*) arg;
	FILE* file = NULL;
	file = fopen(loc, "r");
	char chaine[1000] = "";
	printf("%s\n",loc );

	printf("%d\n", users[0]);
	//ENVOYER AUX USERSSSSS

	
	if (file != NULL)
    {
        while (fgets(chaine, 1000, file) != NULL) // On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL)
        {
 
    		printf("%s", chaine); // On affiche la chaîne qu'on vient de lire
        }
         fclose(file);
    }
	printf("fin envoi fichier\n");
}

void *recevoirFichier (void * arg) { 
	
}

void debutEnvoiFile(){
	printf("Quel fichier voulez vous envoyer ?\n");
	struct dirent *dir;
	char fichier[200];
	char loc[200] = "./files/";
    // opendir() renvoie un pointeur de type DIR. 
    //on affiche les fichiers disponibles
    DIR *d = opendir(loc); 
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    do{
    	printf("Entrez le nom du fichier : ");
    	gets(fichier);
    } while(strlen(fichier)>199);

    demandeUsersServeur();
    
    strcat(loc, fichier);
	pthread_t th;
	void *ret;
	// lancement du thread qui va envoyer le fichier
	if (pthread_create(&th, NULL, envoyerFichier, (void *) loc) < 0) {
        perror("Erreur lors de la création du thread envoi fichier");
        exit(1);
    }
}

// Fonction permettant d'envoyer un message
int sendMessage(int destinaire) {

	char message[TAILLE_MAX];
    int res;

	// Saisie du message
    do {
    	if (strlen(message) >= 1000) {
    		printf("Votre message est trop long (1000 caractères maximum)\n");
    	}

		gets(message);

    } while (strlen(message) >= 1000);


    // Si "file" est envoyé cela signifie que l'on veut envoyer un fichier, on lance donc le thread d'envoi de fichier
    if (strcmp(message, "file") == 0) {
    	return -2;
    }
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
int getMessage(int expediteur) {

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
    	memset(message, 0, sizeof(message));
	}else{
	    printf("Message reçu de %s : %s\n", pseudoExpediteur, message);

	    return nbTotalRecv;
	}
}

// Fonction pour le thread qui envoie un message
void *envoyerMessage (void * arg) {   

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

		res = sendMessage(dS);
		//si on demande l'envoi d'un fichier
		if (res == -2){
			debutEnvoiFile();
		}

		// On stoppe le thread quand on envoie "fin" 
		else if (res == 0 || fin ==1) {
			fin = 1;
			break;
		}
	}
	printf("fin de la communication\n");
	pthread_exit(0);
}

//Fonction pour le thread qui reçoit un message
void *recevoirMessage (void * arg) {

    int res;

    while(1){
    	if (fin == 1) {
    		break;
    	}

    	res = recv(dS, &pseudoExpediteur, sizeof(pseudoExpediteur), 0);

    	if (res == -1) {
    		perror("Erreur lors de la réception du pseudo de l'expéditeur");
    	}

    	getMessage(dS);

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

	dS = connexionSocket(port, ip);
	dS1 = connexionSocket(32565, ip);

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

	if (pthread_create(&th1, NULL, recevoirMessage, "1") < 0) {
        perror("Erreur lors de la création du thread 1");
        exit(1);
    }
    
    if (pthread_create(&th2, NULL, envoyerMessage, "2") < 0) {
        perror("Erreur lors de la création du thread 2");
        exit(1);
    }

    if (pthread_create(&th1, NULL, recevoirFichier, "3") < 0) {
        perror("Erreur lors de la création du thread 3");
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