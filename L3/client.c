#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>



#define TAILLE_MAX 1001 // Taille maximum d'un message
#define NB_CLIENT_MAX 100 //nb max de clients

// Variables globales
char pseudo[21]; // Pseudo du client
char pseudoExpediteur[21]; // Pseudo de l'expéditeur du message
int dS; // Socket pour envoyer des messages
int dS2;
int fin = 0; // Variable pour savoir si le client doit s'arrêter
int nbUser; // nb de clients
int numeroClient;

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

//thread qui va s'occuper de l'envoi d'un fichier

void sendFile(int destinaire, char* name) {

	char loc[200] = "./files/";
	strcat(loc, name);
	FILE* file = NULL;
	file = fopen(loc, "r");
	int fsize = open(loc,O_RDONLY);
	char chaine[1000] = "";
	
	printf("%s\n",loc );

	int res =send(dS2,name,strlen(name),0);
	if(res == -1){
		perror("erreur lors de l'envoi du nom du fichier");
		exit(1);
	}

	struct stat file_stat;
    if(fstat(fsize,&file_stat)<0){
    	perror("erreur obtention des stats");
    	exit(1);
    }
	int taille = file_stat.st_size;
	
    printf("la taille est : %d \n",taille);
    int part =send(dS2,&taille,sizeof(int),0);
    if(part==0||part==-1){
        perror("Erreur envoi taille");
        exit(1);
    }
    int envoye=0;
	char buff[200];
    while(envoye<taille){
        int part = fread(buff,1,200,file);
        part = send(dS2,buff,sizeof(buff),0);
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
    printf("fichier envoyé\n");
    fclose(file);
    pthread_exit(0);

}

void getFile(int expediteur) {
	char name[200];
	char loc[200]="./target/";
	FILE *fichierRecu;
	int res=recv(dS2,name,sizeof(name),0);
    if(res==-1){
    	perror("erreur lors de la reception de la taille");
    	exit(1);
    }
    
	char newfile[200];
	strcpy(newfile,pseudo);
	strcat(newfile,"_");

	strcat(newfile,name);
	strcat(loc,newfile);
	
	fichierRecu = fopen(loc, "w+");
	if (fichierRecu == NULL) {
		perror("Erreur lors de la création du fichier");
		exit(1);
	}
	int taille;


    res=recv(dS2,&taille,sizeof(int),0);
    if(res==-1){
    	perror("erreur lors de la reception de la taille");
    	exit(1);
    }
    printf("la taille à recevoir est : %d\n",taille );
    int recu=0;
    char part[200];
    int byte=0;
    while(recu<taille)
    {
        byte=recv(dS2,part,sizeof(part),0);
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

    printf("fichier recu\n");
    fclose(fichierRecu);
}

void *envoyerFichier(void * arg) {
	char* name = (char*) arg;
	printf("%s\n",name );
	sendFile(dS2, name);
	pthread_exit(0);
}

void debutEnvoiFile(){
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
            	printf("%s\n", dir->d_name);
        	}
        }

        closedir(d);
    }

    do {
    	printf("Entrez le nom du fichier : ");
    	gets(fichier);
    } while(strlen(fichier)>199);

    //demandeUsersServeur();
    
    
	pthread_t th;
	void *ret;
	// lancement du thread qui va envoyer le fichier
	if (pthread_create(&th, NULL, envoyerFichier, (void *) fichier) < 0) {
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
    	nbUser--;
    }
    else if(strcmp(message, "begin238867") == 0){
    	printf("%s a rejoint la conversation\n",pseudoExpediteur );
    	memset(message, 0, sizeof(message));
    	nbUser++;
	} 
	else{
	    printf("Message reçu de %s : %s\n", pseudoExpediteur, message);

	    return nbTotalRecv;
	}
}

void *recevoirFichier(void * arg) {

	int res;

    while(1){

    	res = recv(dS2, &pseudoExpediteur, sizeof(pseudoExpediteur), 0);

    	if (res == -1) {
    		perror("Erreur lors de la réception du pseudo de l'expéditeur");
    	}

    	getFile(dS2);
    	printf("fichier recu de %s\n",pseudoExpediteur);

		// On efface le pseudo de l'expéditeur
		memset(pseudoExpediteur, 0, sizeof(pseudoExpediteur));
	}
	
	pthread_exit(0);

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
	dS2 = connexionSocket(32566, ip);

	int res;

	// Envoi du pseudo au serveur
	res = send(dS, pseudo, strlen(pseudo), 0);

	if (res == -1) {
		perror("Erreur lors de l'envoi du pseudo");
	}

	numeroClient = receptionNumeroClient();

	

	// Création des 2 threads
	pthread_t th1, th2, th3;
	void *ret;

	if (pthread_create(&th1, NULL, recevoirMessage, "1") < 0) {
        perror("Erreur lors de la création du thread 1");
        exit(1);
    }
    
    if (pthread_create(&th2, NULL, envoyerMessage, "2") < 0) {
        perror("Erreur lors de la création du thread 2");
        exit(1);
    }

    if (pthread_create(&th3, NULL, recevoirFichier, "3") < 0) {
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